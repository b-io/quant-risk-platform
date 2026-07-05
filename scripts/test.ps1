param(
    [string]$BuildDir = "build\dev",
    [string]$Config = "RelWithDebInfo",
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

Set-Location -LiteralPath $projectRoot

function Write-Section($Title) {
    Write-Host ""
    Write-Host "========================================================================" -ForegroundColor Cyan
    Write-Host $Title -ForegroundColor Cyan
    Write-Host "========================================================================" -ForegroundColor Cyan
}

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
        $candidates += Join-Path $env:VCPKG_ROOT "installed\$env:VCPKG_TARGET_TRIPLET\tools\python3\python"
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
    if ($env:QRP_VS_COVERAGE_TOOL -and (Test-Path -LiteralPath $env:QRP_VS_COVERAGE_TOOL)) {
        return $env:QRP_VS_COVERAGE_TOOL
    }

    $command = Get-Command "Microsoft.CodeCoverage.Console.exe" -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    return $null
}

function Invoke-NativeProcess($FilePath, [string[]]$Arguments) {
    $output = & $FilePath @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    foreach ($line in $output) {
        Write-Host $line
    }
    return $exitCode
}

function Invoke-MsvcCppCoverage($CoverageTool, $Python, $UnitExe) {
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

    # Integration tests run above, but MSVC native instrumentation currently crashes this Release integration binary.
    # Keep coverage deterministic with the unit test executable until a coverage-specific build profile is available.
    Write-Host "MSVC C++ coverage uses unit tests; integration tests were run normally before coverage." -ForegroundColor Yellow
    $targets = @(
        @{ Name = "unit"; Exe = $UnitExe; Output = $unitCoverage }
    )
    foreach ($target in $targets) {
        Write-Host "Collecting C++ coverage for $($target.Name) tests..." -ForegroundColor Cyan
        $targetExe = $target.Exe
        $targetOutput = $target.Output
        $collectExitCode = Invoke-NativeProcess $CoverageTool @(
            "collect",
            "--nologo",
            "--output", $targetOutput,
            "--output-format", "coverage",
            "--include-files", $targetExe,
            $targetExe
        )
        if ($collectExitCode -ne 0) {
            Write-Host "C++ coverage collection failed for $($target.Name) tests." -ForegroundColor Red
            return $collectExitCode
        }
    }

    Write-Host "Merging C++ coverage reports..." -ForegroundColor Cyan
    $mergeExitCode = Invoke-NativeProcess $CoverageTool @(
        "merge",
        $unitCoverage,
        "--nologo",
        "--output", $coverageXml,
        "--output-format", "cobertura"
    )
    if ($mergeExitCode -ne 0) {
        Write-Host "C++ coverage merge failed." -ForegroundColor Red
        return $mergeExitCode
    }

    $coverageMetricScript = Join-Path $projectRoot "coverage\cpp\coverage_metric.py"
    return Invoke-NativeProcess $Python @(
        $coverageMetricScript,
        "--xml", $coverageXml,
        "--json", $coverageJson,
        "--markdown", $coverageMarkdown,
        "--threshold", $CppCoverageMinLine,
        "--include-source-prefix", (Join-Path $projectRoot "cpp")
    )
}

function Show-CppCoverageScore {
    if ($SkipCppCoverage) {
        Write-Host "C++ Coverage Score: skipped." -ForegroundColor Yellow
        return
    }

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

function Update-CoverageSummary {
    if (-not $Coverage -or $SkipCppCoverage -or $SkipPythonCoverage -or $null -eq $ResolvedPython) {
        return
    }

    $cppMetric = Join-Path $ResolvedBuildDir "coverage\cpp\coverage_metric.json"
    $pythonMetric = Join-Path $ResolvedBuildDir "coverage\python\coverage_metric.json"
    $summaryScript = Join-Path $projectRoot "coverage\coverage_summary.py"
    if (-not (Test-Path -LiteralPath $cppMetric) -or -not (Test-Path -LiteralPath $pythonMetric)) {
        return
    }

    Write-Host "Updating committed coverage summary artifacts..." -ForegroundColor Cyan
    & $ResolvedPython $summaryScript `
        --cpp-json $cppMetric `
        --python-json $pythonMetric `
        --output-dir (Join-Path $projectRoot "coverage")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Write-Section "Build"
Write-Host "Building tests (Config: $Config) in $ResolvedBuildDir..." -ForegroundColor Cyan
$buildTargets = @("unit_tests", "integration_tests")
$targetHelp = & cmake --build $ResolvedBuildDir --target help --config $Config 2>&1
if ($targetHelp | Select-String -Pattern "(^|\s)quant_risk_platform(:|\s|$)" -Quiet) {
    $buildTargets += "quant_risk_platform"
}
cmake --build $ResolvedBuildDir --target $buildTargets --config $Config
if ($LASTEXITCODE -ne 0) { 
    Write-Host "Build failed." -ForegroundColor Red
    exit $LASTEXITCODE 
}

# Helper to find executable in either $BuildDir\tests\ or $BuildDir\tests\$Config\
function Find-TestExe($ExeName) {
    $names = @($ExeName)
    if (-not $ExeName.EndsWith(".exe", [System.StringComparison]::OrdinalIgnoreCase)) {
        $names += "$ExeName.exe"
    }

    foreach ($name in $names) {
        $Paths = @(
            "$ResolvedBuildDir\tests\$name",
            "$ResolvedBuildDir\tests\$Config\$name"
        )
        foreach ($Path in $Paths) {
            if (Test-Path $Path) { return $Path }
        }
    }
    return $null
}

$UnitTestExe = Find-TestExe "unit_tests"
$IntegrationTestExe = Find-TestExe "integration_tests"

if ($null -eq $UnitTestExe) {
    Write-Host "Error: unit_tests executable not found in $ResolvedBuildDir\tests\ or $ResolvedBuildDir\tests\$Config\" -ForegroundColor Red
    exit 1
}

Write-Section "C++ Tests"
Write-Host "Running Unit Tests: $UnitTestExe" -ForegroundColor Green
& $UnitTestExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($null -eq $IntegrationTestExe) {
    Write-Host "Error: integration_tests executable not found in $ResolvedBuildDir\tests\ or $ResolvedBuildDir\tests\$Config\" -ForegroundColor Red
    exit 1
}

Write-Host "Running Integration Tests: $IntegrationTestExe" -ForegroundColor Green
& $IntegrationTestExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ResolvedPython = Resolve-PythonForCoverage
$CppCoverageExitCode = 0
if (-not $SkipCppCoverage) {
    if ($Coverage) {
        $vsCoverageTool = Resolve-VsCoverageTool
        if ($vsCoverageTool) {
            $CppCoverageExitCode = Invoke-MsvcCppCoverage $vsCoverageTool $ResolvedPython $UnitTestExe
        } else {
            Write-Host "Visual Studio coverage tool was not found; trying the CMake coverage target." -ForegroundColor Yellow
            cmake --build $ResolvedBuildDir --target coverage --config $Config
            $CppCoverageExitCode = $LASTEXITCODE
        }
    } else {
        Write-Host "C++ coverage was not requested; pass -Coverage to generate it." -ForegroundColor Yellow
    }
}
if ($CppCoverageExitCode -ne 0) {
    exit $CppCoverageExitCode
}

Write-Host "C++ Section Complete" -ForegroundColor Green
Show-CppCoverageScore

Write-Section "Python Tests"
$PythonCoverageExitCode = 0
if ($null -eq $ResolvedPython) {
    Write-Warning "Python executable was not found; Python tests and coverage were not generated."
} else {
    Write-Host "Running Python binding smoke tests with $ResolvedPython..." -ForegroundColor Green
    ctest --test-dir $ResolvedBuildDir -C $Config -R "python_import" --output-on-failure
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    if ($SkipPythonCoverage) {
        Write-Host "Running Python unit tests without coverage with $ResolvedPython..." -ForegroundColor Green
        & $ResolvedPython -m unittest discover `
            -s (Join-Path $projectRoot "tests\python") `
            -p "test_*.py" `
            -v
        $PythonCoverageExitCode = $LASTEXITCODE
    } else {
        $pythonCoverageScript = Join-Path $projectRoot "coverage\python\coverage_metric.py"
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

Write-Host "Python Section Complete" -ForegroundColor Green
Show-PythonCoverageScore

if ($PythonCoverageExitCode -ne 0) {
    exit $PythonCoverageExitCode
}

Update-CoverageSummary

Write-Section "Final Coverage Summary"
Show-CppCoverageScore
Show-PythonCoverageScore
Write-Host "All tests passed successfully!" -ForegroundColor Green
