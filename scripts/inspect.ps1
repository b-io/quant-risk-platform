param(
    [string]$Table = "summary",
    [string]$Id = "",
    [string]$BuildDir = "build\dev",
    [string]$Config = "RelWithDebInfo",
    [string]$DbFile = "var\quant_risk_platform.sqlite",
    [switch]$SkipEnv
)

# inspect.ps1 - Inspect persisted data and run outputs from the database

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

if (!(Test-Path $DbFile)) {
    Write-Host "Database file $DbFile not found." -ForegroundColor Red
    exit 1
}

function Run-Query($sql) {
    if (Get-Command sqlite3 -ErrorAction SilentlyContinue) {
        sqlite3 -header -column "$DbFile" "$sql"
    } else {
        Write-Host "sqlite3 not found. For richer inspection, install sqlite3." -ForegroundColor Yellow
        Write-Host "Falling back to qrp_cli list for basic info..." -ForegroundColor Cyan
        if ($null -ne $CliExe) {
            & $CliExe list
        } else {
            Write-Host "Error: qrp_cli executable not found to fallback." -ForegroundColor Red
        }
    }
}

switch ($Table) {
    "portfolios" {
        Write-Host "=== Portfolios ===" -ForegroundColor Cyan
        Run-Query "SELECT portfolio_id, portfolio_name, base_currency FROM portfolios;"
    }
    "trades" {
        if ($Id -eq "") {
            Write-Host "Usage: .\scripts\inspect.ps1 trades -Id <portfolio_id>"
            exit 1
        }
        Write-Host "=== Trades for $Id ===" -ForegroundColor Cyan
        Run-Query "SELECT trade_id, book_id, asset_class, trade_type, currency, notional FROM trades WHERE portfolio_id = '$Id';"
    }
    "runs" {
        Write-Host "=== Analysis Runs ===" -ForegroundColor Cyan
        Run-Query "SELECT run_id, run_type, portfolio_id, snapshot_id, created_at FROM analysis_runs ORDER BY created_at DESC;"
    }
    "results" {
        if ($Id -eq "") {
            Write-Host "Usage: .\scripts\inspect.ps1 results -Id <run_id>"
            exit 1
        }
        if (Get-Command sqlite3 -ErrorAction SilentlyContinue) {
            Write-Host "=== Valuation Results for $Id ===" -ForegroundColor Cyan
            Run-Query "SELECT trade_id, npv_base, valuation_ccy, status, asset_class, product_type, support_status, model_name, status_message, error_message FROM valuation_results WHERE run_id = '$Id';"
            Write-Host "=== Risk Results for $Id ===" -ForegroundColor Cyan
            Run-Query "SELECT trade_id, risk_measure, risk_factor_id, value FROM risk_results WHERE run_id = '$Id';"
            Write-Host "=== Scenario Results for $Id ===" -ForegroundColor Cyan
            Run-Query "SELECT scenario_name, portfolio_pnl FROM scenario_results WHERE run_id = '$Id';"
            Write-Host "=== VaR Results for $Id ===" -ForegroundColor Cyan
            Run-Query "SELECT method, confidence_level, var_value, expected_shortfall, scenario_count FROM var_results WHERE run_id = '$Id';"
        } elseif ($null -ne $CliExe) {
            Write-Host "Using qrp_cli report for $Id ..." -ForegroundColor Cyan
            & $CliExe report $Id
        } else {
            Write-Host "Error: sqlite3 and qrp_cli executable are not available." -ForegroundColor Red
            exit 1
        }
    }
    "risk" {
        if ($Id -eq "") {
            Write-Host "Usage: .\scripts\inspect.ps1 risk -Id <run_id>"
            exit 1
        }
        Write-Host "=== Risk Results for $Id ===" -ForegroundColor Cyan
        Run-Query "SELECT trade_id, risk_measure, risk_factor_id, value FROM risk_results WHERE run_id = '$Id';"
    }
    "summary" {
        Write-Host "=== Platform Data Summary ===" -ForegroundColor Green
        if (Get-Command sqlite3 -ErrorAction SilentlyContinue) {
            Write-Host "Portfolios: " -NoNewline; sqlite3 "$DbFile" "SELECT count(*) FROM portfolios;"
            Write-Host "Trades: " -NoNewline; sqlite3 "$DbFile" "SELECT count(*) FROM trades;"
            Write-Host "Snapshots: " -NoNewline; sqlite3 "$DbFile" "SELECT count(*) FROM market_snapshots;"
            Write-Host "Runs: " -NoNewline; sqlite3 "$DbFile" "SELECT count(*) FROM analysis_runs;"
        } else {
             if ($null -ne $CliExe) { & $CliExe list }
        }
        Write-Host "`nUsage: .\scripts\inspect.ps1 [portfolios | trades | runs | results | risk] [-Id <id>] [-DbFile <path>]"
    }
    default {
        Write-Host "Unknown option: $Table"
        Write-Host "Usage: .\scripts\inspect.ps1 [portfolios | trades | runs | results | risk] [-Id <id>] [-DbFile <path>]"
    }
}
