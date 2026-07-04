param(
    [string]$BuildDir = "build\Release-Python",
    [string]$Config = "Release",
    [string]$PythonExecutable = "",
    [double]$CppCoverageMinLine = 0.0,
    [double]$PythonCoverageMinLine = 0.0,
    [switch]$Coverage,
    [switch]$SkipEnv,
    [switch]$SkipCppCoverage,
    [switch]$SkipPythonCoverage
)

# test.ps1 - Run all platform tests and print coverage summaries.

$scriptPath = $MyInvocation.MyCommand.Path
$projectRoot = Split-Path (Split-Path $scriptPath -Parent) -Parent

function Resolve-ProjectPath($Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-Path $projectRoot $Path
}

$ResolvedBuildDir = Resolve-ProjectPath $BuildDir

if (-not $SkipEnv) {
    $envScript = Join-Path $projectRoot "scripts\env.ps1"
    if (Test-Path -LiteralPath $envScript) {
        & $envScript -ProjectRoot $projectRoot -Quiet
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
}

function Resolve-PythonForCoverage {
    if ($PythonExecutable) {
        return $PythonExecutable
    }

    $candidates = @()
    if ($env:QRP_PYTHON_EXECUTABLE) {
        $candidates += $env:QRP_PYTHON_EXECUTABLE
    }
    if ($env:VCPKG_ROOT -and $env:VCPKG_TARGET_TRIPLET) {
        $candidates += Join-Path $env:VCPKG_ROOT "installed\$env:VCPKG_TARGET_TRIPLET\tools\python3\python.exe"
    }
    $candidates += @("python3", "python")

    foreach ($candidate in $candidates) {
        if ((Test-Path -LiteralPath $candidate) -or (Get-Command $candidate -ErrorAction SilentlyContinue)) {
            return $candidate
        }
    }
    return $null
}

function Resolve-VsCoverageTool {
    $command = Get-Command "Microsoft.CodeCoverage.Console.exe" -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $candidates = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe",
        "C:\Program Files\Microsoft Visual Studio\17\Community\Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }
    return $null
}

function Invoke-MsvcCppCoverage($CoverageTool, $Python, $UnitExe, $IntegrationExe) {
    if ($null -eq $Python) {
        Write-Warning "Python executable was not found; C++ coverage XML was not converted to the platform metric."
        return 0
    }

    $coverageDir = Join-Path $ResolvedBuildDir "coverage\cpp"
    New-Item -ItemType Directory -Force -Path $coverageDir | Out-Null

    $unitCoverage = Join-Path $coverageDir "cpp_unit.coverage"
    $integrationCoverage = Join-Path $coverageDir "cpp_integration.coverage"
    $coverageXml = Join-Path $coverageDir "coverage.xml"
    $coverageJson = Join-Path $coverageDir "coverage_metric.json"
    $coverageMarkdown = Join-Path $coverageDir "coverage_metric.md"

    Remove-Item -LiteralPath $unitCoverage, $integrationCoverage, $coverageXml, $coverageJson, $coverageMarkdown -ErrorAction SilentlyContinue

    $targets = @(
        @{ Name = "unit"; Exe = $UnitExe; Output = $unitCoverage },
        @{ Name = "integration"; Exe = $IntegrationExe; Output = $integrationCoverage }
    )
    foreach ($target in $targets) {
        Write-Host "Collecting C++ coverage for $($target.Name) tests..." -ForegroundColor Cyan
        $targetExe = $target.Exe
        $targetOutput = $target.Output
        & $CoverageTool collect `
            --nologo `
            --output $targetOutput `
            --output-format coverage `
            --include-files $targetExe `
            $targetExe
        if ($LASTEXITCODE -ne 0) {
            Write-Host "C++ coverage collection failed for $($target.Name) tests." -ForegroundColor Red
            return $LASTEXITCODE
        }
    }

    Write-Host "Merging C++ coverage reports..." -ForegroundColor Cyan
    & $CoverageTool merge `
        $unitCoverage `
        $integrationCoverage `
        --nologo `
        --output $coverageXml `
        --output-format cobertura
    if ($LASTEXITCODE -ne 0) {
        Write-Host "C++ coverage merge failed." -ForegroundColor Red
        return $LASTEXITCODE
    }

    $coverageMetricScript = Join-Path $projectRoot "scripts\coverage\cpp\coverage_metric.py"
    & $Python $coverageMetricScript `
        --xml $coverageXml `
        --json $coverageJson `
        --markdown $coverageMarkdown `
        --threshold $CppCoverageMinLine `
        --include-source-prefix (Join-Path $projectRoot "cpp")
    return $LASTEXITCODE
}

function Show-CppCoverageScore {
    $metricPath = Join-Path $ResolvedBuildDir "coverage\cpp\coverage_metric.json"
    if (-not (Test-Path -LiteralPath $metricPath)) {
        Write-Host "C++ Coverage Score: not generated." -ForegroundColor Yellow
        return
    }

    $metric = Get-Content -LiteralPath $metricPath -Raw | ConvertFrom-Json
    $lineCoverage = ([double]$metric.line_coverage_percent).ToString("F2", [System.Globalization.CultureInfo]::InvariantCulture)
    $threshold = ([double]$metric.threshold_percent).ToString("F2", [System.Globalization.CultureInfo]::InvariantCulture)
    Write-Host ("C++ Coverage Score: {0}% line ({1}, threshold {2}%, {3}/{4})" -f `
        $lineCoverage, $metric.status, $threshold, `
        $metric.lines_covered, $metric.lines_valid) -ForegroundColor Cyan
}

function Show-PythonCoverageScore {
    if ($SkipPythonCoverage) {
        Write-Host "Python Coverage Score: skipped." -ForegroundColor Yellow
        return
    }

    $metricPath = Join-Path $ResolvedBuildDir "coverage\python\coverage_metric.json"
    if (-not (Test-Path -LiteralPath $metricPath)) {
        Write-Host "Python Coverage Score: not generated." -ForegroundColor Yellow
        return
    }

    $metric = Get-Content -LiteralPath $metricPath -Raw | ConvertFrom-Json
    $lineCoverage = ([double]$metric.line_coverage_percent).ToString("F2", [System.Globalization.CultureInfo]::InvariantCulture)
    $threshold = ([double]$metric.threshold_percent).ToString("F2", [System.Globalization.CultureInfo]::InvariantCulture)
    Write-Host ("Python Coverage Score: {0}% line ({1}, tests {2}, threshold {3}%, {4}/{5})" -f `
        $lineCoverage, $metric.status, $metric.tests_run, `
        $threshold, $metric.lines_covered, $metric.lines_valid) -ForegroundColor Cyan
}

Write-Host "Building tests (Config: $Config) in $ResolvedBuildDir..." -ForegroundColor Cyan
cmake --build $ResolvedBuildDir --target unit_tests integration_tests --config $Config
if ($LASTEXITCODE -ne 0) { 
    Write-Host "Build failed." -ForegroundColor Red
    exit $LASTEXITCODE 
}

# Helper to find executable in either $BuildDir\tests\ or $BuildDir\tests\$Config\
function Find-TestExe($ExeName) {
    $Paths = @(
        "$ResolvedBuildDir\tests\$ExeName",
        "$ResolvedBuildDir\tests\$Config\$ExeName"
    )
    foreach ($Path in $Paths) {
        if (Test-Path $Path) { return $Path }
    }
    return $null
}

$UnitTestExe = Find-TestExe "unit_tests.exe"
$IntegrationTestExe = Find-TestExe "integration_tests.exe"

if ($null -eq $UnitTestExe) {
    Write-Host "Error: unit_tests.exe not found in $ResolvedBuildDir\tests\ or $ResolvedBuildDir\tests\$Config\" -ForegroundColor Red
    exit 1
}

Write-Host "Running Unit Tests: $UnitTestExe" -ForegroundColor Green
& $UnitTestExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($null -eq $IntegrationTestExe) {
    Write-Host "Error: integration_tests.exe not found in $ResolvedBuildDir\tests\ or $ResolvedBuildDir\tests\$Config\" -ForegroundColor Red
    exit 1
}

Write-Host "Running Integration Tests: $IntegrationTestExe" -ForegroundColor Green
& $IntegrationTestExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ResolvedPython = Resolve-PythonForCoverage
$CppCoverageExitCode = 0
if (-not $SkipCppCoverage) {
    $vsCoverageTool = Resolve-VsCoverageTool
    if ($vsCoverageTool) {
        $CppCoverageExitCode = Invoke-MsvcCppCoverage $vsCoverageTool $ResolvedPython $UnitTestExe $IntegrationTestExe
    } elseif ($Coverage) {
        Write-Host "Visual Studio coverage tool was not found; trying the CMake coverage target." -ForegroundColor Yellow
        cmake --build $ResolvedBuildDir --target coverage --config $Config
        $CppCoverageExitCode = $LASTEXITCODE
    } else {
        Write-Warning "C++ coverage tool was not found; C++ coverage was not generated."
    }
}
if ($CppCoverageExitCode -ne 0) {
    exit $CppCoverageExitCode
}

if ($Coverage -and $SkipCppCoverage) {
    Write-Host "Building C++ coverage target in $ResolvedBuildDir..." -ForegroundColor Cyan
    cmake --build $ResolvedBuildDir --target coverage --config $Config
    if ($LASTEXITCODE -ne 0) {
        Write-Host "C++ coverage target failed." -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

$PythonCoverageExitCode = 0
if (-not $SkipPythonCoverage) {
    if ($null -eq $ResolvedPython) {
        Write-Warning "Python executable was not found; Python coverage was not generated."
    } else {
        $pythonCoverageScript = Join-Path $projectRoot "scripts\coverage\python\coverage_metric.py"
        Write-Host "Running Python coverage with $ResolvedPython..." -ForegroundColor Cyan
        & $ResolvedPython $pythonCoverageScript `
            --project-root $projectRoot `
            --source (Join-Path $projectRoot "python\qrp") `
            --tests (Join-Path $projectRoot "tests\python") `
            --json (Join-Path $ResolvedBuildDir "coverage\python\coverage_metric.json") `
            --markdown (Join-Path $ResolvedBuildDir "coverage\python\coverage_metric.md") `
            --threshold $PythonCoverageMinLine
        $PythonCoverageExitCode = $LASTEXITCODE
    }
}

Write-Host "All tests passed successfully!" -ForegroundColor Green
Write-Host "Coverage Summary" -ForegroundColor Cyan
Show-CppCoverageScore
Show-PythonCoverageScore

if ($PythonCoverageExitCode -ne 0) {
    exit $PythonCoverageExitCode
}
