# Benchmarks

## Methodology

- **Environment:** 10×10 GridWorld, fixed start `(0, 0)`, fixed goal `(9, 9)`, optimal path = `18` steps.
- **Settings:** Fixed base `seed = 14`, headless `--benchmark` mode (no rendering, no CSV or stdout I/O inside the timed loop).
- **Platform:** macOS (Apple Silicon ARM64), compiled with `g++ -O2` (`-pthread` for multithreading).
- **Profiling tool:** macOS `sample` (1 ms sampling interval, compiled with `-O2 -g`).
- **Correctness check:** Every run finishes with an automatic headless greedy evaluation (`100` episodes per agent). An optimization is only valid if evaluation stays at **100.00% success** and **18.00 avg steps**.

---

## Single-Threaded Optimization Progression

Workload: 5,000,000 episodes

| Version  | Change                                                          | Duration | Steps/sec | Speedup vs Baseline | Headless Eval |
| -------- | --------------------------------------------------------------- | -------: | --------: | ------------------: | ------------- |
| Baseline | Headless benchmark mode                                         | 0.1124 s |     34.6M |               1.00x | 100% / 18.00  |
| Opt #1   | Inline `isValidAction` in `game.h` (remove `game.cpp`)          | 1.3031 s |     72.7M |               2.10x | 100% / 18.00  |
| Opt #2   | Replace raylib `GetRandomValue` with local xorshift64 (`rng.h`) | 1.1111 s |     85.2M |               2.46x | 100% / 18.00  |
| Opt #3   | Check exploration before Q-table scan in `chooseAction`         | 1.0942 s |     86.5M |               2.50x | 100% / 18.00  |

### Profiler Findings

1. **Cross-translation-unit call overhead (`isValidAction`, ~39% of CPU):** Living in `game.cpp` prevented the compiler from inlining `isValidAction` into `q_learning.cpp`, causing 8+ real function calls per step (~800M calls). Moving it to `game.h` as `inline` also unlocked loop unrolling and eliminated the `switch` statement → **2.1x speedup**.

2. **Dynamic library RNG (`GetRandomValue`, ~18% of CPU):** Every random number crossed a `DYLD-STUB` boundary into `libraylib.dylib` and used global non-thread-safe state. Replacing it with an inline `xorshift64*` RNG gave a **~17% speedup**, decoupled `q_learning.cpp` from raylib, and enabled safe multithreading.

3. **Branch frequency matters (`chooseAction`) :** Checking exploration before scanning Q-values only gave a small gain (~1.5%) on a 5M-episode run because `epsilon` decays to `0.05` after ~6,000 episodes so ~95% of steps are exploitation anyway.

4. **Profile before guessing:** `std::deque` in `updateStats` took **< 1%** of CPU time; profiling prevented wasted optimization effort.

---

## Multithreaded Scaling (Independent Environments)

Workload: `5,000,000` total episodes split across `N` independent `std::thread` workers (each with its own State, Game, QLearning, and Rng{seed + t}).

| Threads | Duration | Steps/sec | Episodes/sec | Speedup vs 1T | Eval (all threads) |
| ------: | -------: | --------: | -----------: | ------------: | ------------------ |
|       1 | 1.0942 s |    86.54M |        4.57M |         1.00x | 100% / 18.00       |
|       2 | 0.6086 s |   155.76M |        8.22M |         1.80x | 100% / 18.00       |
|       4 | 0.3144 s |   302.18M |       15.90M |         3.48x | 100% / 18.00       |
|       8 | 0.2322 s |   410.90M |       21.53M |         4.71x | 100% / 18.00       |
|      16 | 0.2251 s |   427.55M |       22.21M |         4.86x | 100% / 18.00       |

---

## When Parallelism Hurts (Small Workload Stress Test)

Workload: `1,000` total episodes.

| Mode                        |        Duration | Steps/sec | Total Steps | Eval Success | Eval Avg Steps |
| --------------------------- | --------------: | --------: | ----------: | -----------: | -------------: |
| 1 thread (`1000` eps/agent) |        0.0026 s |    35.97M |      94,148 |  **100.00%** |      **18.00** |
| 8 threads (`125` eps/agent) | 0.0011–0.0020 s | 140M–257M |     278,558 |   **37.50%** |     **131.75** |

---

## Tradeoffs & Limitations

- **Thread overhead on tiny workloads:** Spawning and joining OS threads adds 0.5-1 ms, eroding speedups on short runs.
- **Throughput != sample efficiency:** Splitting a small episode budget across independent Q-tables gives each agent `1/N` of the experience, so none converge. Independent multithreading is useful for running **multiple seeds or hyperparameter sweeps** concurrently (each running a full episode budget), not for accelerating a single agent's convergence.
- **Why no shared Q-table updates:** A single step takes around 11 ns and the entire Q-table is small. Per-step locking or atomic cache-line contention across cores would cost more than the step itself and would break fixed-seed determinism.
