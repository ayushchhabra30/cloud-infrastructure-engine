#pragma once
/*
 * PERSON 4 — Scheduling & Resource Allocation
 * Algorithms: 0/1 Knapsack, Bin Packing, Job Sequencing,
 *             Interval Scheduling, Hungarian, Min-Cost Flow
 */
#include <vector>
#include <string>
#include <climits>

namespace CloudInfra {

// ──────────────────────────────────────────────────────────────────
// Algorithm 16 — 0/1 Knapsack for Workload Placement
// items = tasks, capacity = node's free resources
// ──────────────────────────────────────────────────────────────────
struct KnapsackItem {
    int    id;
    int    weight;   // resource units required
    double value;    // business value
};

struct KnapsackResult {
    std::vector<int> selected_ids;
    double           total_value;
    int              total_weight;
};

KnapsackResult knapsack01(const std::vector<KnapsackItem>& items, int capacity);

// ──────────────────────────────────────────────────────────────────
// Algorithm 17 — Bin Packing (First-Fit Decreasing approximation)
// Pack tasks onto servers minimising number of servers used
// ──────────────────────────────────────────────────────────────────
struct BinPackResult {
    std::vector<std::vector<int>> bins;   // bins[b] = list of item ids
    int num_bins;
};

BinPackResult binPackingFFD(std::vector<double> item_sizes, double bin_capacity);

// ──────────────────────────────────────────────────────────────────
// Algorithm 18 — Job Sequencing with Deadlines (Greedy)
// Maximise profit by selecting jobs that fit within their deadlines
// ──────────────────────────────────────────────────────────────────
struct ScheduledJob {
    int    id;
    int    deadline;
    double profit;
};

struct JobSeqResult {
    std::vector<int> sequence;   // job ids in slot order
    double           total_profit;
};

JobSeqResult jobSequencing(std::vector<ScheduledJob> jobs);

// ──────────────────────────────────────────────────────────────────
// Algorithm 19 — Interval Scheduling (Maximum cardinality)
// Select maximum non-overlapping intervals (greedy earliest finish)
// ──────────────────────────────────────────────────────────────────
struct Interval {
    int id;
    int start;
    int finish;
};

std::vector<int> intervalScheduling(std::vector<Interval> intervals);

// ──────────────────────────────────────────────────────────────────
// Algorithm 20 — Hungarian Algorithm (optimal assignment)
// Minimise total cost of assigning n tasks to n nodes
// Cost matrix cost[task][node]
// ──────────────────────────────────────────────────────────────────
struct HungarianResult {
    std::vector<int> assignment;  // assignment[task] = node
    double           total_cost;
};

HungarianResult hungarian(const std::vector<std::vector<double>>& cost);

// ──────────────────────────────────────────────────────────────────
// Algorithm 21 — Minimum Cost Flow (MCMF via SPFA / Bellman-Ford)
// Minimise cost while routing `required_flow` units through network
// ──────────────────────────────────────────────────────────────────
struct MCFEdge {
    int    to, cap, cost, flow;
    int    rev;   // index of reverse edge in adj[to]
};

struct MCFResult {
    int    max_flow;
    double min_cost;
    bool   feasible;
};

class MinCostFlow {
public:
    explicit MinCostFlow(int V);
    void addEdge(int from, int to, int cap, int cost);
    MCFResult solve(int src, int sink, int required_flow = INT_MAX);

private:
    int V_;
    std::vector<std::vector<MCFEdge>> graph_;
    bool spfa(int src, int sink, std::vector<int>& dist, std::vector<int>& prevv,
              std::vector<int>& preve);
};

} // namespace CloudInfra
