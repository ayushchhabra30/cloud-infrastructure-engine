# ☁️ CloudInfraEngine

## 📌 Overview

CloudInfraEngine is a large-scale simulation framework designed to model the behavior of modern cloud infrastructure and distributed systems.

The engine represents data centers as graph-based networks and integrates algorithms from graph theory, dynamic programming, greedy optimization, probabilistic modeling, and scheduling to simulate:

- Network routing and traffic flow
- Failure detection and recovery
- Workload placement and resource allocation
- Infrastructure cost optimization
- Predictive failure analysis
- SLA-aware scheduling

The project demonstrates how diverse algorithmic paradigms can work together inside a realistic systems-engineering environment.

---

## ✨ Highlights

- 🧠 27+ DAA algorithms integrated into a single simulation engine
- 🌐 Graph-based cloud topology modeling
- ⚡ Dynamic shortest-path routing
- 🔍 Infrastructure vulnerability detection
- 📈 Predictive failure modeling
- 📦 Intelligent workload scheduling and placement
- 💰 Cost-aware optimization strategies
- 🔄 Real-time rerouting during failures
- 🧪 Comprehensive automated testing suite

---

## 🏗️ Architecture

```text
CloudInfraEngine/
├── include/
│   ├── core/
│   ├── graph_core/
│   ├── routing/
│   ├── vulnerability/
│   ├── scheduling/
│   └── prediction_dp/
├── src/
├── tests/
└── CMakeLists.txt
```

---

## 🛠️ Tech Stack

| Technology | Purpose |
|------------|---------|
| C++17 | Core implementation |
| STL | Data structures & utilities |
| CMake | Build system |
| Graph Algorithms | Routing & topology analysis |
| Dynamic Programming | Scheduling & optimization |
| Greedy Algorithms | Resource allocation |
| Probabilistic Models | Failure prediction |

---

## 🚀 Algorithm Domains

### 🌐 Graph Foundations & Connectivity

- Breadth First Search (BFS)
- Depth First Search (DFS)
- Kosaraju SCC
- Union-Find
- Topological Sort

Used for topology validation, connectivity analysis, cycle detection, and dependency management.

---

### 🛣️ Routing & Path Optimization

- Dijkstra
- Bellman-Ford
- Floyd-Warshall
- A*
- Priority Queue Scheduling

Provides shortest-path computation, routing optimization, and SLA-aware traffic management.

---

### 🛡️ Failure Analysis & Network Resilience

- Articulation Point Detection
- Bridge Detection
- Prim MST
- Kruskal MST
- Segment Tree

Identifies single points of failure and constructs cost-efficient resilient network backbones.

---

### 📦 Scheduling & Resource Allocation

- 0/1 Knapsack
- Bin Packing (FFD)
- Job Sequencing with Deadlines
- Interval Scheduling
- Hungarian Algorithm
- Minimum Cost Flow

Optimizes workload placement and resource utilization across cloud nodes.

---

### 📈 Prediction & Optimization

- Matrix Chain Multiplication
- Weighted Interval Scheduling
- Optimal BST
- Markov Chains
- Monte Carlo Simulation
- Randomized Load Balancing

Enables predictive analytics, failure forecasting, and intelligent decision-making.

---

## ⚙️ Build Instructions

### Prerequisites

- CMake ≥ 3.14
- C++17 compatible compiler

### Build

```bash
mkdir build
cd build

cmake ..
make -j$(nproc)
```

### Run

```bash
./CloudInfraEngine
```

### Execute Tests

```bash
ctest --output-on-failure
```

---

## 🖥️ Sample Simulation Features

- Dynamic cloud topology generation
- Real-time node health monitoring
- SLA compliance tracking
- Load balancing strategies
- Failure injection and recovery
- Workload migration simulation

---

## ⭐ Why This Project?

Rather than implementing algorithms independently, CloudInfraEngine demonstrates how multiple algorithmic paradigms interact within a realistic cloud infrastructure environment, providing a practical bridge between theoretical DAA concepts and large-scale systems engineering.
