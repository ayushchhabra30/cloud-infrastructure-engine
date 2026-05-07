#include "../../include/graph_core/traversals.h"
#include <algorithm>
#include <stdexcept>

namespace CloudInfra {

// ═══════════════════════════════════════════════════════════════════
// Algorithm 1 — BFS
// ═══════════════════════════════════════════════════════════════════
BFSResult bfs(const Graph& g, int src) {
    BFSResult res;
    res.dist.assign(g.V, -1);
    res.parent.assign(g.V, -1);

    std::queue<int> q;
    res.dist[src] = 0;
    q.push(src);

    while (!q.empty()) {
        int u = q.front(); q.pop();
        res.order.push_back(u);
        for (auto [v, w] : g.adj[u]) {
            if (res.dist[v] == -1) {
                res.dist[v] = res.dist[u] + 1;
                res.parent[v] = u;
                q.push(v);
            }
        }
    }
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 2 — DFS (iterative to avoid stack overflow on large graphs)
// ═══════════════════════════════════════════════════════════════════
DFSResult dfs(const Graph& g, int src) {
    DFSResult res;
    res.disc.assign(g.V, -1);
    res.finish.assign(g.V, -1);
    res.component.assign(g.V, -1);
    res.num_components = 0;

    int timer = 0;

    std::function<void(int, int)> dfsVisit = [&](int u, int comp) {
        res.disc[u] = timer++;
        res.component[u] = comp;
        for (auto [v, w] : g.adj[u]) {
            if (res.disc[v] == -1) {
                dfsVisit(v, comp);
            }
        }
        res.finish[u] = timer++;
        res.order.push_back(u);
    };

    // start from `src`, then sweep remaining
    if (res.disc[src] == -1) {
        dfsVisit(src, res.num_components++);
    }
    for (int i = 0; i < g.V; ++i) {
        if (res.disc[i] == -1) {
            dfsVisit(i, res.num_components++);
        }
    }
    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 3 — Kosaraju's SCC
// ═══════════════════════════════════════════════════════════════════
SCCResult kosaraju(const Graph& g) {
    SCCResult res;
    res.scc_id.assign(g.V, -1);
    res.num_sccs = 0;

    // Pass 1: get finish-order via DFS on original graph
    std::vector<bool> visited(g.V, false);
    std::stack<int>   finish_stack;

    std::function<void(int)> dfs1 = [&](int u) {
        visited[u] = true;
        for (auto [v, w] : g.adj[u]) {
            if (!visited[v]) dfs1(v);
        }
        finish_stack.push(u);
    };

    for (int i = 0; i < g.V; ++i)
        if (!visited[i]) dfs1(i);

    // Pass 2: DFS on reversed graph in finish-order
    std::function<void(int, int)> dfs2 = [&](int u, int id) {
        res.scc_id[u] = id;
        for (auto [v, w] : g.radj[u]) {
            if (res.scc_id[v] == -1) dfs2(v, id);
        }
    };

    while (!finish_stack.empty()) {
        int u = finish_stack.top(); finish_stack.pop();
        if (res.scc_id[u] == -1) {
            res.sccs.push_back({});
            dfs2(u, res.num_sccs++);
        }
    }

    // Populate sccs lists
    res.sccs.resize(res.num_sccs);
    for (int v = 0; v < g.V; ++v)
        res.sccs[res.scc_id[v]].push_back(v);

    return res;
}

// ═══════════════════════════════════════════════════════════════════
// Algorithm 4 — Union-Find with Path Compression + Union-by-Rank
// ═══════════════════════════════════════════════════════════════════
UnionFind::UnionFind(int n) : parent(n), rank_(n, 0), components(n) {
    for (int i = 0; i < n; ++i) parent[i] = i;
}

int UnionFind::find(int x) {
    if (parent[x] != x) parent[x] = find(parent[x]);  // path compression
    return parent[x];
}

bool UnionFind::unite(int x, int y) {
    int px = find(x), py = find(y);
    if (px == py) return false;
    if (rank_[px] < rank_[py]) std::swap(px, py);
    parent[py] = px;
    if (rank_[px] == rank_[py]) rank_[px]++;
    --components;
    return true;
}

bool UnionFind::connected(int x, int y) { return find(x) == find(y); }

// ═══════════════════════════════════════════════════════════════════
// Algorithm 5 — Topological Sort (Kahn's BFS)
// ═══════════════════════════════════════════════════════════════════
TopoResult topologicalSort(const Graph& g) {
    TopoResult res;
    std::vector<int> indegree(g.V, 0);
    for (int u = 0; u < g.V; ++u)
        for (auto [v, w] : g.adj[u]) ++indegree[v];

    std::queue<int> q;
    for (int i = 0; i < g.V; ++i)
        if (indegree[i] == 0) q.push(i);

    while (!q.empty()) {
        int u = q.front(); q.pop();
        res.order.push_back(u);
        for (auto [v, w] : g.adj[u]) {
            if (--indegree[v] == 0) q.push(v);
        }
    }

    res.is_dag = ((int)res.order.size() == g.V);
    return res;
}

} // namespace CloudInfra
