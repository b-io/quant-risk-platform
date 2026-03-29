param (
    [string]$PortfolioId = "demo_macro_book",
    [string]$SnapshotId = "SNAP:2024-03-24",
    [string]$ScenarioSetId = "SC_SET_HIST_01",
    [string]$BuildDir = "build\Debug",
    [string]$LogLevel = "info"
)

$CliExe = Join-Path $BuildDir "qrp_cli.exe"

if (!(Test-Path $CliExe)) {
    Write-Error "Could not find qrp_cli.exe at $CliExe. Please build the project first."
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
