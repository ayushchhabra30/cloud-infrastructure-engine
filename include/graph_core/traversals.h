#pragma once
/*
 * PERSON 1 — Graph Foundations & Connectivity
 * Algorithms: BFS, DFS, Kosaraju SCC, Union-Find, Topological Sort
 */
#include <vector>
#include <queue>
#include <stack>
#include <functional>
#include <unordered_map>
#include <string>

namespace CloudInfra {

// ──────────────────────────────────────────────────────────────────
// Generic adjacency-list graph used by all Person-1 algorithms
// ──────────────────────────────────────────────────────────────────
struct Graph {
    int V;  // number of vertices
    std::vector<std::vector<std::pair<int,double>>> adj;   // adj[u] = {(v, weight)}
    std::vector<std::vector<std::pair<int,double>>> radj;  // reversed graph for Kosaraju

    explicit Graph(int V) : V(V), adj(V), radj(V) {}

    void addEdge(int u, int v, double w = 1.0, bool directed = true) {
        adj[u].push_back({v, w});
        radj[v].push_back({u, w});
        if (!directed) {
            adj[v].push_back({u, w});
            radj[u].push_back({v, w});
        }
    }
};

// ──────────────────────────────────────────────────────────────────
// Algorithm 1 — Breadth First Search
// Returns: BFS traversal order starting from `src`
//          distances[v] = hop-count from src (-1 = unreachable)
// ──────────────────────────────────────────────────────────────────
struct BFSResult {
    std::vector<int> order;
    std::vector<int> dist;
    std::vector<int> parent;
};

BFSResult bfs(const Graph& g, int src);

// ──────────────────────────────────────────────────────────────────
// Algorithm 2 — Depth First Search
// Returns: DFS finish order, discovery/finish times, component IDs
// ──────────────────────────────────────────────────────────────────
struct DFSResult {
    std::vector<int> order;          // post-order (finish order)
    std::vector<int> disc;           // discovery time
    std::vector<int> finish;         // finish time
    std::vector<int> component;      // component id per vertex
    int num_components;
};

DFSResult dfs(const Graph& g, int src = 0);

// ──────────────────────────────────────────────────────────────────
// Algorithm 3 — Strongly Connected Components (Kosaraju's)
// Returns: scc_id[v] = which SCC vertex v belongs to
//          sccs[i]   = list of vertices in SCC i
// ──────────────────────────────────────────────────────────────────
struct SCCResult {
    std::vector<int>              scc_id;
    std::vector<std::vector<int>> sccs;
    int num_sccs;
};

SCCResult kosaraju(const Graph& g);

// ──────────────────────────────────────────────────────────────────
// Algorithm 4 — Union-Find (Disjoint Set Union) with Path Compression
//               & Union-by-Rank
// ──────────────────────────────────────────────────────────────────
class UnionFind {
public:
    std::vector<int> parent, rank_;
    int components;

    explicit UnionFind(int n);
    int  find(int x);
    bool unite(int x, int y);   // returns true if actually merged
    bool connected(int x, int y);
    int  count() const { return components; }
};

// ──────────────────────────────────────────────────────────────────
// Algorithm 5 — Topological Sort (Kahn's BFS-based)
// Returns: topological order (empty if cycle exists)
//          is_dag = true if no cycle detected
// ──────────────────────────────────────────────────────────────────
struct TopoResult {
    std::vector<int> order;
    bool is_dag;
};

TopoResult topologicalSort(const Graph& g);

} // namespace CloudInfra
