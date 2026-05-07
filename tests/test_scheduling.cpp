/*
 * tests/test_scheduling.cpp
 * Unit tests for Person 4 algorithms (Knapsack, BinPacking, JobSeq,
 *                                      Interval, Hungarian, MCF)
 */
#include <iostream>
#include <cassert>
#include <cmath>
#include "../include/scheduling/knapsack_allocator.h"

using namespace CloudInfra;

static int passed = 0, failed = 0;
#define TEST(name, cond) \
    do { if (cond) { std::cout << "  [PASS] " << name << "\n"; ++passed; } \
         else      { std::cout << "  [FAIL] " << name << "\n"; ++failed; } } while(0)

static bool approx(double a, double b, double eps = 1e-6) {
    return std::abs(a - b) < eps;
}

void testKnapsack() {
    std::cout << "\n── Knapsack Tests ──\n";
    std::vector<KnapsackItem> items = {{1,2,6},{2,2,10},{3,3,12}};
    auto r = knapsack01(items, 5);
    TEST("Max value = 22",    approx(r.total_value, 22.0));
    TEST("Weight <= 5",       r.total_weight <= 5);

    // Single item
    std::vector<KnapsackItem> single = {{1,1,100}};
    auto rs = knapsack01(single, 1);
    TEST("Single item selected", rs.total_value == 100.0);

    // Capacity 0
    auto r0 = knapsack01(items, 0);
    TEST("Zero capacity => value=0", approx(r0.total_value, 0.0));
}

void testBinPacking() {
    std::cout << "\n── Bin Packing Tests ──\n";
    std::vector<double> sizes = {0.5, 0.5, 0.5, 0.5};
    auto r = binPackingFFD(sizes, 1.0);
    TEST("4 halves fit in 2 bins", r.num_bins == 2);

    std::vector<double> s2 = {0.9, 0.9, 0.9};
    auto r2 = binPackingFFD(s2, 1.0);
    TEST("3 big items need 3 bins", r2.num_bins == 3);
}

void testJobSequencing() {
    std::cout << "\n── Job Sequencing Tests ──\n";
    std::vector<ScheduledJob> jobs = {{1,2,100},{2,1,50},{3,2,200},{4,1,75}};
    auto r = jobSequencing(jobs);
    TEST("Max profit >= 275", r.total_profit >= 275.0);
    TEST("Sequence non-empty", !r.sequence.empty());
}

void testIntervalScheduling() {
    std::cout << "\n── Interval Scheduling Tests ──\n";
    std::vector<Interval> ivs = {{1,1,3},{2,2,5},{3,4,6},{4,7,9},{5,8,10}};
    auto sel = intervalScheduling(ivs);
    TEST("At least 2 selected",        sel.size() >= 2);
    // Verify no overlaps
    std::vector<Interval> byId;
    for (int id : sel) {
        for (auto& iv : ivs) if (iv.id == id) { byId.push_back(iv); break; }
    }
    bool no_overlap = true;
    for (int i = 0; i + 1 < (int)byId.size(); ++i)
        if (byId[i].finish > byId[i+1].start) { no_overlap = false; break; }
    TEST("No overlaps in selection", no_overlap);
}

void testHungarian() {
    std::cout << "\n── Hungarian Algorithm Tests ──\n";
    // Known minimum: 0→1(2), 1→2(3), 2→3(8), 3→0(7) => sum=20? Let's use a clear example
    std::vector<std::vector<double>> cost = {
        {4, 1, 3},
        {2, 0, 5},
        {3, 2, 2}
    };
    auto r = hungarian(cost);
    TEST("Assignment size = 3",   r.assignment.size() == 3);
    TEST("Total cost > 0",        r.total_cost > 0);
    // Each task assigned to distinct node
    std::vector<bool> used(3, false);
    bool distinct = true;
    for (int a : r.assignment) {
        if (a < 0 || a >= 3 || used[a]) { distinct = false; break; }
        used[a] = true;
    }
    TEST("All distinct assignments", distinct);
}

void testMinCostFlow() {
    std::cout << "\n── Min Cost Flow Tests ──\n";
    MinCostFlow mcf(4);
    mcf.addEdge(0, 1, 2, 1);
    mcf.addEdge(0, 2, 2, 3);
    mcf.addEdge(1, 3, 2, 2);
    mcf.addEdge(2, 3, 2, 1);
    auto r = mcf.solve(0, 3, 3);
    TEST("Flow = 3",    r.max_flow == 3);
    TEST("Feasible",    r.feasible);
    TEST("Cost > 0",    r.min_cost > 0);

    // Infeasible: request more than capacity
    MinCostFlow mcf2(2);
    mcf2.addEdge(0, 1, 1, 1);
    auto r2 = mcf2.solve(0, 1, 5);
    TEST("Infeasible detected", !r2.feasible);
}

int main() {
    std::cout << "╔══════════════════════════════════╗\n";
    std::cout << "║   Test Suite: Scheduling (P4)    ║\n";
    std::cout << "╚══════════════════════════════════╝\n";
    testKnapsack();
    testBinPacking();
    testJobSequencing();
    testIntervalScheduling();
    testHungarian();
    testMinCostFlow();
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed.\n";
    return failed > 0 ? 1 : 0;
}
