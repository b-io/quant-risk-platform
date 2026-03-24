import sys
import os

# Add build directory to path to find the compiled module
sys.path.append(os.path.abspath("build"))

try:
    import quant_risk_platform as qrp
except ImportError:
    print("Could not import quant_risk_platform. Make sure it's built in the 'build' directory.")
    sys.exit(1)

def run_demo():
    print("--- Quant Risk Platform Python Demo ---")
    
    # Load market and portfolio
    market_dto = qrp.load_market("data/market/base_market.json")
    portfolio = qrp.load_portfolio("data/portfolios/demo_macro_book.json")
    
    print(f"Loaded portfolio: {portfolio.portfolio_id} with {len(portfolio.trades)} trades.")
    
    # Create MarketObject (QuantLib-backed)
    market_obj = qrp.MarketObject(market_dto)
    
    # Price Portfolio
    print("\nPricing Portfolio:")
    valuation_results = qrp.price_portfolio(portfolio, market_obj)
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
    curr_market_dto = qrp.load_market("data/market/base_market.json")
    for AC, markets in curr_market_dto.markets.items():
        for name, curve in markets.items():
            for node in curve.nodes:
                node.value += 0.0010 
    
    pnl_results = qrp.explain_pnl(portfolio, market_dto, curr_market_dto)
    for res in pnl_results:
        print(f"  Trade: {res.trade_id}, Total P&L: {res.total_pnl:,.2f}, Market Move: {res.market_move_pnl:,.2f}")

if __name__ == "__main__":
    run_demo()
