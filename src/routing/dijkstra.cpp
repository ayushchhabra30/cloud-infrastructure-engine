#include "../../include/routing/dijkstra.h"
#include "../../include/core/constants.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace CloudInfra {

// ═══════════════════════════════════════════════════════════════════
// Algorithm 6 — Dijkstra's Algorithm
// ═══════════════════════════════════════════════════════════════════
DijkstraResult dijkstra(const Graph& g, int src) {
    DijkstraResult res;
    res.dist.assign(g.V, INF);
    res.parent.assign(g.V, -1);
    res.dist[src] = 0;

    // min-heap: {dist, node}
    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    pq.push({0.0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > res.dist[u]) continue;  // stale entry
        for (auto [v, w] : g.adj[u]) {
            if (res.dist[u] + w < res.dist[v]) {
                res.dist[v]  = res.dist[u] + w;
                res.parent[v] = u;
                pq.push({res.dist[v], v});
            }
        }
    }
    return res;
}

PathResult dijkstraPath(const Graph& g, int src, int dst) {
    auto res = dijkstra(g, src);
    PathResult pr;
    pr.cost  = res.dist[dst];
    pr.found = res.dist[dst] < INF;
    if (pr.found) {
        for (int v = dst; v != -1; v = res.parent[v])
            pr.path.push_back(v);
        std::reverse(pr.path.begin(), pr.path.end());
    }
    return pr;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 7 — Bellman-Ford
// ═══════════════════════════════════════════════════════════════════
BellmanFordResult bellmanFord(const Graph& g, int src) {
    BellmanFordResult res;
    res.dist.assign(g.V, INF);
    res.parent.assign(g.V, -1);
    res.has_negative_cycle = false;
    res.dist[src] = 0;

    // Collect all edges
    struct Edge { int u, v; double w; };
    std::vector<Edge> edges;
    for (int u = 0; u < g.V; ++u)
        for (auto [v, w] : g.adj[u])
            edges.push_back({u, v, w});

    // Relax V-1 times
    for (int i = 0; i < g.V - 1; ++i) {
        for (auto& e : edges) {
            if (res.dist[e.u] < INF && res.dist[e.u] + e.w < res.dist[e.v]) {
                res.dist[e.v]   = res.dist[e.u] + e.w;
                res.parent[e.v] = e.u;
            }
        }
    }

    // Detect negative cycle
    for (auto& e : edges) {
        if (res.dist[e.u] < INF && res.dist[e.u] + e.w < res.dist[e.v]) {
            res.has_negative_cycle = true;
            break;
        }
    }
    return res;
}

PathResult bellmanFordPath(const Graph& g, int src, int dst) {
    auto res = bellmanFord(g, src);
    PathResult pr;
    pr.cost  = res.dist[dst];
    pr.found = res.dist[dst] < INF && !res.has_negative_cycle;
    if (pr.found) {
        for (int v = dst; v != -1; v = res.parent[v])
            pr.path.push_back(v);
        std::reverse(pr.path.begin(), pr.path.end());
    }
    return pr;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 8 — Floyd-Warshall
// ═══════════════════════════════════════════════════════════════════
FloydWarshallResult floydWarshall(const Graph& g) {
    int V = g.V;
    FloydWarshallResult res;
    res.dist.assign(V, std::vector<double>(V, INF));
    res.next.assign(V, std::vector<int>(V, -1));

    // Initialise
    for (int i = 0; i < V; ++i) res.dist[i][i] = 0;
    for (int u = 0; u < V; ++u)
        for (auto [v, w] : g.adj[u]) {
            if (w < res.dist[u][v]) {
                res.dist[u][v] = w;
                res.next[u][v] = v;
            }
        }

    // DP
    for (int k = 0; k < V; ++k)
        for (int i = 0; i < V; ++i)
            for (int j = 0; j < V; ++j)
                if (res.dist[i][k] < INF && res.dist[k][j] < INF)
                    if (res.dist[i][k] + res.dist[k][j] < res.dist[i][j]) {
                        res.dist[i][j] = res.dist[i][k] + res.dist[k][j];
                        res.next[i][j] = res.next[i][k];
                    }
    return res;
}

PathResult fwPath(const FloydWarshallResult& fw, int src, int dst) {
    PathResult pr;
    pr.cost  = fw.dist[src][dst];
    pr.found = pr.cost < INF;
    if (!pr.found || fw.next[src][dst] == -1) return pr;

    int cur = src;
    while (cur != dst) {
        pr.path.push_back(cur);
        cur = fw.next[cur][dst];
    }
    pr.path.push_back(dst);
    return pr;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 9 — A* Search
// ═══════════════════════════════════════════════════════════════════
PathResult aStar(const Graph& g, int src, int dst,
                 std::function<double(int, int)> heuristic) {
    PathResult pr;
    std::vector<double> g_cost(g.V, INF);
    std::vector<int>    parent(g.V, -1);
    g_cost[src] = 0;

    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> open;
    open.push({heuristic(src, dst), src});

    while (!open.empty()) {
        auto [f, u] = open.top(); open.pop();
        if (u == dst) break;

        for (auto [v, w] : g.adj[u]) {
            double ng = g_cost[u] + w;
            if (ng < g_cost[v]) {
                g_cost[v] = ng;
                parent[v] = u;
                open.push({ng + heuristic(v, dst), v});
            }
        }
    }

    pr.cost  = g_cost[dst];
    pr.found = pr.cost < INF;
    if (pr.found) {
        for (int v = dst; v != -1; v = parent[v])
            pr.path.push_back(v);
        std::reverse(pr.path.begin(), pr.path.end());
    }
    return pr;
}

PathResult aStar(const Graph& g, int src, int dst) {
    return aStar(g, src, dst, [](int, int) { return 0.0; });
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 10 — Priority Queue Scheduler
// ═══════════════════════════════════════════════════════════════════
void PriorityQueueScheduler::push(int node_id, double urgency) {
    pq_.push({node_id, urgency});
}

PQEntry PriorityQueueScheduler::pop() {
    if (pq_.empty()) throw std::runtime_error("PQ empty");
    auto e = pq_.top(); pq_.pop();
    return e;
}

bool PriorityQueueScheduler::empty() const { return pq_.empty(); }
int  PriorityQueueScheduler::size()  const { return (int)pq_.size(); }

} // namespace CloudInfra
