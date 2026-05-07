/*
 * tests/test_routing.cpp
 * Unit tests for Person 2 algorithms (Dijkstra, BF, FW, A*, PQ)
 */
#include <iostream>
#include <cassert>
#include <cmath>
#include "../include/routing/dijkstra.h"

using namespace CloudInfra;

static int passed = 0, failed = 0;
#define TEST(name, cond) \
    do { if (cond) { std::cout << "  [PASS] " << name << "\n"; ++passed; } \
         else      { std::cout << "  [FAIL] " << name << "\n"; ++failed; } } while(0)

static bool approx(double a, double b, double eps = 1e-6) {
    return std::abs(a - b) < eps;
}

static Graph buildG() {
    Graph g(5);
    g.addEdge(0,1,4.0,true); g.addEdge(0,2,1.0,true);
    g.addEdge(2,1,2.0,true); g.addEdge(1,3,1.0,true);
    g.addEdge(2,3,5.0,true); g.addEdge(3,4,3.0,true);
    return g;
}

void testDijkstra() {
    std::cout << "\n── Dijkstra Tests ──\n";
    auto g = buildG();
    auto r = dijkstra(g, 0);
    TEST("dist[0]=0",   approx(r.dist[0], 0.0));
    TEST("dist[1]=3",   approx(r.dist[1], 3.0));  // 0→2→1
    TEST("dist[2]=1",   approx(r.dist[2], 1.0));
    TEST("dist[3]=4",   approx(r.dist[3], 4.0));
    TEST("dist[4]=7",   approx(r.dist[4], 7.0));

    auto path = dijkstraPath(g, 0, 4);
    TEST("Path found",           path.found);
    TEST("Path starts at 0",     !path.path.empty() && path.path.front() == 0);
    TEST("Path ends at 4",       !path.path.empty() && path.path.back()  == 4);
    TEST("Path cost = 7",        approx(path.cost, 7.0));
}

void testBellmanFord() {
    std::cout << "\n── Bellman-Ford Tests ──\n";
    auto g = buildG();
    auto r = bellmanFord(g, 0);
    TEST("BF dist[1]=3",  approx(r.dist[1], 3.0));
    TEST("BF dist[4]=7",  approx(r.dist[4], 7.0));
    TEST("No neg cycle",  !r.has_negative_cycle);

    auto p = bellmanFordPath(g, 0, 3);
    TEST("BF path found",  p.found);
    TEST("BF path cost=4", approx(p.cost, 4.0));
}

void testFloydWarshall() {
    std::cout << "\n── Floyd-Warshall Tests ──\n";
    auto g = buildG();
    auto fw = floydWarshall(g);
    TEST("FW [0][4]=7",  approx(fw.dist[0][4], 7.0));
    TEST("FW [0][0]=0",  approx(fw.dist[0][0], 0.0));
    TEST("FW [1][4]=4",  approx(fw.dist[1][4], 4.0));

    auto p = fwPath(fw, 0, 4);
    TEST("FW path found",   p.found);
    TEST("FW path start=0", p.path.front() == 0);
    TEST("FW path end=4",   p.path.back()  == 4);
}

void testAStar() {
    std::cout << "\n── A* Tests ──\n";
    auto g = buildG();
    auto p = aStar(g, 0, 4);  // zero heuristic = Dijkstra
    TEST("A* path found",   p.found);
    TEST("A* cost=7",       approx(p.cost, 7.0));
    TEST("A* path valid",   p.path.front()==0 && p.path.back()==4);
}

void testPriorityQueue() {
    std::cout << "\n── Priority Queue Scheduler Tests ──\n";
    PriorityQueueScheduler pqs;
    TEST("Initially empty", pqs.empty());
    pqs.push(1, 0.3);
    pqs.push(2, 0.9);
    pqs.push(3, 0.6);
    TEST("Size=3", pqs.size() == 3);
    auto top = pqs.pop();
    TEST("Top is highest urgency (node 2)", top.node_id == 2);
    TEST("Size=2 after pop", pqs.size() == 2);
}

int main() {
    std::cout << "╔══════════════════════════════════╗\n";
    std::cout << "║   Test Suite: Routing (P2)       ║\n";
    std::cout << "╚══════════════════════════════════╝\n";
    testDijkstra();
    testBellmanFord();
    testFloydWarshall();
    testAStar();
    testPriorityQueue();
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed.\n";
    return failed > 0 ? 1 : 0;
}
