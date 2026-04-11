import glob
import math
import os
import sys

# Add build directory to path to find the compiled module
# Use QRP_PYTHON_PATH if set, otherwise fallback to default build directory
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(os.path.dirname(script_dir))

build_path = os.environ.get("QRP_PYTHON_PATH")
if not build_path:
    # Try multiple common build locations, preferring Release-Python
    candidates = [
        os.path.join(project_root, "build", "Release-Python", "python", "Release"),
    ]
    for candidate in candidates:
        if os.path.exists(candidate) and glob.glob(os.path.join(candidate, "quant_risk_platform*.pyd")):
            build_path = candidate
            break
    
    # Final fallback if none found
    if not build_path:
        build_path = os.path.join(project_root, "build", "Release-Python", "python", "Release")

if os.path.exists(build_path):
    sys.path.append(build_path)
    
    # On Windows, we need to explicitly add DLL search paths for dependencies
    # starting from Python 3.8.
    if sys.platform == 'win32' and hasattr(os, 'add_dll_directory'):
        # Add the directory containing the .pyd itself
        os.add_dll_directory(build_path)
        
        # Add the vcpkg bin directory where core dependencies reside
        # For multi-config generators, it might be in build_root/vcpkg_installed/x64-windows/bin
        # We need to find the build_root first
        build_root = build_path
        while build_root != project_root:
            vcpkg_bin_path = os.path.join(build_root, "vcpkg_installed", "x64-windows", "bin")
            if os.path.exists(vcpkg_bin_path):
                os.add_dll_directory(vcpkg_bin_path)
                break
            parent = os.path.dirname(build_root)
            if parent == build_root: break
            build_root = parent
            
        # Also check for tools path just in case
        vcpkg_tool_path = os.path.join(build_root, "vcpkg_installed", "x64-windows", "tools", "python3")
        if os.path.exists(vcpkg_tool_path):
            os.add_dll_directory(vcpkg_tool_path)
            
    # For older Python or non-Windows, we also update PATH
    # Find build_root again for non-Windows or older Python
    build_root = build_path
    while build_root != project_root:
        vcpkg_bin_path = os.path.join(build_root, "vcpkg_installed", "x64-windows", "bin")
        if os.path.exists(vcpkg_bin_path):
            os.environ["PATH"] = vcpkg_bin_path + os.pathsep + os.environ["PATH"]
            break
        parent = os.path.dirname(build_root)
        if parent == build_root: break
        build_root = parent

try:
    import quant_risk_platform as qrp
except ImportError as e:
    print(f"Error: Could not import 'quant_risk_platform' module.")
    print(f"Details: {e}")
    print(f"Search Path (sys.path): {sys.path}")
    print(f"Python Executable: {sys.executable}")
    print(f"Python Version: {sys.version}")
    
    # Check for the actual .pyd file
    import glob
    pyd_files = glob.glob(os.path.join(build_path, "quant_risk_platform*.pyd"))
    if not pyd_files:
        print(f"Diagnostic: No .pyd files found in {build_path}. Did you build the project?")
    else:
        print(f"Diagnostic: Found .pyd files: {[os.path.basename(f) for f in pyd_files]}")
        print(f"Likely cause: Missing DLL dependencies in PATH or incompatible Python version.")
    
    sys.exit(1)


def print_portfolio_summary(portfolio, base_market_dto):
    valuation_results = qrp.price_portfolio(portfolio, base_market_dto)
    total_npv = sum(res.npv for res in valuation_results)
    print(f"Portfolio: {portfolio.portfolio_id}")
    print(f"Trades: {len(portfolio.trades)}")
    print(f"Base portfolio value: {total_npv:,.2f} USD")
    return total_npv

def print_factor_summary(factors, covariance, config):
    print(f"\nFactors used ({len(factors)}):")
    for i, f in enumerate(factors):
        print(f"  {i+1}. {f.factor_id}")
    
    print(f"\nCovariance matrix:")
    print(f"  rows = {covariance.rows()}, cols = {covariance.columns()}")
    print(f"  horizon scaling = {'already scaled' if config.covariance_is_already_horizon_scaled else f'from 1d to {config.horizon_days}d'}")
    print(f"  seed = {config.seed}")

def print_traces(traces, mode):
    for trace in traces:
        print(f"\nPath {trace.path_index + 1}:")
        if mode == qrp.MonteCarloMode.AgedHorizonRevaluation:
            print(f"  Valuation date:           {trace.valuation_date_before} -> {trace.valuation_date_after}")
            print(f"  Base value at t0:         {trace.portfolio_value_before:,.2f}")
            print(f"  Frozen-aged value at tH:  {trace.portfolio_value_frozen_aged:,.2f}")
            print(f"  Shocked-aged value at tH: {trace.portfolio_value_after:,.2f}")
            print("")
            print(f"  Aging P&L:                {trace.aging_pnl:,.2f}")
            print(f"  Horizon market P&L:       {trace.market_pnl:,.2f}")
            print(f"  Total horizon P&L:        {trace.total_pnl:,.2f}")
        else:
            print(f"  Portfolio value: {trace.portfolio_value_before:,.2f} -> {trace.portfolio_value_after:,.2f}")
            print(f"  Path P&L: {trace.path_pnl:,.2f}")

        print("\n  Factor shocks:")
        for fid, shock in trace.factor_shocks.items():
            print(f"    {fid}: {shock:+.6f}")
        
        print("\n  Quote changes (selected):")
        for qid, (before, after) in trace.quote_before_after.items():
            print(f"    {qid}: {before:.6f} -> {after:.6f}")

def run_mc_case(portfolio, market_dto, factors, bindings, cov, mode_name, horizon_days, num_paths, seed):
    print(f"\n" + "="*50)
    print(f"RUNNING MC CASE: Mode={mode_name}, Horizon={horizon_days}d")
    print("="*50)
    
    config = qrp.MonteCarloConfig()
    config.num_paths = num_paths
    config.seed = seed
    config.horizon_days = horizon_days
    if mode_name == "HorizonShockOnly":
        config.mode = qrp.MonteCarloMode.HorizonShockOnly
    elif mode_name == "AgedHorizonRevaluation":
        config.mode = qrp.MonteCarloMode.AgedHorizonRevaluation
        
    try:
        sim_result = qrp.run_simulation(portfolio, market_dto, factors, bindings, cov, config)
        print(f"Simulation completed with {len(sim_result.portfolio_values)} paths.")
        
        print("\n  Baseline Diagnostics:")
        print(f"    Total Trades: {sim_result.num_trades_total}")
        print(f"    Priced at t0: {sim_result.num_trades_priced_t0}")
        if sim_result.num_trades_failed_t0 > 0:
            print(f"    FAILED at t0: {sim_result.num_trades_failed_t0}")
            
        if mode_name == "AgedHorizonRevaluation":
            print(f"    Priced at tH: {sim_result.num_trades_priced_tH}")
            if sim_result.num_trades_expired_tH > 0:
                print(f"    Expired at tH: {sim_result.num_trades_expired_tH}")
            if sim_result.num_trades_failed_tH > 0:
                print(f"    FAILED at tH: {sim_result.num_trades_failed_tH}")
            if sim_result.num_trades_unsupported_tH > 0:
                print(f"    Unsupported at tH: {sim_result.num_trades_unsupported_tH}")

        if sim_result.construction_errors:
            print("\n  Construction Errors (Top 5):")
            for i, (tid, err) in enumerate(list(sim_result.construction_errors.items())[:5]):
                print(f"    {tid}: {err}")

        # Print traces for first few paths
        print_traces(sim_result.traces, config.mode)
        
        return sim_result
    except Exception as e:
        print(f"Simulation failed for {mode_name} {horizon_days}d: {e}")
        return None

def run_demo():
    # ... (existing setup code until Monte Carlo Demo)
    print("--- Quant Risk Platform Python Demo ---")

    # Load market and portfolio
    market_path = os.path.join(project_root, "data", "market", "demo_market.json")
    portfolio_path = os.path.join(project_root, "data", "portfolios", "demo_portfolio.json")

    print(f"Loading market from {market_path}")
    market_dto = qrp.load_market(market_path)
    print(f"Loading portfolio from {portfolio_path}")
    portfolio = qrp.load_portfolio(portfolio_path)

    print(f"Loaded portfolio: {portfolio.portfolio_id} with {len(portfolio.trades)} trades.")

    # Price Portfolio
    print("\nPricing Portfolio:")
    valuation_results = qrp.price_portfolio(portfolio, market_dto)
    for res in valuation_results:
        print(f"  Trade: {res.trade_id}, NPV: {res.npv:,.2f} {res.currency}")

    # Compute Risk
    print("\nComputing Risk (PV01 and Bucketed):")
    # Risk computation now requires factors and bindings
    factors = []
    bindings = []
    
    # Define some factors for USD OIS and LIBOR
    # 2Y OIS Node
    f1 = qrp.FactorDefinition()
    f1.factor_id = "USD_OIS_2Y"
    f1.factor_type = qrp.FactorType.RateZero
    f1.shock_measure = qrp.ShockMeasure.Absolute
    f1.currency = qrp.Currency.USD
    f1.tenor = "2Y"
    f1.quote_ids = ["USD_OIS_2Y"]
    factors.append(f1)
    
    # 5Y OIS Node
    f2 = qrp.FactorDefinition()
    f2.factor_id = "USD_OIS_5Y"
    f2.factor_type = qrp.FactorType.RateZero
    f2.shock_measure = qrp.ShockMeasure.Absolute
    f2.currency = qrp.Currency.USD
    f2.tenor = "5Y"
    f2.quote_ids = ["USD_OIS_5Y"]
    factors.append(f2)
    
    # 5Y LIBOR Node
    f3 = qrp.FactorDefinition()
    f3.factor_id = "USD_LIBOR_5Y"
    f3.factor_type = qrp.FactorType.RateZero
    f3.shock_measure = qrp.ShockMeasure.Absolute
    f3.currency = qrp.Currency.USD
    f3.tenor = "5Y"
    f3.quote_ids = ["USD_LIBOR_3M_IRS_5Y"]
    factors.append(f3)

    # Bindings link factors to quotes
    for f in factors:
        b = qrp.FactorBinding()
        b.factor_id = f.factor_id
        b.quote_id = f.quote_ids[0]
        b.shock_measure = qrp.ShockMeasure.Absolute
        b.weight = 1.0
        bindings.append(b)

    try:
        risk_results = qrp.compute_risk(portfolio, market_dto, factors, bindings)
        for res in risk_results:
            print(f"  Trade: {res.trade_id}, PV01: {res.pv01:,.2f}")
            # Note: compute_risk might still return bucketed risk by quote if implemented that way internally
            # but now it's driven by factors.
            for node, risk in res.bucketed_risk.items():
                if abs(risk) > 1e-4:
                    print(f"    {node}: {risk:,.2f}")
    except Exception as e:
        print(f"  Risk computation failed: {e}")

    # P&L Explain Demo
    print("\nP&L Explain Demo:")
    # Simulate a market move: 10bp up
    curr_market_dto = qrp.load_market(market_path)
    for quote in curr_market_dto.quotes:
        quote.value += 0.0010

    pnl_results = qrp.explain_pnl(portfolio, market_dto, curr_market_dto)
    for res in pnl_results:
        print(f"  Trade: {res.trade_id}, "
              f"Total P&L: {res.total_pnl:,.2f}, "
              f"Carry: {res.carry_pnl:,.2f}, "
              f"Market Move: {res.market_move_pnl:,.2f}")

    # Monte Carlo Demo
    print("\n" + "#"*60)
    print("# MONTE CARLO SIMULATION DEMO")
    print("#"*60)
    
    print_portfolio_summary(portfolio, market_dto)

    # Covariance for our 3 factors
    # Let's assume some correlations and volatilities (in absolute rate terms, e.g. 1bp = 0.0001)
    # Vol: 1bp/day -> 0.0001
    # Var: 1e-8
    n = len(factors)
    cov = qrp.Matrix(n, n)
    for i in range(n):
        for j in range(n):
            if i == j:
                cov[i, j] = 1e-8 # 1bp daily vol
            else:
                cov[i, j] = 0.5 * 1e-8 # 0.5 correlation
    
    cases = [
        ("HorizonShockOnly", 1.0),
        ("HorizonShockOnly", 10.0),
        ("AgedHorizonRevaluation", 10.0),
    ]
    
    results_summary = []
    
    for mode, horizon in cases:
        res = run_mc_case(portfolio, market_dto, factors, bindings, cov, mode, horizon, 500, 42)
        if res:
            mean_pnl = sum(res.portfolio_pnls) / len(res.portfolio_pnls)
            results_summary.append({
                "Mode": mode,
                "Horizon": f"{horizon}d",
                "VaR 95": f"{res.var_95:,.2f}",
                "ES 95": f"{res.expected_shortfall_95:,.2f}",
                "Mean P&L": f"{mean_pnl:,.2f}"
            })
        else:
             results_summary.append({
                "Mode": mode,
                "Horizon": f"{horizon}d",
                "VaR 95": "N/A",
                "ES 95": "N/A",
                "Mean P&L": "N/A"
            })

    # Print Summary Table
    print("\n" + "="*80)
    print("MONTE CARLO SUMMARY")
    print("="*80)
    print(f"{'Mode':<25} {'Horizon':<10} {'VaR 95':<15} {'ES 95':<15} {'Mean P&L':<15}")
    print("-" * 80)
    for r in results_summary:
        print(f"{r['Mode']:<25} {r['Horizon']:<10} {r['VaR 95']:<15} {r['ES 95']:<15} {r['Mean P&L']:<15}")
    print("="*80)


if __name__ == "__main__":
    run_demo()
