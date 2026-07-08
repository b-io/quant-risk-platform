# Python, C++, Performance Engineering, and Bindings

## Chapter Motivation

A modern quant platform often uses both Python and C++. Python excels at orchestration, experimentation, and user-facing
analytics workflows. C++ excels at deterministic low-level performance and memory control. This chapter explains how the
two languages complement each other and what performance principles matter in production.

## 1. When Python is the right tool

Python is usually best for:

- orchestration of workflows,
- research notebooks and exploratory analysis,
- data cleaning and transformation,
- scripting and automation,
- user-facing APIs for quants,
- glue code across services.

Its ecosystem is a major advantage: NumPy, pandas, SciPy, statsmodels, and bindings to lower-level libraries make Python
a high-productivity language for quant workflows.

## 2. When C++ is the right tool

C++ is usually best for:

- performance-critical pricing kernels,
- large-scale revaluation loops,
- low-latency calculations,
- custom memory management,
- multithreaded numerical engines,
- deterministic integration with numerical libraries.

This does not mean every quant component must be in C++. The goal is to place the performance-sensitive hot path in C++
while keeping workflow and experimentation flexible.

## 3. Performance profiling before optimization

A core engineering principle is: measure before optimizing. If runtime is represented as
$$
T_{total}=T_{compute}+T_{memory}+T_{io}+T_{serialization}+T_{synchronization}
$$
then optimization begins by identifying which component dominates. Without profiling, engineers often optimize the wrong
part of the stack.

Typical profiling questions are:

- where is CPU time spent,
- where are allocations concentrated,
- are cache misses dominating,
- is the bottleneck I/O rather than arithmetic,
- are threads blocked on locks,
- is serialization or object conversion expensive.

## 4. Memory layout and cache behavior

Arithmetic speed alone does not determine performance. Modern processors are heavily influenced by memory locality.
Contiguous, cache-friendly access patterns often outperform theoretically elegant but fragmented object graphs.

Practical implications include:

- avoid unnecessary copying of large vectors and matrices,
- prefer layouts aligned with actual access patterns,
- reduce temporary object creation in inner loops,
- be aware that pointer-heavy structures can create cache-miss storms.

The design rule is simple: data layout is part of algorithm design.

## 5. Threading pitfalls

Multithreading can improve throughput, but only when the workload is suitable. Common pitfalls include:

- race conditions,
- deadlocks,
- false sharing,
- oversubscription,
- non-deterministic ordering effects,
- difficult reproducibility in lazy or observer-based frameworks.

These issues matter especially in finance because reproducibility and explainability are often as important as raw speed.
A fast but non-reproducible risk number is an operational problem.

## 6. Vectorization and numerical kernels

Vectorization means performing arithmetic on arrays or blocks of data rather than scalar elements one by one. In a simple
inner product,
$$
\sum_{i=1}^{n} x_i y_i
$$
a vectorized or BLAS-backed implementation can dramatically outperform naive scalar loops.

Vectorization helps most when:

- data is contiguous,
- operations are applied in bulk,
- branch-heavy control flow is minimized,
- conversions between Python objects and numerical buffers are avoided.

## 7. Numerical stability

Performance alone is insufficient. Numerical routines must also be stable. Common failure modes include:

- subtractive cancellation,
- overflow and underflow,
- unstable root-finding,
- ill-conditioned matrix problems,
- inappropriate finite-difference bump sizes.

A useful engineering principle is to view numerical stability and performance as co-equal requirements. A fast engine
that behaves poorly near edge cases will generate support and control issues quickly.

## 8. Wrapping C++ for higher-level use

A practical platform often uses C++ for the hot path and Python for orchestration. The general pattern is
$$
\text{C++ pricing and risk kernels} \rightarrow \text{language bindings} \rightarrow \text{Python workflows and APIs}
$$
Binding technologies such as `pybind11` make this architecture straightforward. The advantage is that users can work in a
productive Python environment while the heavy numerical work remains in optimized C++ code.

Important design points for bindings include:

- minimize unnecessary copying between Python and C++,
- expose stable and typed interfaces rather than internal implementation details,
- preserve deterministic error handling,
- keep object lifetime semantics clear.

### Current repository mapping

The built Python extension is named `quant_risk_platform`. Binding groups are
split by domain, analytics, I/O, market, and revaluation APIs:

- domain and market DTOs are passed as typed objects loaded from JSON;
- analytics bindings expose valuation, deterministic risk, stress, Monte Carlo,
  PnL explain, historical VaR/ES contributions, and covariance helpers;
- `create_revaluation_session(...)` returns a C++-owned `RevaluationSession`
  that keeps mutable market state and cached instruments on the C++ side while
  exposing Python methods for quote updates, scenario application, pricing,
  reset, and base/shocked/restored reporting.

This keeps Python ergonomic without exposing raw QuantLib handles or requiring
Python callers to manage observer lifetimes.

## 9. Choosing the split in practice

The best language split is driven by the workload, not by ideology. A sensible default is:

- Python for workflow, notebooks, data preparation, and user-facing access,
- C++ for market-object construction, pricing kernels, scenario revaluation loops, and performance-critical risk
  aggregation.

If profiling later shows that a Python layer is the bottleneck, then more functionality can be pushed downward. If it
shows that I/O or database access dominates, then rewriting numerical code in C++ will not help much.

## 10. Relationship to the rest of the documentation

- `docs/architecture/ARCHITECTURE.md` explains where analytics services and bindings fit in the overall platform.
- `docs/architecture/PLATFORM_IMPLEMENTATION_GUIDE.md` explains the high-level pipeline from data to APIs.
- `docs/architecture/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md` explains the persistence and audit layers that surround the
  analytics engine.
