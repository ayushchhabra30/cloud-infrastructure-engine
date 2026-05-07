# CloudInfraEngine

> **Production-grade cloud infrastructure simulation engine implementing 27 DAA algorithms across 5 domains.**

---

## Architecture

```
CloudInfraEngine/
├── include/
│   ├── core/               # Shared data structures (CloudNode, CloudEdge, WorkloadTask)
│   ├── graph_core/         # Person 1 — Graph Foundations
│   ├── routing/            # Person 2 — Routing & Path Optimization
│   ├── vulnerability/      # Person 3 — Failure & Backbone Analysis
│   ├── scheduling/         # Person 4 — Scheduling & Resource Allocation
│   └── prediction_dp/      # Person 5 — DP, Prediction & Randomization
├── src/                    # Implementations (.cpp)
├── tests/                  # Automated test suites
└── CMakeLists.txt
```

---

## Algorithm Distribution

| Person | Domain | Algorithms |
|--------|--------|-----------|
| **1** | Graph Foundations & Connectivity | BFS, DFS, Kosaraju SCC, Union-Find, Topological Sort |
| **2** | Routing & Path Optimization | Dijkstra, Bellman-Ford, Floyd-Warshall, A*, Priority Queue |
| **3** | Failure Vulnerability & Backbone | Articulation Points, Bridge Detection, Kruskal MST, Prim MST, Segment Tree |
| **4** | Scheduling & Resource Allocation | 0/1 Knapsack, Bin Packing FFD, Job Sequencing, Interval Scheduling, Hungarian, Min-Cost Flow |
| **5** | DP, Prediction & Randomization | Matrix Chain DP, Weighted Interval DP, Optimal BST, Markov Chain, Monte Carlo, Randomised Load Balancing |

---

## Build Instructions

### Prerequisites
- CMake ≥ 3.14
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)

### Build & Run

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run the full simulation
./CloudInfraEngine

# Run all tests
ctest --output-on-failure
```

### Windows (MSVC)
```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
.\Release\CloudInfraEngine.exe
```

---

## Domain Details

### Person 1 — Graph Foundations
- **BFS**: Layer-by-layer traversal for hop-distance routing, reachability checks
- **DFS**: Deep traversal for dependency resolution, component detection
- **Kosaraju SCC**: Finds groups of mutually reachable nodes (fault domains)
- **Union-Find**: O(α) connectivity tracking with path compression + union-by-rank
- **Topological Sort**: Kahn's BFS-based — validates task dependency DAGs

### Person 2 — Routing
- **Dijkstra**: O((V+E)logV) shortest path for SLA-aware routing
- **Bellman-Ford**: Handles negative cost/penalty edges; detects negative cycles
- **Floyd-Warshall**: All-pairs shortest paths — used for topology snapshots
- **A***: Heuristic-guided search for faster routing in geo-aware topologies
- **Priority Queue**: SLA urgency-based dispatch with O(log n) push/pop

### Person 3 — Vulnerability
- **Articulation Points**: Tarjan's algorithm — finds single-point-of-failure nodes
- **Bridge Detection**: Tarjan's — finds single-point-of-failure links
- **Kruskal MST**: O(E log E) minimum-cost backbone using Union-Find
- **Prim MST**: O(E log V) with heap — finds backup spanning tree
- **Segment Tree**: Range-sum/min/max queries for fast utilization monitoring

### Person 4 — Scheduling
- **0/1 Knapsack**: DP-based optimal workload placement under resource constraints
- **Bin Packing FFD**: Greedy approximation — minimizes servers for task packing
- **Job Sequencing**: Greedy deadline-aware profit maximization
- **Interval Scheduling**: Earliest-finish greedy for maximum job concurrency
- **Hungarian**: O(n³) optimal task-to-node assignment minimizing total cost
- **Min-Cost Flow**: SPFA-based successive shortest path for network flow routing

### Person 5 — DP & Prediction
- **Matrix Chain DP**: Optimal computation order for micro-service chains
- **Weighted Interval DP**: DP-based maximum-weight non-overlapping job selection
- **Optimal BST**: Knuth's DP for minimum expected lookup cost
- **Markov Chain**: 4-state node health model (HEALTHY/DEGRADED/FAILED/RECOVERING)
- **Monte Carlo**: 50k+ simulation runs for SLA breach probability estimation
- **Randomized LB**: Power-of-2-choices, consistent hashing, round-robin strategies

---

## Testing

Each test binary returns exit code 0 on full pass:
```
tests/test_graph_core.cpp   — 20 tests covering BFS/DFS/SCC/UF/Topo
tests/test_routing.cpp      — 18 tests covering Dijkstra/BF/FW/A*/PQ
tests/test_scheduling.cpp   — 18 tests covering all P4 algorithms
```

---

## License
MIT — Academic/educational use.
