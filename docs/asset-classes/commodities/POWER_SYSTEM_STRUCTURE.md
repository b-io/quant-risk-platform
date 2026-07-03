# Power System Structure

## 1. Physical Structure And Price Formation

Electricity is a financial commodity with a strong physical constraint: production and consumption must balance at high
frequency. This makes power prices sensitive to generation mix, demand shape, weather, outages, transmission constraints,
and storage or flexibility assets.

For pricing and risk, the physical system matters because it explains why a forward curve can have:

- strong seasonality;
- peak/off-peak spreads;
- scarcity spikes;
- negative prices;
- regime shifts after fuel, carbon, weather, or outage shocks;
- location spreads between bidding zones.

## 2. Main Generation Types

Typical power systems combine several production technologies:

| Technology | Economic role | Quant relevance |
|---|---|---|
| Hydro reservoir | Flexible energy storage | Optionality, seasonal water value, dispatch timing |
| Run-of-river hydro | Weather-driven generation | Hydrology risk and seasonal supply shape |
| Nuclear | Baseload supply | Outage and maintenance risk |
| Gas-fired thermal | Marginal and flexible generation | Clean spark spread, gas and carbon linkage |
| Coal or lignite | Thermal generation where present | Fuel and carbon exposure |
| Wind | Intermittent renewable supply | Weather-driven volume and price-shape risk |
| Solar | Intermittent renewable supply | Midday price depression and intraday shape |
| Batteries | Short-duration storage | Intraday optionality and spread capture |

The precise mix is regional. The platform should therefore represent system drivers as configurable factors rather than
embedded country assumptions.

## 3. Demand, Weather, and Calendar Shape

Power demand is shaped by:

- hour of day;
- weekday, weekend, and holiday effects;
- heating and cooling demand;
- industrial load;
- weather forecast revisions;
- daylight-saving and delivery-calendar conventions.

This is why delivery-period products are not simple maturities. A monthly baseload forward is an average over many
delivery hours:

$$
F(t,T_1,T_2)
=
\frac{\sum_{h \in [T_1,T_2]} w_h f_h}
{\sum_{h \in [T_1,T_2]} w_h}.
$$

The implementation should keep the delivery calendar, load profile, and aggregation weights explicit.

## 4. Interconnection and Location Spreads

Bidding zones are connected by transmission capacity. Imports and exports can reduce scarcity in one zone and transmit
price shocks from another zone, but constraints prevent full convergence.

A generic location-spread factor can be written as:

$$
S^{A/B}_t = P^A_t - P^B_t.
$$

The same notation applies to country borders, bidding zones, hubs, or internal market areas. Documentation should avoid
assuming that one location is the default reference system.

## 5. Flexibility and Optionality

Flexible assets convert physical constraints into option value:

- reservoirs decide whether to generate now or later;
- batteries decide when to charge and discharge;
- gas plants decide whether the clean spark spread covers variable costs;
- storage assets decide when to inject or withdraw;
- swing contracts decide how much volume to nominate.

These decisions make many energy products dynamic-programming or stochastic-control problems rather than simple curve
discounting problems.

## 6. Platform Implications

The market model should represent:

- quoted delivery products;
- granular curve buckets;
- load profiles and delivery calendars;
- fuel, carbon, weather, outage, and location-spread factors;
- flexible-asset state variables;
- scenario overlays with traceable factor shocks.

The same architecture can then support multiple regions. Region-specific notes, figures, or preparation material belong
under the ignored `temp/docs/` area unless they are converted into reusable project documentation.
