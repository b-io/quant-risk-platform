# visualize.ps1 - Visualize data from the database

$DbFile = "var\quant_risk_platform.sqlite"
$BuildDir = "build\Debug"
$CliExe = "$BuildDir\qrp_cli.exe"

if (!(Test-Path $DbFile)) {
    Write-Host "Database file $DbFile not found." -ForegroundColor Red
    exit 1
}

$Table = if ($args.Count -gt 0) { $args[0] } else { "summary" }

function Run-Query($sql) {
    if (Get-Command sqlite3 -ErrorAction SilentlyContinue) {
        sqlite3 -header -column "$DbFile" "$sql"
    } else {
        Write-Host "sqlite3 not found. For better visualization, install sqlite3." -ForegroundColor Yellow
        Write-Host "Falling back to qrp_cli list for basic info..." -ForegroundColor Cyan
        & $CliExe list
    }
}

switch ($Table) {
    "portfolios" {
        Write-Host "=== Portfolios ===" -ForegroundColor Cyan
        Run-Query "SELECT portfolio_id, portfolio_name, base_currency FROM portfolios;"
    }
    "trades" {
        $PortfolioId = if ($args.Count -gt 1) { $args[1] } else { "" }
        if ($PortfolioId -eq "") {
            Write-Host "Usage: .\scripts\visualize.ps1 trades <portfolio_id>"
            exit 1
        }
        Write-Host "=== Trades for $PortfolioId ===" -ForegroundColor Cyan
        Run-Query "SELECT trade_id, book_id, asset_class, product_type, currency, notional FROM trades WHERE portfolio_id = '$PortfolioId';"
    }
    "runs" {
        Write-Host "=== Analysis Runs ===" -ForegroundColor Cyan
        Run-Query "SELECT run_id, run_type, portfolio_id, snapshot_id, created_at FROM analysis_runs ORDER BY created_at DESC;"
    }
    "results" {
        $RunId = if ($args.Count -gt 1) { $args[1] } else { "" }
        if ($RunId -eq "") {
            Write-Host "Usage: .\scripts\visualize.ps1 results <run_id>"
            exit 1
        }
        Write-Host "Using qrp_cli report for $RunId ..." -ForegroundColor Cyan
        & $CliExe report $RunId
    }
    "summary" {
        Write-Host "=== Platform Data Summary ===" -ForegroundColor Green
        if (Get-Command sqlite3 -ErrorAction SilentlyContinue) {
            Write-Host "Portfolios: " -NoNewline; sqlite3 "$DbFile" "SELECT count(*) FROM portfolios;"
            Write-Host "Trades: " -NoNewline; sqlite3 "$DbFile" "SELECT count(*) FROM trades;"
            Write-Host "Snapshots: " -NoNewline; sqlite3 "$DbFile" "SELECT count(*) FROM market_snapshots;"
            Write-Host "Runs: " -NoNewline; sqlite3 "$DbFile" "SELECT count(*) FROM analysis_runs;"
        } else {
             & $CliExe list
        }
        Write-Host "`nUsage: .\scripts\visualize.ps1 [portfolios | trades <id> | runs | results <id>]"
    }
    default {
        Write-Host "Unknown option: $Table"
        Write-Host "Usage: .\scripts\visualize.ps1 [portfolios | trades <id> | runs | results <id>]"
    }
}
