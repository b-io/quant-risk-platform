import sys
import os

# Add build directory to path to find the compiled module
# Use QRP_PYTHON_PATH if set, otherwise fallback to default build directory
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(os.path.dirname(script_dir))

build_path = os.environ.get("QRP_PYTHON_PATH")
if not build_path:
    # Default to build/Debug
    build_path = os.path.join(project_root, "build", "Debug")

if os.path.exists(build_path):
    sys.path.append(build_path)
    # Check for the actual .pyd file to give better diagnostics
    import glob

    pyd_files = glob.glob(os.path.join(build_path, "quant_risk_platform*.pyd"))
    if not pyd_files:
        print(f"DEBUG: No quant_risk_platform*.pyd found in {build_path}")

    # Also add vcpkg tool path if it exists to find dependencies
    vcpkg_python_path = os.path.join(build_path, "vcpkg_installed", "x64-windows", "tools", "python3")
    if os.path.exists(vcpkg_python_path):
        os.environ["PATH"] = vcpkg_python_path + os.pathsep + os.environ["PATH"]

try:
    import quant_risk_platform as qrp
except ImportError as e:
    print(f"Could not import quant_risk_platform: {e}")
    print(f"Looking in: {build_path}")
    print(f"Python version: {sys.version}")
    print(f"Make sure it's built in the 'build' directory and matches your Python version.")
    sys.exit(1)


def run_demo():
    print("--- Quant Risk Platform Python Demo ---")

    # Load market and portfolio
    market_path = os.path.join(project_root, "data", "market", "base_market_v2.json")
    portfolio_path = os.path.join(project_root, "data", "portfolios", "demo_macro_book.json")

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
    risk_results = qrp.compute_risk(portfolio, market_dto)
    for res in risk_results:
        print(f"  Trade: {res.trade_id}, PV01: {res.pv01:,.2f}")
        for tenor, risk in res.bucketed_risk.items():
            if abs(risk) > 1e-4:
                print(f"    {tenor}: {risk:,.2f}")

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
    print("\nMonte Carlo Simulation Demo:")
    sim_result = qrp.run_simulation(portfolio, market_dto, 100)
    print(f"  Simulation completed with {len(sim_result.portfolio_values)} paths.")
    print(f"  95% VaR: {sim_result.var_95:,.2f}")
    print(f"  95% ES : {sim_result.expected_shortfall_95:,.2f}")


if __name__ == "__main__":
    run_demo()
