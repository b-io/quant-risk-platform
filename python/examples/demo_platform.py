import sys
import os
import glob

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
