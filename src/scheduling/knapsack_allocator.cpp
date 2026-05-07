#include "../../include/scheduling/knapsack_allocator.h"
#include <algorithm>
#include <numeric>
#include <queue>
#include <climits>
#include <stdexcept>

namespace CloudInfra {

// ═══════════════════════════════════════════════════════════════════
// Algorithm 16 — 0/1 Knapsack
// ═══════════════════════════════════════════════════════════════════
KnapsackResult knapsack01(const std::vector<KnapsackItem>& items, int capacity) {
    int n = items.size();
    // dp[i][w] = max value using first i items with weight <= w
    std::vector<std::vector<double>> dp(n + 1, std::vector<double>(capacity + 1, 0));

    for (int i = 1; i <= n; ++i) {
        for (int w = 0; w <= capacity; ++w) {
            dp[i][w] = dp[i-1][w];
            if (items[i-1].weight <= w) {
                dp[i][w] = std::max(dp[i][w], dp[i-1][w - items[i-1].weight] + items[i-1].value);
            }
        }
    }

    // Traceback
    KnapsackResult res;
    res.total_value  = dp[n][capacity];
    res.total_weight = 0;
    int w = capacity;
    for (int i = n; i >= 1; --i) {
        if (dp[i][w] != dp[i-1][w]) {
            res.selected_ids.push_back(items[i-1].id);
            res.total_weight += items[i-1].weight;
            w -= items[i-1].weight;
        }
    }
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 17 — Bin Packing (First-Fit Decreasing)
// ═══════════════════════════════════════════════════════════════════
BinPackResult binPackingFFD(std::vector<double> item_sizes, double bin_capacity) {
    // Sort by decreasing size with original indices
    int n = item_sizes.size();
    std::vector<int> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        return item_sizes[a] > item_sizes[b];
    });

    BinPackResult res;
    std::vector<double> bin_remaining;

    for (int i : idx) {
        double sz = item_sizes[i];
        bool placed = false;
        for (int b = 0; b < (int)bin_remaining.size(); ++b) {
            if (bin_remaining[b] >= sz) {
                res.bins[b].push_back(i);
                bin_remaining[b] -= sz;
                placed = true;
                break;
            }
        }
        if (!placed) {
            res.bins.push_back({i});
            bin_remaining.push_back(bin_capacity - sz);
        }
    }
    res.num_bins = res.bins.size();
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 18 — Job Sequencing with Deadlines (Greedy)
// ═══════════════════════════════════════════════════════════════════
JobSeqResult jobSequencing(std::vector<ScheduledJob> jobs) {
    // Sort by descending profit
    std::sort(jobs.begin(), jobs.end(), [](const ScheduledJob& a, const ScheduledJob& b) {
        return a.profit > b.profit;
    });

    int max_deadline = 0;
    for (auto& j : jobs) max_deadline = std::max(max_deadline, j.deadline);

    std::vector<int> slot(max_deadline + 1, -1);  // slot[t] = job id
    JobSeqResult res;
    res.total_profit = 0;
    res.sequence.resize(max_deadline, -1);

    for (auto& j : jobs) {
        // Find latest available slot <= deadline
        for (int t = std::min(j.deadline, max_deadline); t >= 1; --t) {
            if (slot[t] == -1) {
                slot[t] = j.id;
                res.total_profit += j.profit;
                break;
            }
        }
    }

    for (int t = 1; t <= max_deadline; ++t)
        if (slot[t] != -1) res.sequence.push_back(slot[t]);

    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 19 — Interval Scheduling (Earliest Finish Greedy)
// ═══════════════════════════════════════════════════════════════════
std::vector<int> intervalScheduling(std::vector<Interval> intervals) {
    std::sort(intervals.begin(), intervals.end(), [](const Interval& a, const Interval& b) {
        return a.finish < b.finish;
    });

    std::vector<int> selected;
    int last_finish = -1;
    for (auto& iv : intervals) {
        if (iv.start >= last_finish) {
            selected.push_back(iv.id);
            last_finish = iv.finish;
        }
    }
    return selected;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 20 — Hungarian Algorithm (O(n³))
// ═══════════════════════════════════════════════════════════════════
HungarianResult hungarian(const std::vector<std::vector<double>>& cost) {
    int n = cost.size();
    const double INF_D = 1e18;

    std::vector<double> u(n+1), v(n+1);
    std::vector<int>    p(n+1, 0), way(n+1, 0);

    for (int i = 1; i <= n; ++i) {
        p[0] = i;
        int j0 = 0;
        std::vector<double> minVal(n+1, INF_D);
        std::vector<bool>   used(n+1, false);

        do {
            used[j0] = true;
            int i0 = p[j0], j1 = -1;
            double delta = INF_D;
            for (int j = 1; j <= n; ++j) {
                if (!used[j]) {
                    double cur = cost[i0-1][j-1] - u[i0] - v[j];
                    if (cur < minVal[j]) { minVal[j] = cur; way[j] = j0; }
                    if (minVal[j] < delta) { delta = minVal[j]; j1 = j; }
                }
            }
            for (int j = 0; j <= n; ++j) {
                if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                else          minVal[j] -= delta;
            }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }

    HungarianResult res;
    res.assignment.resize(n);
    res.total_cost = 0;
    for (int j = 1; j <= n; ++j) {
        if (p[j] != 0) {
            res.assignment[p[j]-1] = j-1;
            res.total_cost += cost[p[j]-1][j-1];
        }
    }
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 21 — Minimum Cost Flow (SPFA / successive shortest paths)
// ═══════════════════════════════════════════════════════════════════
MinCostFlow::MinCostFlow(int V) : V_(V), graph_(V) {}

void MinCostFlow::addEdge(int from, int to, int cap, int cost) {
    int rev_from = graph_[to].size();
    int rev_to   = graph_[from].size();
    graph_[from].push_back({to,  cap,  cost, 0, rev_from});
    graph_[to  ].push_back({from, 0,  -cost, 0, rev_to  });
}

bool MinCostFlow::spfa(int src, int sink, std::vector<int>& dist,
                       std::vector<int>& prevv, std::vector<int>& preve) {
    dist.assign(V_, INT_MAX);
    std::vector<bool> inq(V_, false);
    dist[src] = 0;
    std::queue<int> q;
    q.push(src); inq[src] = true;

    while (!q.empty()) {
        int v = q.front(); q.pop(); inq[v] = false;
        for (int i = 0; i < (int)graph_[v].size(); ++i) {
            auto& e = graph_[v][i];
            if (e.cap > e.flow && dist[v] + e.cost < dist[e.to]) {
                dist[e.to] = dist[v] + e.cost;
                prevv[e.to] = v;
                preve[e.to] = i;
                if (!inq[e.to]) { q.push(e.to); inq[e.to] = true; }
            }
        }
    }
    return dist[sink] != INT_MAX;
}

MCFResult MinCostFlow::solve(int src, int sink, int required_flow) {
    MCFResult res{0, 0, false};
    std::vector<int> dist(V_), prevv(V_), preve(V_);

    while (res.max_flow < required_flow) {
        if (!spfa(src, sink, dist, prevv, preve)) break;

        // Find max push along shortest-cost path
        int d = required_flow - res.max_flow;
        for (int v = sink; v != src; v = prevv[v])
            d = std::min(d, graph_[prevv[v]][preve[v]].cap - graph_[prevv[v]][preve[v]].flow);

        for (int v = sink; v != src; v = prevv[v]) {
            graph_[prevv[v]][preve[v]].flow += d;
            graph_[v][graph_[prevv[v]][preve[v]].rev].flow -= d;
        }
        res.max_flow += d;
        res.min_cost += (double)d * dist[sink];
    }
    res.feasible = (res.max_flow == required_flow);
    return res;
}

} // namespace CloudInfra
