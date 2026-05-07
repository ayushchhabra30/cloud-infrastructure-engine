#pragma once
/*
 * PERSON 5 — Dynamic Programming, Prediction & Randomisation
 * Algorithms: Matrix Chain DP, Weighted Interval DP, Optimal BST DP,
 *             Markov Chain, Monte Carlo, Randomised Load Balancing
 */
#include <vector>
#include <string>
#include <random>
#include <functional>

namespace CloudInfra {

// ──────────────────────────────────────────────────────────────────
// Algorithm 22 — Matrix Chain Multiplication DP
// Used to find optimal computation order for dependent micro-services
// matrices[i] has dimensions dims[i] × dims[i+1]
// ──────────────────────────────────────────────────────────────────
struct MCMResult {
    long long min_ops;          // minimum scalar multiplications
    std::vector<std::vector<int>> split;   // split[i][j] = k for reconstruction
    std::string optimal_order;  // parenthesisation string
};

MCMResult matrixChainMultiplication(const std::vector<int>& dims);

// ──────────────────────────────────────────────────────────────────
// Algorithm 23 — Weighted Interval Scheduling DP
// Maximise total value of non-overlapping jobs with weights
// ──────────────────────────────────────────────────────────────────
struct WeightedInterval {
    int    id;
    int    start;
    int    finish;
    double weight;
};

struct WISResult {
    std::vector<int> selected;
    double           total_weight;
};

WISResult weightedIntervalScheduling(std::vector<WeightedInterval> jobs);

// ──────────────────────────────────────────────────────────────────
// Algorithm 24 — Optimal Binary Search Tree DP (Knuth)
// Minimises expected search cost for a known key access distribution
// ──────────────────────────────────────────────────────────────────
struct OBSTResult {
    double                        min_cost;
    std::vector<std::vector<int>> root;   // root[i][j] = optimal root for keys i..j
};

OBSTResult optimalBST(const std::vector<double>& prob,         // key probabilities
                      const std::vector<double>& dummy_prob);  // dummy key probs

// ──────────────────────────────────────────────────────────────────
// Algorithm 25 — Markov Chain State Transition
// Model node health states: HEALTHY → DEGRADED → FAILED → RECOVERING
// ──────────────────────────────────────────────────────────────────
enum class NodeState { HEALTHY = 0, DEGRADED = 1, FAILED = 2, RECOVERING = 3 };

class MarkovChain {
public:
    // transition[i][j] = P(state j | state i)
    explicit MarkovChain(const std::vector<std::vector<double>>& transition);

    NodeState       step(NodeState current);
    NodeState       simulate(NodeState initial, int ticks);
    std::vector<double> steadyState(int iterations = 10000) const;
    double          failureProbability(NodeState current, int horizon) const;

private:
    std::vector<std::vector<double>> trans_;
    mutable std::mt19937 rng_{std::random_device{}()};
};

// ──────────────────────────────────────────────────────────────────
// Algorithm 26 — Monte Carlo Simulation
// Estimate infrastructure availability and SLA breach probability
// ──────────────────────────────────────────────────────────────────
struct MonteCarloResult {
    double mean_availability;
    double std_dev;
    double sla_breach_probability;
    double p95_latency;
    int    simulations_run;
};

MonteCarloResult monteCarloAvailability(
    int   num_nodes,
    int   num_edges,
    const std::vector<double>& node_failure_probs,
    const std::vector<double>& edge_failure_probs,
    int   iterations = 100000);

// ──────────────────────────────────────────────────────────────────
// Algorithm 27 — Randomised Load Balancing
// Strategies: Random, Power-of-Two-Choices, Consistent Hashing
// ──────────────────────────────────────────────────────────────────
enum class LBStrategy { RANDOM, POWER_OF_TWO, CONSISTENT_HASH, ROUND_ROBIN };

class RandomisedLoadBalancer {
public:
    explicit RandomisedLoadBalancer(int num_nodes, LBStrategy strategy = LBStrategy::POWER_OF_TWO);

    // Returns node id to assign the task to
    int assign(const std::vector<double>& node_loads, int task_id = 0);

    // Update load after assignment
    void updateLoad(int node_id, double delta);

    LBStrategy strategy() const { return strategy_; }

private:
    int        num_nodes_;
    LBStrategy strategy_;
    std::vector<double> loads_;
    int        rr_counter_{0};
    mutable std::mt19937 rng_{std::random_device{}()};

    int randomAssign();
    int powerOfTwoChoices();
    int consistentHash(int task_id);
    int roundRobin();
};

} // namespace CloudInfra
