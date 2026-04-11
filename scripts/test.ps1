param(
    [string]$BuildDir = "build\Release-Python",
    [string]$Config = "Release"
)

# test.ps1 - Run all platform tests

Write-Host "Building tests (Config: $Config) in $BuildDir..." -ForegroundColor Cyan
cmake --build $BuildDir --target unit_tests integration_tests --config $Config
if ($LASTEXITCODE -ne 0) { 
    Write-Host "Build failed." -ForegroundColor Red
    exit $LASTEXITCODE 
}

# Helper to find executable in either $BuildDir\tests\ or $BuildDir\tests\$Config\
function Find-TestExe($ExeName) {
    $Paths = @(
        "$BuildDir\tests\$ExeName",
        "$BuildDir\tests\$Config\$ExeName"
    )
    foreach ($Path in $Paths) {
        if (Test-Path $Path) { return $Path }
    }
    return $null
}

$UnitTestExe = Find-TestExe "unit_tests.exe"
$IntegrationTestExe = Find-TestExe "integration_tests.exe"

if ($null -eq $UnitTestExe) {
    Write-Host "Error: unit_tests.exe not found in $BuildDir\tests\ or $BuildDir\tests\$Config\" -ForegroundColor Red
    exit 1
}

Write-Host "Running Unit Tests: $UnitTestExe" -ForegroundColor Green
& $UnitTestExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($null -eq $IntegrationTestExe) {
    Write-Host "Error: integration_tests.exe not found in $BuildDir\tests\ or $BuildDir\tests\$Config\" -ForegroundColor Red
    exit 1
}

Write-Host "Running Integration Tests: $IntegrationTestExe" -ForegroundColor Green
& $IntegrationTestExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "All tests passed successfully!" -ForegroundColor Green
