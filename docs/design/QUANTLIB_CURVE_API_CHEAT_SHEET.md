# QuantLib Curve API Cheat Sheet

This appendix maps common curve calculations to QuantLib-style calls.

## 1. Read a discount factor

```cpp
DiscountFactor df = curve->discount(date);
```

Use when you need present values, forward discount ratios, carry, or deterministic cash-flow discounting.

## 2. Read a zero rate

```cpp
Rate z = curve->zeroRate(date, Actual365Fixed(), Continuous).rate();
```

Use for reporting, curve diagnostics, and macro discussion.

## 3. Read a forward rate between two dates

```cpp
Rate f = curve->forwardRate(d1, d2, Actual365Fixed(), Continuous).rate();
```

Use for FRA logic, desk reporting, and forward-looking interpretation.

## 4. Compute a forward discount factor

```cpp
DiscountFactor p12 = curve->discount(d2) / curve->discount(d1);
```

Use for shifted-horizon valuation or to explain $P(T_1,T_2)$ directly.

## 5. Build a shifted curve

```cpp
Handle<YieldTermStructure> base(curve);
auto implied = ext::make_shared<ImpliedTermStructure>(base, d1);
```

Use when you need a reusable curve object with reference date $d_1$.

## 6. Build a flat curve for tests

```cpp
auto flat = ext::make_shared<FlatForward>(today, 0.04, Actual365Fixed(), Continuous);
```

Use in unit tests, examples, and controlled scenario analysis.

## 7. Core bootstrapping helpers

- `DepositRateHelper`
- `FraRateHelper`
- `FuturesRateHelper`
- `OISRateHelper`
- `SwapRateHelper`

## 8. Common overnight indexes

- `Sofr`
- `Estr`
- `Saron`

## 9. Practical summary

> Helper instruments encode market conventions, bootstrap a curve object once, and then support valuation and risk queries through `discount`, `zeroRate`, `forwardRate`, or an implied term structure when the analytics need a shifted anchor date.
