# C++ Style Guide

This project uses modern C++20 for quantitative analytics, market construction,
persistence, and Python bindings. Code should be fast enough for production risk
workflows and still easy for another engineer to read months later.

## Core Principles

- Prefer clarity first, then optimize the parts that profiling or domain knowledge
  shows are hot.
- Keep business concepts explicit: trades, factors, scenarios, curves, snapshots,
  and valuation results should be named in domain language.
- Make code locally readable. A reader should understand a file's purpose from the
  top comment and a public API's purpose from its declaration.
- Preserve consistency over personal style. Match nearby naming, ordering, and
  abstraction level before introducing a new pattern.
- Avoid clever code in financial calculations unless it is measured, documented,
  and covered by tests.

## File Organization

Use this order for headers:

1. `#pragma once`
2. One short file-purpose comment.
3. Includes.
4. Forward declarations, when they materially reduce dependencies.
5. `namespace qrp::...`.
6. Constants and enums.
7. DTO structs.
8. Public service classes.
9. Inline helpers.

Use this order for implementation files:

1. One short file-purpose comment.
2. The matching project header first.
3. Other project headers.
4. Third-party library headers.
5. Standard library headers.
6. Design note, when the file contains non-obvious architecture or performance
   tradeoffs.
7. `namespace qrp::...`.
8. Local helpers in an anonymous namespace.
9. Public function implementations in the same order as the header.

Sort includes alphabetically inside each group when there is no stronger reason.
Use business order when it helps comprehension, such as lifecycle order,
valuation-before-risk, or DTO-before-service.

## Naming

- Namespaces: `qrp::<domain>` in lowercase.
- Types, classes, and enums: `PascalCase`.
- Functions and variables: `snake_case`.
- Private data members: trailing underscore, for example `db_`.
- Constants: prefer descriptive `snake_case` or existing local convention.
- Enum values: `PascalCase`.
- JSON fields and persisted identifiers: keep canonical external strings stable.

Prefer names that describe meaning, not mechanics. For example, use
`base_market_dto`, `horizon_state`, or `factor_bindings` instead of `data`,
`state2`, or `vec`.

## Comments

Comments should explain intent, invariants, business rules, and performance
choices. Do not restate obvious code.

- Every `.hpp` and `.cpp` file should start with a short purpose comment.
- Public classes, structs, and non-trivial public functions should use Doxygen:

```cpp
/**
 * @brief Prices trades and portfolios while preserving product-support diagnostics.
 */
class ValuationService {
public:
    /**
     * @brief Builds and prices each supported trade in the portfolio.
     */
    static std::vector<ValuationResult> price_portfolio(...);
};
```

- Use `//` comments for local sections and non-obvious implementation details.
- Use a `Design note` block for architectural choices, numerical assumptions, or
  performance-sensitive strategies.
- Keep comments current with the code. A stale comment is worse than no comment.
- Comment financial assumptions explicitly: shock measure, unit, day-count,
  compounding, valuation date, aging policy, and fallback behavior.

## Formatting

- Indent with 4 spaces. Do not use tabs.
- Put opening braces on the same line for functions, classes, control flow, and
  namespaces.
- Keep lines readable; prefer breaking long expressions around function arguments
  or logical clauses.
- Use blank lines to separate conceptual blocks, not every statement.
- Keep namespace close comments, for example `} // namespace qrp::analytics`.
- Prefer early validation and clear guard clauses over deeply nested control flow.

## Types And Ownership

- Prefer values for small DTOs and cheap domain objects.
- Pass large objects by `const&`.
- Use `std::unique_ptr` for exclusive ownership.
- Use `std::shared_ptr` only when ownership is genuinely shared or required by
  QuantLib interfaces.
- Prefer `std::optional` over sentinel values when absence is meaningful.
- Avoid raw owning pointers. Raw pointers may be used only as non-owning views
  when the lifetime is obvious.

## Performance

- Reserve vector or map capacity when sizes are known.
- Keep hot loops allocation-light.
- Avoid rebuilding QuantLib instruments, curves, or handles inside path loops
  unless the model requires it.
- Prefer immutable inputs and mutable quote handles for scenario and Monte Carlo
  repricing.
- Validate dimensions before numerical routines such as covariance repair,
  Cholesky decomposition, regression, and optimization.
- Measure before replacing clear code with complex micro-optimizations.

## Error Handling

- Validate public inputs at API boundaries.
- Throw exceptions with domain context, such as trade id, factor id, curve id, or
  quote id.
- Preserve diagnostics in result DTOs when workflows should continue after a
  recoverable trade-level failure.
- Keep fatal market-data gates explicit. Blocking diagnostics should stop
  valuation, risk, and reporting workflows.

## Ordering

Default to alphanumerical ordering for lists of independent items. Use a
business or dependency order only when the order itself carries domain meaning,
or when registration/construction requires it, and make that reason clear from
section names or nearby code.

Use alphanumerical ordering for common neutral sets:

- Include files within a group.
- Enum values.
- Binding groups, `m.def(...)` entries, enum `.value(...)` entries, and
  independent DTO fields in Python bindings.
- Test helper declarations.
- Report and dashboard trade rows, unless the view is explicitly ranked by a
  metric. The default trade row key is `asset_class`, then `product_type`, then
  `trade_id`, all alphanumerical.

Use business ordering only when it reads better and has domain meaning:

- Market loading before market building.
- Valuation before risk, stress, Monte Carlo, and reporting.
- Base valuation before horizon valuation.
- Input validation before transformation and persistence.
- DTO fields only when the field order is part of a documented external format.

Metric-ranked charts are allowed when the title or section name makes the
ranking explicit, such as "Worst Stress", "Top NPV Contributors", or
"Contribution by Absolute P&L". Dense categorical charts should use a bounded
row-aware height or split into a dedicated detail view before labels overlap.
Dashboard chart titles should be visually bold. Dense x-axis labels should use
a consistent bottom-left to top-right diagonal angle when wrapping alone is not
enough. Trade-level charts should preserve the asset-class and product-type
hierarchy when it helps users scan the result. Dashboard tables should prefer
fixed responsive widths and soft wrapping over horizontal scrollbars.

## Tests

- Test names should describe behavior, not implementation.
- Keep fixtures focused on the domain concept under test.
- Cover both successful workflows and validation failures.
- Prefer deterministic seeds for simulation tests.
- Add regression tests for pricing, persistence, and market-data behavior before
  changing established numerical semantics.

## Git Commit Messages

Use Conventional Commits for project history. Keep messages concise, searchable,
and useful for release notes.

- Format the subject as `type(scope): summary` or `type: summary`.
- Use common types: `feat`, `fix`, `refactor`, `docs`, `test`, `build`, `ci`,
  `perf`, and `chore`.
- Use a short, lowercase scope when it clarifies ownership, for example
  `feat(rates)`, `refactor(build)`, or `test(persistence)`.
- Write the summary in the imperative mood and keep it under 72 characters.
- Add a blank line before the body. Use the body to explain what changed and why,
  not to repeat the diff.
- Wrap body lines at roughly 72 characters.
- Use footers for breaking changes and issue references, for example
  `BREAKING CHANGE: ...`.

## Python Bindings

- Keep binding files grouped by domain: domain, market, analytics, and IO.
- Bind enums and DTOs before services that consume them.
- Sort binding declarations and bound member lists alphanumerically unless a
  pybind registration dependency, such as a base class before derived classes,
  requires otherwise.
- Preserve canonical names exposed to Python unless a breaking API change is
  intentional and documented.

## Dependencies

- Prefer the standard library for general-purpose utilities.
- Use QuantLib for financial instruments, calendars, schedules, and term
  structures.
- Use Eigen for linear algebra where already present in optimization or
  regression code.
- Avoid introducing new dependencies unless they replace substantial complexity
  or provide a proven domain capability.

## Documentation Style

The public Markdown files should read like a quant-finance and implementation
handbook. They should explain theory, algorithms, architecture, model choices,
and implementation trade-offs in a reusable way.

- Keep `docs/` publishable and project-centered.
- Put private preparation notes, company-specific research, local context, and
  scratch examples under the ignored `temp/docs/` workspace.
- Avoid question-and-answer prose, personal positioning, application-oriented
  wording, or claims that only make sense for a private preparation context.
- Keep examples only when they teach a reusable method, convention, model, or
  implementation pattern.
- Make location-specific material generic unless the location is essential to a
  public market convention being documented.
- Prefer "pricing", "valuation", "risk", "control", "portfolio", and "market
  state" language over informal stakeholder slogans.

### Documentation Architecture

Keep the public `docs/` tree organized by orthogonal roles.

- `docs/foundations/` contains mathematical prerequisites.
- `docs/architecture/` contains platform design, service boundaries, storage,
  lineage, and implementation rationale.
- `docs/market-data/` contains market-state normalization, quote identity,
  conventions, and snapshot construction.
- `docs/asset-classes/` contains rates, credit, FX, equity, and commodity
  conventions and product families.
- `docs/models/` contains model families that are reused across asset classes,
  such as volatility models and exercise-policy methods.
- `docs/risk/` contains PnL explain, sensitivities, stress, VaR, Expected
  Shortfall, Monte Carlo, and attribution.
- `docs/reference/` is the single home for shared lexis, formula notation, and
  public source lists.
- `docs/implementation/` contains phased build plans, validation standards, and
  platform delivery sequencing.

Do not duplicate a shared glossary, formula sheet, or bibliography inside an
asset-class folder. Asset-class chapters may introduce local notation when
needed, but reusable definitions should be moved into `docs/reference/` and
linked from the chapter.

### Book And Research Prose

Use neutral, durable chapter language.

- Prefer noun-phrase headings such as "Power Curve Construction Constraints",
  "Model Selection Criteria", "Implementation Considerations", and
  "Limitations".
- Avoid conversational headings such as "why this is hard", "what users look
  at", "the right answer", "good enough", or "quick summary".
- Avoid evaluative adjectives when a precise criterion is available. Prefer
  "binding constraint", "low-dimensional approximation", "computationally
  inexpensive", "statistically unstable", or "model-incomplete" to vague labels
  such as "hard", "easy", "bad", "good", or "strong".
- State claims as reproducible technical statements. If a claim depends on a
  market convention, model assumption, or implementation context, name that
  context.
- Use "intuition" sparingly. Prefer "interpretation", "economic meaning",
  "model implication", or "implementation implication" when those are more
  precise.
- Do not write as if preparing an oral answer. Public documents should not
  contain answer templates, personal framing, or rhetorical contrasts.
- Each substantial chapter should make its structure explicit: definition,
  notation, assumptions, method, implementation mapping, validation, and
  limitations where applicable.

### Markdown Math

Use dollar-delimited LaTeX so formulas render in common Markdown environments.

- Inline formulas use single dollar signs: `$PV$`, `$D(t,T)$`,
  `$F(t;T_1,T_2)$`.
- Display formulas use double dollar signs on their own lines:

```markdown
$$
PV = \sum_i CF_i D(t,T_i)
$$
```

Every displayed formula should be followed or preceded by enough text to define
the symbols, units, convention, and implementation meaning.
