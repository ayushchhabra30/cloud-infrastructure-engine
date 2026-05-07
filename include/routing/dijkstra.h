#pragma once
/*
 * PERSON 2 — Routing & Path Optimization
 * Algorithms: Dijkstra, Bellman-Ford, Floyd-Warshall, A*, Priority Queue
 */
#include <vector>
#include <queue>
#include <functional>
#include <limits>
#include "../graph_core/traversals.h"

namespace CloudInfra {

// ──────────────────────────────────────────────────────────────────
// Shared path result
// ──────────────────────────────────────────────────────────────────
struct PathResult {
    std::vector<int> path;    // node sequence src → dst
    double           cost;    // total cost / latency
    bool             found;
};

// ──────────────────────────────────────────────────────────────────
// Algorithm 6 — Dijkstra's Algorithm (single-source shortest path)
// Complexity: O((V + E) log V) with binary heap
// ──────────────────────────────────────────────────────────────────
struct DijkstraResult {
    std::vector<double> dist;
    std::vector<int>    parent;
};

DijkstraResult dijkstra(const Graph& g, int src);
PathResult     dijkstraPath(const Graph& g, int src, int dst);

// ──────────────────────────────────────────────────────────────────
// Algorithm 7 — Bellman-Ford (handles negative weights, detects
//               negative cycles — useful for cost/penalty edges)
// Complexity: O(V·E)
// ──────────────────────────────────────────────────────────────────
struct BellmanFordResult {
    std::vector<double> dist;
    std::vector<int>    parent;
    bool                has_negative_cycle;
};

BellmanFordResult bellmanFord(const Graph& g, int src);
PathResult        bellmanFordPath(const Graph& g, int src, int dst);

// ──────────────────────────────────────────────────────────────────
// Algorithm 8 — Floyd-Warshall (all-pairs shortest paths)
// Complexity: O(V³)  — use for small topology snapshots
// ──────────────────────────────────────────────────────────────────
struct FloydWarshallResult {
    std::vector<std::vector<double>> dist;   // dist[i][j]
    std::vector<std::vector<int>>    next;   // path reconstruction
};

FloydWarshallResult floydWarshall(const Graph& g);
PathResult          fwPath(const FloydWarshallResult& fw, int src, int dst);

// ──────────────────────────────────────────────────────────────────
// Algorithm 9 — A* Search (heuristic-guided shortest path)
// heuristic(node, goal) must be admissible
// ──────────────────────────────────────────────────────────────────
PathResult aStar(const Graph& g, int src, int dst,
                 std::function<double(int, int)> heuristic);

// Default heuristic: zero (degenerates to Dijkstra)
PathResult aStar(const Graph& g, int src, int dst);

// ──────────────────────────────────────────────────────────────────
// Algorithm 10 — Priority Queue Scheduler
// Wraps std::priority_queue for SLA-aware job dispatch;
// returns node IDs sorted by urgency score
// ──────────────────────────────────────────────────────────────────
struct PQEntry {
    int    node_id;
    double urgency;  // higher = serve first
    bool operator<(const PQEntry& o) const { return urgency < o.urgency; }
};

class PriorityQueueScheduler {
public:
    void   push(int node_id, double urgency);
    PQEntry pop();
    bool   empty() const;
    int    size()  const;
private:
    std::priority_queue<PQEntry> pq_;
};

} // namespace CloudInfra
