# init.ps1 - Initialize the database and load sample data

# Default build directory
$BuildDir = "build\Debug"
$CliExe = "$BuildDir\qrp_cli.exe"
$DbFile = "var\quant_risk_platform.sqlite"

# Ensure var directory exists
if (!(Test-Path var)) { New-Item -ItemType Directory -Path var }

if (Test-Path $DbFile) {
    $title = "Warning: Database Exists"
    $message = "Database $DbFile already exists. Do you want to delete and recreate it?"
    $yes = New-Object System.Management.Automation.Host.ChoiceDescription "&Yes", "Deletes and recreates the database."
    $no = New-Object System.Management.Automation.Host.ChoiceDescription "&No", "Cancels the operation."
    $options = [System.Management.Automation.Host.ChoiceDescription[]]($yes, $no)
    $result = $host.ui.PromptForChoice($title, $message, $options, 1)
    
    if ($result -eq 0) {
        Write-Host "Deleting $DbFile..." -ForegroundColor Yellow
        Remove-Item $DbFile
    } else {
        Write-Host "Initialization cancelled."
        exit 0
    }
}

Write-Host "Initializing database schema..." -ForegroundColor Cyan
& $CliExe init-db

Write-Host "Importing sample market data..." -ForegroundColor Cyan
& $CliExe import-market data\market\base_market.json

Write-Host "Importing sample portfolios..." -ForegroundColor Cyan
& $CliExe import-portfolio data\portfolios\demo_macro_book.json

Write-Host "Importing sample scenarios..." -ForegroundColor Cyan
& $CliExe import-scenarios data\scenarios\sample_scenarios.json

Write-Host "Data initialization complete!" -ForegroundColor Green
& $CliExe list
