# test.ps1 - Run all platform tests

# Default build directory
$BuildDir = "build\Debug"

Write-Host "Building tests..." -ForegroundColor Cyan
cmake --build $BuildDir --target unit_tests integration_tests
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Running Unit Tests..." -ForegroundColor Green
& "$BuildDir\tests\unit_tests.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Running Integration Tests..." -ForegroundColor Green
& "$BuildDir\tests\integration_tests.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "All tests passed successfully!" -ForegroundColor Green
