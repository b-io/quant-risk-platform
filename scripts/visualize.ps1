param(
    [string]$Table = "summary",
    [string]$Id = "",
    [string]$BuildDir = "build\Release-Python",
    [string]$Config = "Release"
)

# visualize.ps1 - Visualize data from the database

$DbFile = "var\quant_risk_platform.sqlite"

# Helper to find executable
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

if (!(Test-Path $DbFile)) {
    Write-Host "Database file $DbFile not found." -ForegroundColor Red
    exit 1
}

function Run-Query($sql) {
    if (Get-Command sqlite3 -ErrorAction SilentlyContinue) {
        sqlite3 -header -column "$DbFile" "$sql"
    } else {
        Write-Host "sqlite3 not found. For better visualization, install sqlite3." -ForegroundColor Yellow
        Write-Host "Falling back to qrp_cli list for basic info..." -ForegroundColor Cyan
        if ($null -ne $CliExe) {
            & $CliExe list
        } else {
            Write-Host "Error: qrp_cli.exe not found to fallback." -ForegroundColor Red
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
            Write-Host "Usage: .\scripts\visualize.ps1 trades -Id <portfolio_id>"
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
            Write-Host "Usage: .\scripts\visualize.ps1 results -Id <run_id>"
            exit 1
        }
        if (Get-Command sqlite3 -ErrorAction SilentlyContinue) {
            Write-Host "=== Valuation Results for $Id ===" -ForegroundColor Cyan
            Run-Query "SELECT trade_id, npv_base, valuation_ccy, status, error_message FROM valuation_results WHERE run_id = '$Id';"
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
            Write-Host "Error: sqlite3 and qrp_cli.exe are not available." -ForegroundColor Red
            exit 1
        }
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
        Write-Host "`nUsage: .\scripts\visualize.ps1 [portfolios | trades <id> | runs | results <id>]"
    }
    default {
        Write-Host "Unknown option: $Table"
        Write-Host "Usage: .\scripts\visualize.ps1 [portfolios | trades <id> | runs | results <id>]"
    }
}
