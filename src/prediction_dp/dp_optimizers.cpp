#include "../../include/prediction_dp/dp_optimizers.h"
#include "../../include/core/constants.h"
#include <climits>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace CloudInfra {

// ═══════════════════════════════════════════════════════════════════
// Algorithm 22 — Matrix Chain Multiplication DP
// ═══════════════════════════════════════════════════════════════════
static std::string buildExpr(const std::vector<std::vector<int>>& s, int i, int j) {
    if (i == j) return "M" + std::to_string(i);
    return "(" + buildExpr(s, i, s[i][j]) + " × " + buildExpr(s, s[i][j]+1, j) + ")";
}

MCMResult matrixChainMultiplication(const std::vector<int>& dims) {
    int n = (int)dims.size() - 1; // number of matrices
    MCMResult res;
    res.split.assign(n, std::vector<int>(n, 0));

    std::vector<std::vector<long long>> dp(n, std::vector<long long>(n, 0));

    for (int len = 2; len <= n; ++len) {
        for (int i = 0; i <= n - len; ++i) {
            int j = i + len - 1;
            dp[i][j] = LLONG_MAX;
            for (int k = i; k < j; ++k) {
                long long q = dp[i][k] + dp[k+1][j] + (long long)dims[i]*dims[k+1]*dims[j+1];
                if (q < dp[i][j]) {
                    dp[i][j] = q;
                    res.split[i][j] = k;
                }
            }
        }
    }
    res.min_ops      = dp[0][n-1];
    res.optimal_order = buildExpr(res.split, 0, n-1);
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 23 — Weighted Interval Scheduling DP
// ═══════════════════════════════════════════════════════════════════
WISResult weightedIntervalScheduling(std::vector<WeightedInterval> jobs) {
    std::sort(jobs.begin(), jobs.end(), [](const WeightedInterval& a, const WeightedInterval& b) {
        return a.finish < b.finish;
    });

    int n = jobs.size();
    // p[i] = largest j < i such that job j is compatible with i
    auto latestCompatible = [&](int i) {
        for (int j = i - 1; j >= 0; --j)
            if (jobs[j].finish <= jobs[i].start) return j;
        return -1;
    };

    std::vector<double> dp(n + 1, 0);
    for (int i = 1; i <= n; ++i) {
        int p = latestCompatible(i - 1);
        double include = jobs[i-1].weight + (p >= 0 ? dp[p+1] : 0);
        dp[i] = std::max(dp[i-1], include);
    }

    // Traceback
    WISResult res;
    res.total_weight = dp[n];
    int i = n;
    while (i >= 1) {
        int p = latestCompatible(i - 1);
        double include = jobs[i-1].weight + (p >= 0 ? dp[p+1] : 0);
        if (include >= dp[i-1]) {
            res.selected.push_back(jobs[i-1].id);
            i = (p >= 0) ? p + 1 : 0;
        } else {
            --i;
        }
    }
    std::reverse(res.selected.begin(), res.selected.end());
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 24 — Optimal BST (Knuth's DP) O(n³)
// ═══════════════════════════════════════════════════════════════════
OBSTResult optimalBST(const std::vector<double>& prob,
                      const std::vector<double>& dummy_prob) {
    int n = prob.size();
    OBSTResult res;
    res.root.assign(n + 1, std::vector<int>(n, 0));

    // e[i][j] = expected cost for keys i..j (1-indexed)
    std::vector<std::vector<double>> e(n + 2, std::vector<double>(n + 1, 0));
    std::vector<std::vector<double>> w(n + 2, std::vector<double>(n + 1, 0));

    for (int i = 1; i <= n + 1; ++i) {
        e[i][i-1] = dummy_prob[i-1];
        w[i][i-1] = dummy_prob[i-1];
    }

    for (int len = 1; len <= n; ++len) {
        for (int i = 1; i <= n - len + 1; ++i) {
            int j = i + len - 1;
            e[i][j] = 1e18;
            w[i][j] = w[i][j-1] + prob[j-1] + dummy_prob[j];

            int r_lo = res.root[i][j-1] > 0 ? res.root[i][j-1] : i;
            int r_hi = (j < n) ? (res.root[i+1][j] > 0 ? res.root[i+1][j] : j) : j;

            for (int r = r_lo; r <= r_hi; ++r) {
                double t = e[i][r-1] + e[r+1][j] + w[i][j];
                if (t < e[i][j]) {
                    e[i][j] = t;
                    res.root[i][j] = r;
                }
            }
        }
    }
    res.min_cost = e[1][n];
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 25 — Markov Chain
// ═══════════════════════════════════════════════════════════════════
MarkovChain::MarkovChain(const std::vector<std::vector<double>>& transition)
    : trans_(transition) {}

NodeState MarkovChain::step(NodeState current) {
    int s = (int)current;
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng_);
    double cumulative = 0;
    for (int j = 0; j < (int)trans_[s].size(); ++j) {
        cumulative += trans_[s][j];
        if (r < cumulative) return (NodeState)j;
    }
    return (NodeState)(trans_[s].size() - 1);
}

NodeState MarkovChain::simulate(NodeState initial, int ticks) {
    NodeState s = initial;
    for (int t = 0; t < ticks; ++t) s = step(s);
    return s;
}

std::vector<double> MarkovChain::steadyState(int iterations) const {
    int S = trans_.size();
    std::vector<double> pi(S, 1.0 / S);
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<double> next(S, 0);
        for (int i = 0; i < S; ++i)
            for (int j = 0; j < S; ++j)
                next[j] += pi[i] * trans_[i][j];
        pi = next;
    }
    return pi;
}

double MarkovChain::failureProbability(NodeState current, int horizon) const {
    int S = trans_.size();
    std::vector<double> dist_(S, 0);
    dist_[(int)current] = 1.0;
    for (int t = 0; t < horizon; ++t) {
        std::vector<double> next(S, 0);
        for (int i = 0; i < S; ++i)
            for (int j = 0; j < S; ++j)
                next[j] += dist_[i] * trans_[i][j];
        dist_ = next;
    }
    return dist_[(int)NodeState::FAILED];
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 26 — Monte Carlo Availability Simulation
// ═══════════════════════════════════════════════════════════════════
MonteCarloResult monteCarloAvailability(
    int   num_nodes,
    int   num_edges,
    const std::vector<double>& node_failure_probs,
    const std::vector<double>& edge_failure_probs,
    int   iterations)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::vector<double> availabilities;
    availabilities.reserve(iterations);

    for (int iter = 0; iter < iterations; ++iter) {
        int alive_nodes = 0;
        for (int i = 0; i < num_nodes; ++i)
            if (dist(rng) >= node_failure_probs[i]) ++alive_nodes;

        int alive_edges = 0;
        for (int i = 0; i < num_edges; ++i)
            if (dist(rng) >= edge_failure_probs[i]) ++alive_edges;

        double avail = (double)(alive_nodes + alive_edges) / (num_nodes + num_edges);
        availabilities.push_back(avail);
    }

    double mean = 0;
    for (double a : availabilities) mean += a;
    mean /= iterations;

    double var = 0;
    for (double a : availabilities) var += (a - mean) * (a - mean);
    var /= iterations;

    std::sort(availabilities.begin(), availabilities.end());
    double p95 = availabilities[(int)(0.95 * iterations)];

    double breach = 0;
    for (double a : availabilities)
        if (a < SLA_AVAILABILITY_MIN) ++breach;

    MonteCarloResult res;
    res.mean_availability       = mean;
    res.std_dev                 = std::sqrt(var);
    res.sla_breach_probability  = breach / iterations;
    res.p95_latency             = 1.0 - p95;  // proxy
    res.simulations_run         = iterations;
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 27 — Randomised Load Balancer
// ═══════════════════════════════════════════════════════════════════
RandomisedLoadBalancer::RandomisedLoadBalancer(int num_nodes, LBStrategy strategy)
    : num_nodes_(num_nodes), strategy_(strategy), loads_(num_nodes, 0.0) {}

int RandomisedLoadBalancer::assign(const std::vector<double>& node_loads, int task_id) {
    // Sync external loads
    for (int i = 0; i < std::min((int)node_loads.size(), num_nodes_); ++i)
        loads_[i] = node_loads[i];

    switch (strategy_) {
        case LBStrategy::RANDOM:          return randomAssign();
        case LBStrategy::POWER_OF_TWO:    return powerOfTwoChoices();
        case LBStrategy::CONSISTENT_HASH: return consistentHash(task_id);
        case LBStrategy::ROUND_ROBIN:     return roundRobin();
    }
    return 0;
}

void RandomisedLoadBalancer::updateLoad(int node_id, double delta) {
    if (node_id >= 0 && node_id < num_nodes_) loads_[node_id] += delta;
}

int RandomisedLoadBalancer::randomAssign() {
    std::uniform_int_distribution<int> d(0, num_nodes_ - 1);
    return d(rng_);
}

int RandomisedLoadBalancer::powerOfTwoChoices() {
    std::uniform_int_distribution<int> d(0, num_nodes_ - 1);
    int a = d(rng_), b = d(rng_);
    return (loads_[a] <= loads_[b]) ? a : b;
}

int RandomisedLoadBalancer::consistentHash(int task_id) {
    // Simple hash ring
    return std::hash<int>{}(task_id) % num_nodes_;
}

int RandomisedLoadBalancer::roundRobin() {
    int node = rr_counter_ % num_nodes_;
    ++rr_counter_;
    return node;
}

} // namespace CloudInfra
