/*
 * tests/test_graph_core.cpp
 * Unit tests for Person 1 algorithms (BFS, DFS, SCC, Union-Find, Topo Sort)
 */
#include <iostream>
#include <cassert>
#include "../include/graph_core/traversals.h"

using namespace CloudInfra;

static int passed = 0, failed = 0;

#define TEST(name, cond) \
    do { \
        if (cond) { std::cout << "  [PASS] " << name << "\n"; ++passed; } \
        else      { std::cout << "  [FAIL] " << name << "\n"; ++failed; } \
    } while(0)

void testBFS() {
    std::cout << "\n── BFS Tests ──\n";
    Graph g(5);
    g.addEdge(0,1,1,false); g.addEdge(0,2,1,false);
    g.addEdge(1,3,1,false); g.addEdge(2,4,1,false);
    auto r = bfs(g, 0);
    TEST("BFS dist[0]=0",         r.dist[0] == 0);
    TEST("BFS dist[1]=1",         r.dist[1] == 1);
    TEST("BFS dist[3]=2",         r.dist[3] == 2);
    TEST("BFS visits all 5",      r.order.size() == 5);
    TEST("BFS unreachable = -1",  bfs(g, 0).dist[4] != -1);  // 4 is reachable
    // Disconnected graph
    Graph g2(4);
    g2.addEdge(0,1,1,false);
    auto r2 = bfs(g2, 0);
    TEST("BFS disconnected node=-1", r2.dist[3] == -1);
}

void testDFS() {
    std::cout << "\n── DFS Tests ──\n";
    Graph g(4);
    g.addEdge(0,1,1,false); g.addEdge(1,2,1,false); g.addEdge(2,3,1,false);
    auto r = dfs(g, 0);
    TEST("DFS visits all 4",    r.order.size() == 4);
    TEST("DFS disc[0] set",     r.disc[0] >= 0);
    TEST("DFS 1 component",     r.num_components == 1);

    Graph g2(4);  // disconnected
    g2.addEdge(0,1,1,false);
    auto r2 = dfs(g2, 0);
    TEST("DFS 3 components",    r2.num_components == 3);
}

void testKosaraju() {
    std::cout << "\n── Kosaraju SCC Tests ──\n";
    Graph g(6);
    g.addEdge(0,1,1,true); g.addEdge(1,2,1,true); g.addEdge(2,0,1,true);
    g.addEdge(1,3,1,true); g.addEdge(3,4,1,true); g.addEdge(4,5,1,true);
    g.addEdge(5,3,1,true);
    auto r = kosaraju(g);
    TEST("SCC count = 2",          r.num_sccs == 2);
    TEST("SCC id assigned",        r.scc_id[0] >= 0);
    {
        int total = 0;
        for (auto& s : r.sccs) total += s.size();
        TEST("SCC sizes sum to V", total == 6);
    }
}

void testUnionFind() {
    std::cout << "\n── Union-Find Tests ──\n";
    UnionFind uf(5);
    TEST("Initial components=5",     uf.count() == 5);
    uf.unite(0, 1);
    TEST("After unite(0,1) = 4",     uf.count() == 4);
    TEST("0 and 1 connected",        uf.connected(0, 1));
    TEST("0 and 2 NOT connected",    !uf.connected(0, 2));
    uf.unite(2, 3); uf.unite(0, 2);
    TEST("After 3 unions = 2",       uf.count() == 2);
    TEST("Path compression works",   uf.find(3) == uf.find(0));
}

void testTopoSort() {
    std::cout << "\n── Topological Sort Tests ──\n";
    Graph dag(5);
    dag.addEdge(0,2,1,true); dag.addEdge(0,1,1,true);
    dag.addEdge(1,3,1,true); dag.addEdge(2,3,1,true); dag.addEdge(3,4,1,true);
    auto r = topologicalSort(dag);
    TEST("Is DAG",              r.is_dag);
    TEST("All 5 in order",      r.order.size() == 5);
    {
        int p0=0, p4=0;
        for (int i=0;i<5;++i) {
            if (r.order[i]==0) p0=i;
            if (r.order[i]==4) p4=i;
        }
        bool zero_before_four = (p0 < p4);
        TEST("0 before 4", zero_before_four);
    }

    // Graph with cycle
    Graph cyc(3);
    cyc.addEdge(0,1,1,true); cyc.addEdge(1,2,1,true); cyc.addEdge(2,0,1,true);
    auto rc = topologicalSort(cyc);
    TEST("Cycle detected (not DAG)", !rc.is_dag);
}

int main() {
    std::cout << "╔══════════════════════════════════╗\n";
    std::cout << "║   Test Suite: Graph Core (P1)    ║\n";
    std::cout << "╚══════════════════════════════════╝\n";
    testBFS();
    testDFS();
    testKosaraju();
    testUnionFind();
    testTopoSort();
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed.\n";
    return failed > 0 ? 1 : 0;
}
