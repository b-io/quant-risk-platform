# Macro Indicators and Index Construction

## Notation used in this chapter

Unless stated otherwise:

- $I_t$ is an index level at time $t$.
- $p_{i,t}$ is the price of item $i$ at time $t$.
- $q_{i,0}$ is the base-period quantity or expenditure weight for item $i$.
- $CPI_t$, $PCE_t$, and $PPI_t$ are the relevant price index levels at time $t$.
- $\pi$ is inflation and $\pi^*$ is the inflation target.
- $GDP$ is gross domestic product.
- $C$, $I$, $G$, $X$, and $M$ are consumption, investment, government spending, exports, and imports respectively.
- $U3$ is the headline unemployment rate.
- $\hat{p}$ denotes an estimated proportion when a sample share is used.

## What a trader or quant is really trying to infer

A macro desk is rarely interested in a data point for its own sake. The goal is to infer:

$$
\text{growth} ,\quad \text{inflation} ,\quad \text{labor tightness} ,\quad \text{financial conditions} ,\quad \text{policy reaction}
$$

The practical chain is:

$$
\text{data surprise} \rightarrow \text{updated macro view} \rightarrow \text{updated central-bank path} \rightarrow \text{repricing of rates, FX, credit, equities}
$$

## First distinction: not every indicator is an index

There are three broad types of macro indicators:

1. **Price indexes**
   - CPI
   - Core CPI
   - PCE price index
   - Core PCE
   - PPI
   - import/export price indexes
   - wage cost indexes such as ECI

2. **Activity / quantity measures**
   - GDP
   - retail sales
   - industrial production
   - housing starts
   - payroll employment
   - hours worked

3. **Rates / ratios / balances / surveys**
   - unemployment rate
   - labor force participation rate
   - vacancy rate / job openings
   - PMI / ISM diffusion indexes
   - consumer confidence
   - credit spreads
   - lending standards

This matters because unemployment is not a price index and GDP is not a fixed-weight basket. They are different statistical objects.

## How a price index is constructed in general

At the most intuitive level, a price index compares the cost of a basket today with the cost of a reference basket.

A simple fixed-basket index looks like:

$$
I_t = \frac{\sum_i p_{i,t} q_{i,0}}{\sum_i p_{i,0} q_{i,0}} \times 100
$$

where:
- $p_{i,t}$ is the price of item $i$ at time $t$
- $q_{i,0}$ is the base-period quantity or expenditure weight

This is the core fixed-basket intuition behind consumer inflation measurement.

#### Example — fixed-basket price index with concrete weights

Take a tiny basket with three items:
- bread: $q_{0}=10$, $p_{0}=2.0$, $p_{t}=2.2$
- rent: $q_{0}=1$, $p_{0}=1000$, $p_{t}=1040$
- fuel: $q_{0}=50$, $p_{0}=1.5$, $p_{t}=1.8$

Base basket value:

$$
\sum_i p_{i,0} q_{i,0} = 10\cdot 2.0 + 1\cdot 1000 + 50\cdot 1.5 = 1095
$$

Where:
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Current basket value:

$$
\sum_i p_{i,t} q_{i,0} = 10\cdot 2.2 + 1\cdot 1040 + 50\cdot 1.8 = 1152
$$

Where:
- $t$ is a time variable, or the real argument of a moment generating function depending on context.

So the fixed-basket index is:

$$
I_t = \frac{1152}{1095}\times 100 \approx 105.21
$$

Meaning:
- the basket is about 5.21% more expensive than in the base period,
- and the weight of rent dominates the result because its expenditure share is large.

In practice, agencies sample:
- which items to price
- where to collect prices
- how to update weights
- how to adjust for quality changes
- how to seasonally adjust some series

## Fixed basket vs. chain weighting

### Fixed basket

A fixed-basket index keeps the basket weights constant for a period. It is easier to interpret because you know what basket is being repriced.

Good for:
- pure price-change measurement
- stable interpretation

Weakness:
- consumers and firms substitute when relative prices move
- a stale basket can become less representative over time

### Chain weighting

Chain weighting updates expenditure patterns more frequently and links adjacent periods together.

BEA explicitly states that PCE price indexes are aggregated using a **Fisher chain-weighted formula**, and BEA uses chain-type indexes/chained dollars for real GDP to remove the effects of price changes from output measures. This is why PCE and real GDP are generally more substitution-aware than a pure fixed-basket measure. 

## Inflation indicators

## 1. CPI

The U.S. BLS defines CPI as the average change over time in the prices paid by consumers for a representative basket of goods and services. It is built for target populations such as CPI-U and CPI-W. Relative importance weights reflect expenditure or value weights, i.e. how consumers distribute spending across components. 

### What traders mean by CPI

Usually they look at:
- headline CPI month-over-month
- headline CPI year-over-year
- core CPI month-over-month
- core CPI year-over-year

### Headline CPI

Includes everything.

Most useful for:
- cost-of-living inflation
- public and political inflation perception
- first reaction when energy or food shocks hit

Weakness:
- volatile
- can be heavily driven by food and energy

### Core CPI

Excludes food and energy.

Used because it is often a better signal of underlying persistence.

### Why CPI matters to markets

If CPI comes in above expectations, markets usually infer:

$$
\text{higher inflation persistence} \rightarrow \text{higher expected policy path} \rightarrow \text{front-end yields up}
$$

But interpretation depends on the composition:
- goods vs. services
- shelter vs. non-shelter
- one-off vs. broad-based

### How weights work in CPI

The BLS says relative importance is based on expenditure/value weights representing average annual expenditures, and those ratios estimate how the index population distributes expenditures among components. In practice, this means shelter, transport, food, medical care, and so on do not contribute equally; large household spending categories get larger weights. 

Summary:

> CPI is a weighted consumer basket. A 1% move in a heavily weighted component matters much more for headline inflation than the same move in a tiny component.

#### Example — CPI basket weighting with practical numbers

If last year CPI was 102.0 and this year CPI is 105.21, then year-on-year CPI inflation is:

$$
\pi_{yoy} = \frac{105.21 - 102.0}{102.0} \approx 3.15\%
$$

Where:
- $\pi$ denotes inflation.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Meaning:
- consumer prices are 3.15% higher than a year ago,
- but a macro desk would still ask whether the move came from energy, shelter, or broader underlying inflation.

## 2. PCE price index

The BEA defines the PCE price index as a measure of the prices paid for goods and services purchased by consumers, or on their behalf, and notes that it reflects changes in consumer behavior. BEA also states that detailed PCE price indexes are aggregated using a **Fisher chain-weighted formula**, with many underlying detailed prices derived from CPI and PPI components. 

### Why central banks often care a lot about PCE

Because PCE:
- has broader coverage than CPI
- captures substitution better through chain weighting
- includes some expenditures made on behalf of households

### Core PCE

This is simply PCE excluding food and energy.

Summary:

> Core PCE is often treated as the cleaner medium-term inflation signal because it is broader than CPI and more adaptive in weights.

## 3. PPI

The BLS defines PPI as the average change over time in the selling prices received by domestic producers for their output. The FD-ID system organizes PPI into **final demand** and **intermediate demand** and covers goods, services, construction, exports, and government purchases. Final demand means sales to personal consumption, capital investment, government, and exports; intermediate demand covers business purchases as inputs to production excluding capital investment. 

### What PPI is useful for

- upstream cost pressure
- margin pressure on firms
- early warning for pipeline inflation
- sector-specific pricing power

### Interpretation

If PPI rises sharply:
- producers may face higher input costs
- firms may pass through some of it to consumers later
- or margins may compress instead

So:

$$
\text{PPI up} \not\Rightarrow \text{CPI up one-for-one}
$$

Summary:

> PPI is not consumer inflation; it is producer-side pricing. It helps you see where inflation pressure is entering the production chain.

## 4. Inflation expectations

These are not official price indexes but are critical market indicators.

Main types:
- survey expectations
- inflation breakevens from nominal vs. inflation-linked bonds
- inflation swaps

Why they matter:
- persistent inflation becomes more dangerous if expectations become unanchored
- long-end rates can reflect this even when spot CPI improves

## 5. Wage / labor-cost inflation

### Average Hourly Earnings (AHE)

From the establishment survey. Useful but composition-sensitive: if lower-paid workers leave the sample mix, average wages can rise mechanically.

### Employment Cost Index (ECI)

The BLS says ECI measures the change in hourly labor cost to employers and uses a **fixed basket of labor**, designed to produce a pure cost change free from workers moving between occupations and industries; it includes wages/salaries and benefits. 

Why markets like ECI:
- cleaner wage-cost signal than AHE
- good for judging services inflation persistence

Summary:

> AHE is fast but composition-sensitive; ECI is slower but methodologically cleaner because it uses a fixed basket of labor.

## Labor-market indicators

The labor market has two main official U.S. survey sources each month:
- the **household survey (CPS)**
- the **establishment survey (CES)**

BLS states that the establishment survey provides employment, hours, and earnings on nonfarm payrolls, while the household survey is used for labor-force status and unemployment. 

## 1. Unemployment rate (U-3)

BLS defines U-3 as:

$$
U3 = \frac{\text{Total Unemployed}}{\text{Labor Force}} \times 100
$$

This is the official unemployment rate. 

### Why it matters

- low unemployment usually means strong labor demand
- very low unemployment can signal a tight labor market
- but unemployment alone can miss hidden weakness

## 2. Broader underutilization: U-4, U-5, U-6

BLS defines:
- **U-4** = adds discouraged workers
- **U-5** = adds all marginally attached workers
- **U-6** = adds marginally attached workers and people working part time for economic reasons 

U-6 is often useful when the labor market looks tight by U-3 but still has slack.

Summary:

> U-3 is the headline rate; U-6 is the broader slack measure.

## 3. Labor force participation rate (LFPR)

This is a ratio, not an index:

$$
LFPR = \frac{\text{Labor Force}}{\text{Civilian Noninstitutional Population}} \times 100
$$

Why it matters:
- falling unemployment with falling participation can be less impressive than it looks
- rising participation can increase labor supply and reduce wage pressure

## 4. Nonfarm payrolls

From the establishment survey. BLS says CES collects monthly payroll records from about 119,000 businesses and government agencies, covering roughly 622,000 worksites. 

Why payrolls matter:
- high-frequency signal of labor demand and growth momentum
- often one of the most market-moving releases

## 5. Hours worked

Useful because firms often reduce hours before cutting headcount.

So hours can be an early cyclical signal.

## 6. JOLTS: vacancies, hires, quits, layoffs

BLS states that JOLTS publishes job openings, hires, quits, layoffs and discharges, other separations, and total separations. 

What each tells you:
- **job openings**: labor demand
- **hires**: realized hiring flow
- **quits**: worker confidence / bargaining power
- **layoffs**: labor-market deterioration

A common macro signal is the openings-to-unemployed ratio.

## Growth and demand indicators

## 1. GDP

GDP is the broadest measure of economic activity. BEA uses chain-type indexes/chained dollars for real GDP, and real measures remove the effects of price changes. 

The expenditure identity is:

$$
GDP = C + I + G + (X-M)
$$

Where:
- $GDP$ is gross domestic product.
- $C$ is household consumption in the GDP identity.
- $I$ is investment in the GDP identity.
- $G$ is government spending in the GDP identity.
- $X$ is exports in the GDP identity or a generic random variable depending on context.
- $M$ is imports in the GDP identity.

#### Example — CPI basket weighting with practical numbers

Suppose:
- consumption $C=700$
- investment $I=150$
- government spending $G=200$
- exports $X=120$
- imports $M=170$

Then:

$$
GDP = 700 + 150 + 200 + (120 - 170) = 1000
$$

If the previous year GDP was 970, then growth is:

$$
g = \frac{1000 - 970}{970} \approx 3.09\%
$$

Meaning:
- GDP tells you total activity,
- growth tells you how fast that activity is changing.

where:
- $X-M$ = net exports

### Nominal vs. real GDP

- **Nominal GDP** includes price changes
- **Real GDP** removes price changes

Very roughly:

$$
\text{Real GDP growth} \approx \text{Nominal GDP growth} - \text{inflation}
$$

### Why GDP matters

- broad growth signal
- benchmark for whether the economy is overheating or slowing
- input into central-bank assessment of slack / output gap

### Why GDP is not enough on its own

- lagged
- revised
- quarterly in many countries

So macro traders supplement it with monthly and survey data.

## 2. Retail sales / consumption indicators

These are activity measures, not price indexes.

Why they matter:
- consumption is often the largest part of GDP
- weak retail sales can be an early sign of demand slowing
- strong retail sales can offset weak manufacturing

## 3. Industrial production

Captures output from factories, mines, and utilities.

Why it matters:
- cyclical
- often sensitive to inventory swings and external demand
- useful for identifying manufacturing slowdowns

## 4. Housing indicators

Examples:
- housing starts
- building permits
- mortgage applications
- home sales

Why they matter:
- housing is one of the most rate-sensitive parts of the economy
- often turns before broader GDP

## 5. PMIs / ISM

These are **diffusion indexes**, not price indexes.

A diffusion index is constructed from survey responses such as:
- better
- same
- worse

The level often centers around 50:
- above 50 = expansion
- below 50 = contraction

Why they matter:
- timely
- forward-looking
- often move rates because they affect growth expectations before GDP is released

#### Example — GDP and PMI side-by-side interpretation

Suppose 100 firms answer a PMI-style survey:
- 40 say activity is better,
- 35 say activity is unchanged,
- 25 say activity is worse.

A standard diffusion-style construction is:

$$
PMI = \%(\text{better}) + 0.5\cdot \%(\text{same})
$$

Where:
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

So:

$$
PMI = 40 + 0.5\cdot 35 = 57.5
$$

Meaning:
- above 50 suggests expansion,
- below 50 suggests contraction,
- PMI is fast and directional, whereas GDP is broader but slower and revised later.

## 6. Financial conditions indicators

Examples:
- sovereign yields
- OIS curve
- credit spreads
- mortgage spreads
- equity levels
- volatility
- bank lending standards
- repo / funding spreads

These are not all official indexes, but they are essential because they determine how monetary policy reaches the real economy.

## How all these indicators fit together

A clean mental map is:

$$
\text{inflation indicators} \rightarrow \text{how sticky is inflation?}
$$

$$
\text{labor indicators} \rightarrow \text{how tight is the labor market?}
$$

$$
\text{growth indicators} \rightarrow \text{how strong is demand?}
$$

$$
\text{financial indicators} \rightarrow \text{how restrictive are conditions already?}
$$

Then the central-bank question becomes:

$$
\text{Do we need tighter, unchanged, or easier policy to bring inflation back to target without causing unnecessary instability?}
$$

## Practical summary

> A useful taxonomy groups macro indicators into price indexes, activity measures, and labor or financial ratios and surveys. CPI and PCE describe consumer inflation, with PCE more chain-weighted and substitution-aware. PPI captures producer-side pricing and pipeline pressure. In labor, household-survey metrics such as unemployment and participation should be separated from establishment-survey metrics such as payrolls, hours, and earnings, while JOLTS is useful for vacancies and turnover. For growth, GDP is the broad aggregate, but because it is lagged it helps to monitor retail sales, PMIs, industrial production, housing, and financial conditions as well. The key point for markets is not one indicator in isolation but how the full dashboard changes the expected central-bank path.

## Quick contrasts

### CPI vs. PCE
- CPI: consumer basket, fixed-basket intuition, very market-moving
- PCE: broader, chain-weighted, captures substitution better

### CPI vs. PPI
- CPI: what consumers pay
- PPI: what producers receive / face upstream

### AHE vs. ECI
- AHE: fast, composition-sensitive
- ECI: cleaner labor-cost signal, fixed basket of labor

### Payrolls vs. unemployment
- payrolls: jobs added / labor demand from establishments
- unemployment: household labor-force status

### GDP vs. PMI
- GDP: broad but lagged
- PMI: narrow but timely and forward-looking
