param (
    [string]$BuildDir = "build\dev",
    [string]$Config = "RelWithDebInfo",
    [string]$LogLevel = "info",
    [string]$PortfolioId = "demo_portfolio",
    [string]$ScenarioSetId = "demo_factor_scenarios",
    [string]$SnapshotId = "SNAP:2026-03-24",
    [switch]$SkipEnv
)

$scriptPath = $MyInvocation.MyCommand.Path
$projectRoot = Split-Path (Split-Path $scriptPath -Parent) -Parent
Set-Location -LiteralPath $projectRoot

if (-not $SkipEnv) {
    $envScript = Join-Path $projectRoot "scripts\env.ps1"
    if (Test-Path -LiteralPath $envScript) {
        & $envScript -ProjectRoot $projectRoot -Quiet
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
}

# Helper to find executable
function Find-Exe($ExeName) {
    $names = @($ExeName)
    if (-not $ExeName.EndsWith(".exe", [System.StringComparison]::OrdinalIgnoreCase)) {
        $names += "$ExeName.exe"
    }

    foreach ($name in $names) {
        $Paths = @(
            "$BuildDir\$name",
            "$BuildDir\$Config\$name",
            "$BuildDir\bin\$name",
            "$BuildDir\bin\$Config\$name"
        )
        foreach ($Path in $Paths) {
            if (Test-Path $Path) { return $Path }
        }
    }
    return $null
}

$CliExe = Find-Exe "qrp_cli"

if ($null -eq $CliExe) {
    Write-Error "Could not find qrp_cli executable in $BuildDir or nested directories. Please build the project first."
    exit 1
}

# Set log level for the session
$env:QRP_LOG_LEVEL = $LogLevel

Write-Host "--- Quant Risk Platform: Compute Flow ---" -ForegroundColor Cyan
Write-Host "Portfolio: $PortfolioId"
Write-Host "Snapshot:  $SnapshotId"
Write-Host "Scenarios: $ScenarioSetId"
Write-Host "----------------------------------------"

# 1. Run Valuation
Write-Host "`n[1/3] Running Valuation..." -ForegroundColor Yellow
$ValRunOutput = & $CliExe run-valuation --portfolio $PortfolioId --snapshot $SnapshotId
if ($LASTEXITCODE -ne 0) { Write-Error "Valuation failed"; exit 1 }

# Extract Run ID from output (expecting "Run ID: RUN_VAL_...")
$ValRunId = ($ValRunOutput | Select-String "Run ID: (.*)").Matches.Groups[1].Value
Write-Host "Valuation Run ID: $ValRunId" -ForegroundColor Green

# 2. Run Risk (Sensitivities)
Write-Host "`n[2/3] Running Risk (Sensitivities)..." -ForegroundColor Yellow
$RiskRunOutput = & $CliExe run-risk --portfolio $PortfolioId --snapshot $SnapshotId
if ($LASTEXITCODE -ne 0) { Write-Error "Risk calculation failed"; exit 1 }

$RiskRunId = ($RiskRunOutput | Select-String "Run ID: (.*)").Matches.Groups[1].Value
Write-Host "Risk Run ID: $RiskRunId" -ForegroundColor Green

# 3. Run Historical VaR
Write-Host "`n[3/3] Running Historical VaR..." -ForegroundColor Yellow
$VarRunOutput = & $CliExe run-hvar --portfolio $PortfolioId --snapshot $SnapshotId --scenarios $ScenarioSetId
if ($LASTEXITCODE -ne 0) { Write-Error "VaR calculation failed"; exit 1 }

$VarRunId = ($VarRunOutput | Select-String "Run ID: (.*)").Matches.Groups[1].Value
Write-Host "VaR Run ID: $VarRunId" -ForegroundColor Green

# 4. Generate Final Report
Write-Host "`n--- Final Valuation Report ---" -ForegroundColor Cyan
& $CliExe report $ValRunId

Write-Host "`nComputation flow completed successfully!" -ForegroundColor Green
