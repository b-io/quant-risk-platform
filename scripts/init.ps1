param(
    [string]$BuildDir = "build\Release-Python",
    [string]$Config = "Release",
    [string]$MarketFile = "data\market\demo_market.json",
    [string]$PortfolioFile = "data\portfolios\demo_portfolio.json",
    [string]$ScenarioFile = "data\scenarios\demo_scenarios.json",
    [switch]$Force = $false,
    [switch]$SkipEnv
)

# init.ps1 - Initialize the database and load sample data

$scriptPath = $MyInvocation.MyCommand.Path
$projectRoot = Split-Path (Split-Path $scriptPath -Parent) -Parent

if (-not $SkipEnv) {
    $envScript = Join-Path $projectRoot "scripts\env.ps1"
    if (Test-Path -LiteralPath $envScript) {
        & $envScript -ProjectRoot $projectRoot -Quiet
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
}

# Helper to find executable in either $BuildDir\ or $BuildDir\$Config\
function Find-Exe($ExeName) {
    $Paths = @(
        "$BuildDir\$ExeName",
        "$BuildDir\$Config\$ExeName",
        "$BuildDir\bin\$ExeName",
        "$BuildDir\bin\$Config\$ExeName"
    )
    foreach ($Path in $Paths) {
        if (Test-Path $Path) { return $Path }
    }
    return $null
}

$CliExe = Find-Exe "qrp_cli.exe"
$DbFile = "var\quant_risk_platform.sqlite"

if ($null -eq $CliExe) {
    Write-Host "Error: qrp_cli.exe not found in $BuildDir or nested directories." -ForegroundColor Red
    exit 1
}

$dbDirectory = Split-Path -Parent $DbFile
if ($dbDirectory -and !(Test-Path -LiteralPath $dbDirectory)) {
    New-Item -ItemType Directory -Path $dbDirectory -Force | Out-Null
}

if (Test-Path -LiteralPath $DbFile) {
    if (!$Force) {
        $title = "Warning: Database Exists"
        $message = "Database $DbFile already exists. Do you want to delete and recreate it?"
        $yes = New-Object System.Management.Automation.Host.ChoiceDescription "&Yes", "Deletes and recreates the database."
        $no = New-Object System.Management.Automation.Host.ChoiceDescription "&No", "Cancels the operation."
        $options = [System.Management.Automation.Host.ChoiceDescription[]]($yes, $no)
        $result = $host.ui.PromptForChoice($title, $message, $options, 1)
        
        if ($result -ne 0) {
            Write-Host "Initialization cancelled."
            exit 0
        }
    }
    
    Write-Host "Deleting $DbFile..." -ForegroundColor Yellow
    Remove-Item -LiteralPath $DbFile
}

Write-Host "Initializing database schema..." -ForegroundColor Cyan
& $CliExe init-db

Write-Host "Importing sample market data..." -ForegroundColor Cyan
& $CliExe import-market $MarketFile

Write-Host "Importing sample portfolios..." -ForegroundColor Cyan
& $CliExe import-portfolio $PortfolioFile

Write-Host "Importing sample scenarios..." -ForegroundColor Cyan
& $CliExe import-scenarios $ScenarioFile

Write-Host "Data initialization complete!" -ForegroundColor Green
& $CliExe list
