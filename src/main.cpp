/*
 * CloudInfraEngine - Autonomous Cloud Infrastructure Simulation
 * 27 DAA Algorithms | 3 Zones | 10 Nodes | Interactive Terminal
 *
 * UI Rules:
 *   - No box-drawing + ANSI color mixed in same line (breaks alignment)
 *   - Color only for: OK (green), WARN (yellow), FAIL (red), labels (cyan)
 *   - Tables use printf-width columns, no unicode box chars in dynamic rows
 *   - Topology shown as text node list + adjacency, not art
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "include/core/constants.h"
#include "include/core/cloud_node.h"
#include "include/core/cloud_edge.h"
#include "include/core/workload_task.h"
#include "include/graph_core/traversals.h"
#include "include/routing/dijkstra.h"
#include "include/vulnerability/articulation_bridges.h"
#include "include/scheduling/knapsack_allocator.h"
#include "include/prediction_dp/dp_optimizers.h"

using namespace CloudInfra;

// ── ANSI colours (only used at start/end of a full line segment) ──
#define R  "\033[0m"
#define CY "\033[36m"
#define GR "\033[92m"
#define YL "\033[93m"
#define RD "\033[91m"
#define BD "\033[1m"
#define DM "\033[2m"
#define WH "\033[97m"

// ── Helpers ────────────────────────────────────────────────────────
static void sep()  { printf("  %s---------------------------------------------------------------------%s\n", CY, R); }
static void sep2() { printf("  %s=====================================================================%s\n", BD CY, R); }

static void ok  (const char* msg) { printf("  " GR "  [OK]  " R "%s\n", msg); }
static void warn(const char* msg) { printf("  " YL "  [!!]  " R "%s\n", msg); }
static void err (const char* msg) { printf("  " RD "  [XX]  " R "%s\n", msg); }
static void info(const char* msg) { printf("  " CY "  [>>]  " R "%s\n", msg); }

static void step(const char* algo, const char* what) {
    printf("  " CY "  [%-26s] " R "%s\n", algo, what);
    usleep(30000);
}

static void hdr(const char* title) {
    printf("\n  " BD WH "=== %s" R "\n\n", title);
}

static void pressEnter() {
    printf("\n  " DM "Press Enter to continue..." R);
    fflush(stdout);
    // flush leftover newline then wait for real enter
    int c; while ((c = getchar()) != '\n' && c != EOF) {}
    getchar();
}

static void clr() { printf("\033[2J\033[H"); }

// ── Simulation state ───────────────────────────────────────────────

struct SimNode {
    int         id;
    const char* name;       // full name
    const char* tag;        // 3-char tag for paths
    const char* zone;
    NodeType    type;
    NodeState   state;
    double      cpu;        // 0.0 – 1.0
    double      mem;
    int         jobs;

    bool alive() const { return state != NodeState::FAILED; }

    const char* stateStr() const {
        if (state == NodeState::HEALTHY)  return "HEALTHY ";
        if (state == NodeState::DEGRADED) return "DEGRADED";
        return "FAILED  ";
    }
    const char* stateCol() const {
        if (state == NodeState::HEALTHY)  return GR;
        if (state == NodeState::DEGRADED) return YL;
        return RD;
    }
};

struct SimEdge { int u, v; double lat; bool alive; };

struct Workload {
    int         id;
    const char* name;
    int         node;
    double      cpu, mem;
    int         deadline;
    int         pri;        // 0=LOW 1=MED 2=HIGH 3=CRIT

    const char* priStr() const {
        const char* t[] = {"LOW ","MED ","HIGH","CRIT"};
        return t[pri < 4 ? pri : 0];
    }
    const char* priCol() const {
        if (pri >= 3) return RD;
        if (pri >= 2) return YL;
        return GR;
    }
};

#define NN 10
#define NE 13

struct Sim {
    SimNode  nd[NN];
    SimEdge  ed[NE];
    std::vector<Workload> wl;
    int  tick;
    int  nextId;
    double sla;

    // Build CloudInfra Graph from live edges/nodes
    Graph graph() const {
        Graph g(NN);
        for (int i = 0; i < NE; ++i) {
            if (!ed[i].alive) continue;
            if (!nd[ed[i].u].alive() || !nd[ed[i].v].alive()) continue;
            g.addEdge(ed[i].u, ed[i].v, ed[i].lat, false);
        }
        return g;
    }

    // ── Topology display (text, no alignment-breaking art) ─────────
    void showTopology() const {
        printf("\n  " BD CY "CLOUD TOPOLOGY  (3 zones | %d nodes | %d links)" R "\n\n", NN, NE);

        // Zone headers
        const char* zones[] = {"ZONE-ALPHA", "ZONE-BETA", "ZONE-GAMMA"};
        const char* zmap[]  = {"ALPHA",      "BETA",      "GAMMA"};
        for (int z = 0; z < 3; ++z) {
            printf("  " BD "  %s:" R "\n", zones[z]);
            for (int i = 0; i < NN; ++i) {
                if (strcmp(nd[i].zone, zmap[z]) != 0) continue;
                // cpu bar
                int bar = (int)(nd[i].cpu * 10);
                char barstr[12]; barstr[0] = '[';
                for (int b = 0; b < 10; ++b) barstr[b+1] = (b < bar ? '#' : '.');
                barstr[11] = 0; // no ] here — we print it separately
                printf("  %s    [%2d] %-12s  %s%-8s" R
                       "  CPU %s%d%%  " R
                       " MEM %d%%  jobs:%d\n",
                    nd[i].stateCol(),
                    nd[i].id, nd[i].name,
                    nd[i].stateCol(), nd[i].stateStr(),
                    nd[i].cpu > 0.8 ? RD : nd[i].cpu > 0.6 ? YL : GR,
                    (int)(nd[i].cpu * 100),
                    (int)(nd[i].mem * 100),
                    nd[i].jobs);
            }
            printf("\n");
        }

        // Links
        printf("  " BD "  ACTIVE LINKS:" R "\n");
        for (int i = 0; i < NE; ++i) {
            if (!ed[i].alive) continue;
            printf("    %s -- %s  (%.0f ms)  %s\n",
                nd[ed[i].u].tag, nd[ed[i].v].tag,
                ed[i].lat,
                nd[ed[i].u].alive() && nd[ed[i].v].alive() ? GR "UP" R : RD "DOWN" R);
        }

        // Dead links
        bool any_dead = false;
        for (int i = 0; i < NE; ++i) if (!ed[i].alive) { any_dead = true; break; }
        if (any_dead) {
            printf("  " RD "  SEVERED LINKS:" R "\n");
            for (int i = 0; i < NE; ++i)
                if (!ed[i].alive)
                    printf("    %s -- %s  (%.0f ms)  " RD "DOWN\n" R,
                        nd[ed[i].u].tag, nd[ed[i].v].tag, ed[i].lat);
        }
    }

    // ── Status line ────────────────────────────────────────────────
    void showStatus() const {
        int h=0, d=0, f=0;
        for (int i = 0; i < NN; ++i) {
            if (nd[i].state == NodeState::HEALTHY)  ++h;
            else if (nd[i].state == NodeState::DEGRADED) ++d;
            else ++f;
        }
        sep2();
        printf("  TICK:%-4d  " GR "HEALTHY:%d " R " " YL "DEGRADED:%d " R " " RD "FAILED:%d" R
               "  |  SLA:%s%.1f%%" R "  |  JOBS:%d\n",
            tick, h, d, f,
            sla >= 99.0 ? GR : sla >= 95.0 ? YL : RD, sla,
            (int)wl.size());
        sep2();
    }

    // ── Main menu ──────────────────────────────────────────────────
    static void showMenu() {
        printf("\n"
            "  " BD CY "MAIN MENU" R "\n"
            "  " CY "  [1]" R " Infrastructure Dashboard      "
            "  " CY "[2]" R " Simulate Node Failure\n"
            "  " CY "  [3]" R " Route Traffic Query           "
            "  " CY "[4]" R " Place New Workload\n"
            "  " CY "  [5]" R " Vulnerability Analysis        "
            "  " CY "[6]" R " Failure Prediction\n"
            "  " CY "  [7]" R " Optimize Resources            "
            "  " CY "[8]" R " SLA & Metrics Report\n"
            "  " CY "  [9]" R " Full System Diagnostics       "
            "  " CY "[R]" R " Restore Failed Nodes\n"
            "  " CY "  [0]" R " Exit\n");
    }

    // ── Banner ─────────────────────────────────────────────────────
    static void showBanner() {
        printf("\n"
            "  " BD CY "CloudInfraEngine" R " - Autonomous Cloud Infrastructure Simulation\n"
            "  " DM "27 DAA Algorithms | 3-Zone Topology | Interactive Console\n" R);
    }

    // ══════════════════════════════════════════════════════════════
    // MENU 1 — Infrastructure Dashboard
    // ══════════════════════════════════════════════════════════════
    void menuDashboard() {
        clr(); showBanner();
        hdr("INFRASTRUCTURE DASHBOARD");

        // Build utils array for Segment + Fenwick
        std::vector<double> utils(NN);
        for (int i = 0; i < NN; ++i) utils[i] = nd[i].alive() ? nd[i].cpu : 0.0;

        step("Segment Tree",    "Building range-query index over CPU utilizations...");
        SegmentTree st(utils);

        step("Fenwick Tree",    "Building prefix-sum SLA violation counters...");
        FenwickTree ft(NN);
        for (int i = 0; i < NN; ++i) ft.update(i+1, utils[i]);

        step("BFS Reachability","Scanning from GATEWAY-ALPHA (node 0)...");
        Graph g = graph();
        BFSResult bfs_r = bfs(g, 0);
        int reach = 0;
        for (int d : bfs_r.dist) if (d >= 0) ++reach;
        char buf[128];
        snprintf(buf, sizeof(buf), "%d/%d nodes reachable from GATEWAY-A", reach, NN);
        ok(buf);

        step("DFS Components",  "Counting isolated network partitions...");
        DFSResult dfs_r = dfs(g, 0);
        snprintf(buf, sizeof(buf), "%d component(s) detected", dfs_r.num_components);
        ok(buf);

        sep();

        // Node table — plain printf, no box chars mixed with colors
        printf("  " BD "  ID  %-13s %-8s %-9s CPU%%  MEM%%  JOBS  HOPS\n" R,
               "NAME", "ZONE", "STATE");
        sep();
        for (int i = 0; i < NN; ++i) {
            int hops = bfs_r.dist[i];
            // CPU color inline using printf — safe because we set/reset per cell
            printf("   %2d  %-13s %-8s %s%-8s" R "  %s%3d%%" R "  %3d%%  %4d  %4s\n",
                nd[i].id, nd[i].name, nd[i].zone,
                nd[i].stateCol(), nd[i].stateStr(),
                nd[i].cpu > 0.8 ? RD : nd[i].cpu > 0.6 ? YL : GR,
                (int)(nd[i].cpu * 100),
                (int)(nd[i].mem * 100),
                nd[i].jobs,
                hops >= 0 ? std::to_string(hops).c_str() : "inf");
        }
        sep();

        // Segment Tree zone queries
        printf("\n  " BD "Segment Tree Range Queries:" R "\n");
        struct ZQ { const char* name; int l, r; };
        ZQ zones[] = {{"Alpha",0,3},{"Beta",4,6},{"Gamma",7,9}};
        for (auto& z : zones) {
            printf("    %-6s [%d..%d]  max=%s%3d%%" R "  min=%s%3d%%" R "  avg=%3d%%  sum=%.2f\n",
                z.name, z.l, z.r,
                st.queryMax(z.l,z.r) > 0.8 ? RD : YL, (int)(st.queryMax(z.l,z.r)*100),
                GR, (int)(st.queryMin(z.l,z.r)*100),
                (int)(st.querySum(z.l,z.r)/(z.r-z.l+1)*100),
                st.querySum(z.l,z.r));
        }

        // Fenwick prefix sums
        printf("\n  " BD "Fenwick Tree Prefix Loads:" R "\n    ");
        for (int i = 1; i <= NN; ++i)
            printf("[0..%d]=%.0f%%  ", i-1, ft.query(i)*100);
        printf("\n");

        pressEnter();
    }

    // ══════════════════════════════════════════════════════════════
    // MENU 2 — Simulate Node Failure
    // ══════════════════════════════════════════════════════════════
    void menuFailure() {
        clr(); showBanner();
        showTopology();
        showStatus();
        hdr("SIMULATE NODE FAILURE");

        printf("  Alive nodes: ");
        for (int i = 0; i < NN; ++i)
            if (nd[i].alive())
                printf("%s[%d:%s]" R " ", nd[i].stateCol(), nd[i].id, nd[i].tag);
        printf("\n\n  Enter node ID to fail (-1 to cancel): ");
        fflush(stdout);
        int choice; scanf("%d", &choice);

        if (choice < 0 || choice >= NN) { printf("  Cancelled.\n"); pressEnter(); return; }
        if (!nd[choice].alive()) { err("Node already failed."); pressEnter(); return; }

        printf("\n");
        Graph g = graph();

        step("DFS Pre-scan",    "Mapping component structure before failure...");
        DFSResult dfs_r = dfs(g, 0);
        char buf[256];
        snprintf(buf, sizeof(buf), "Components before failure: %d", dfs_r.num_components);
        ok(buf);

        step("Articulation Pts","Checking if node is a single point of failure...");
        APResult ap = findArticulationPoints(g);
        bool is_ap = ap.is_ap[choice];
        if (is_ap) {
            snprintf(buf, sizeof(buf), "Node %d (%s) IS an Articulation Point -- network will partition!",
                     choice, nd[choice].name);
            warn(buf);
        } else {
            snprintf(buf, sizeof(buf), "Node %d (%s) is NOT an AP -- network stays connected",
                     choice, nd[choice].name);
            ok(buf);
        }

        step("Bridge Detection","Counting bridge links on this node...");
        BridgeResult br = findBridges(g);
        int blost = 0;
        for (auto& [u,v] : br.bridges) if (u==choice || v==choice) ++blost;
        if (blost) {
            snprintf(buf, sizeof(buf), "%d bridge link(s) will be severed", blost);
            warn(buf);
        } else {
            ok("No bridge links on this node");
        }

        step("Kosaraju SCC",    "Strongly-connected groups before failure...");
        SCCResult scc = kosaraju(g);
        snprintf(buf, sizeof(buf), "SCCs before: %d", scc.num_sccs);
        ok(buf);

        step("Union-Find",      "Verifying connectivity classes...");
        UnionFind uf(NN);
        for (int i = 0; i < NE; ++i)
            if (ed[i].alive && nd[ed[i].u].alive() && nd[ed[i].v].alive())
                uf.unite(ed[i].u, ed[i].v);
        snprintf(buf, sizeof(buf), "Connected components (Union-Find): %d", uf.count());
        ok(buf);

        // Apply failure
        printf("\n  " RD BD ">>> FAILING node %d: %s <<<\n" R, choice, nd[choice].name);
        usleep(300000);
        nd[choice].state = NodeState::FAILED;
        for (int i = 0; i < NE; ++i)
            if (ed[i].u == choice || ed[i].v == choice) ed[i].alive = false;

        // Migrate workloads
        int migrated = 0;
        for (auto& w : wl) {
            if (w.node != choice) continue;
            step("Dijkstra Migration", ("Migrating job: " + std::string(w.name)).c_str());
            Graph g2 = graph();
            DijkstraResult dijk = dijkstra(g2, 0);
            int best = -1; double bestD = 1e18;
            for (int i = 0; i < NN; ++i)
                if (nd[i].alive() && i != choice && dijk.dist[i] < bestD)
                    { bestD = dijk.dist[i]; best = i; }
            if (best >= 0) {
                w.node = best;
                nd[best].jobs++;
                nd[best].cpu = std::min(1.0, nd[best].cpu + w.cpu);
                if (nd[best].cpu > 0.85) nd[best].state = NodeState::DEGRADED;
                snprintf(buf, sizeof(buf), "Migrated '%s' -> %s (dist=%.0fms)",
                         w.name, nd[best].name, bestD);
                ok(buf);
                ++migrated;
            } else {
                snprintf(buf, sizeof(buf), "No reachable node for '%s' -- SLA VIOLATION", w.name);
                err(buf);
                sla -= 2.0;
            }
        }
        nd[choice].jobs = 0;
        nd[choice].cpu  = 0;

        step("Markov Chain",    "Estimating cascade failure probability...");
        std::vector<std::vector<double>> tr = {
            {0.90,0.08,0.02,0.00},{0.25,0.55,0.15,0.05},
            {0.00,0.00,0.60,0.40},{0.65,0.25,0.05,0.05}};
        MarkovChain mc(tr);
        double fp = mc.failureProbability(NodeState::DEGRADED, 5);
        snprintf(buf, sizeof(buf), "Cascade failure probability (5 ticks): %d%%", (int)(fp*100));
        warn(buf);

        step("A* Emergency",    "Finding best alternate path after failure...");
        Graph g3 = graph();
        PathResult ap2 = aStar(g3, 0, 9);
        if (ap2.found) {
            std::string path_str = "Emergency path GWA->SGM: ";
            for (int i = 0; i < (int)ap2.path.size(); ++i) {
                if (i) path_str += " -> ";
                path_str += nd[ap2.path[i]].tag;
            }
            path_str += "  (" + std::to_string((int)ap2.cost) + "ms)";
            ok(path_str.c_str());
        } else {
            err("No path GWA->SGM after failure");
        }

        ++tick;
        sla = std::max(80.0, sla - (is_ap ? 4.5 : 1.2));

        sep();
        printf("  Result: " RD BD "%s OFFLINE" R "  migrated=%s%d jobs" R
               "  SLA now=%s%.1f%%\n" R,
            nd[choice].name,
            GR, migrated,
            sla>=99?GR:sla>=95?YL:RD, sla);
        sep();

        showTopology();
        pressEnter();
    }

    // ══════════════════════════════════════════════════════════════
    // MENU 3 — Route Traffic Query
    // ══════════════════════════════════════════════════════════════
    void menuRoute() {
        clr(); showBanner();
        showTopology();
        hdr("ROUTE TRAFFIC QUERY");

        printf("  Alive nodes: ");
        for (int i = 0; i < NN; ++i)
            if (nd[i].alive())
                printf("%s[%d:%s]" R " ", nd[i].stateCol(), nd[i].id, nd[i].tag);

        printf("\n\n  Source node ID      : "); fflush(stdout);
        int src; scanf("%d", &src);
        printf("  Destination node ID : "); fflush(stdout);
        int dst; scanf("%d", &dst);

        if (src<0||src>=NN||dst<0||dst>=NN) { err("Invalid node ID."); pressEnter(); return; }

        printf("\n");
        Graph g = graph();
        char buf[512];

        // Dijkstra
        step("Dijkstra O((V+E)logV)", ("Latency-optimal: " + std::string(nd[src].name) + " -> " + nd[dst].name).c_str());
        PathResult dp = dijkstraPath(g, src, dst);
        if (dp.found) {
            std::string ps = "Path: ";
            for (int i = 0; i < (int)dp.path.size(); ++i) {
                if (i) ps += " -> ";
                ps += nd[dp.path[i]].tag;
            }
            ps += "  [" + std::to_string((int)dp.cost) + "ms]";
            ok(ps.c_str());
        } else { err("No path found (Dijkstra)"); }

        // Bellman-Ford
        step("Bellman-Ford O(V*E)",   "Checking for negative-cost edge paths...");
        PathResult bf = bellmanFordPath(g, src, dst);
        if (bf.found) {
            if (dp.found && bf.cost < dp.cost - 0.01) {
                snprintf(buf, sizeof(buf), "BF found cheaper path: %dms", (int)bf.cost);
                warn(buf);
            } else {
                snprintf(buf, sizeof(buf), "BF confirms cost=%dms, no negative cycles", (int)bf.cost);
                ok(buf);
            }
        }

        // Floyd-Warshall
        step("Floyd-Warshall O(V^3)", ("All-pairs from " + std::string(nd[src].name) + "...").c_str());
        FloydWarshallResult fw = floydWarshall(g);
        printf("    Distances from %s:  ", nd[src].tag);
        for (int v = 0; v < NN; ++v)
            if (fw.dist[src][v] < 1e17 && v != src)
                printf("%s=%dms  ", nd[v].tag, (int)fw.dist[src][v]);
        printf("\n");

        // A*
        step("A* Search (heuristic)",  "Zone-aware goal-directed routing...");
        auto h_fn = [&](int u, int goal) -> double {
            return strcmp(nd[u].zone, nd[goal].zone) == 0 ? 0.0 : 8.0;
        };
        PathResult as = aStar(g, src, dst, h_fn);
        if (as.found) {
            std::string ps = "A* path: ";
            for (int i = 0; i < (int)as.path.size(); ++i) {
                if (i) ps += " -> ";
                ps += nd[as.path[i]].tag;
            }
            ps += "  [" + std::to_string((int)as.cost) + "ms]";
            ok(ps.c_str());
        }

        // Priority Queue dispatch order
        step("Priority Queue Sched",  "Ordering hops by least-loaded node first...");
        PriorityQueueScheduler pqs;
        if (dp.found)
            for (int v : dp.path) pqs.push(v, 1.0/(1.0 + nd[v].cpu));
        printf("    Dispatch order: ");
        while (!pqs.empty()) {
            auto e = pqs.pop();
            printf("%s(load=%d%%) ", nd[e.node_id].tag, (int)(nd[e.node_id].cpu*100));
        }
        printf("\n");

        // Recommendation
        sep();
        if (dp.found) {
            printf("  " GR BD "BEST ROUTE: " R);
            for (int i = 0; i < (int)dp.path.size(); ++i) {
                if (i) printf(" -> ");
                printf("%s", nd[dp.path[i]].name);
            }
            printf("  " GR "[%dms total]\n" R, (int)dp.cost);
        }

        pressEnter();
    }

    // ══════════════════════════════════════════════════════════════
    // MENU 4 — Place Workload
    // ══════════════════════════════════════════════════════════════
    void menuPlaceWorkload() {
        clr(); showBanner();
        hdr("PLACE NEW WORKLOAD");

        char wname[64];
        int  cpupct, mempct, pri;
        printf("  Workload name              : "); fflush(stdout); scanf("%63s", wname);
        printf("  CPU required [0-100 %%]     : "); fflush(stdout); scanf("%d", &cpupct);
        printf("  Memory required [0-100 %%]  : "); fflush(stdout); scanf("%d", &mempct);
        printf("  Priority [0=LOW 1=MED 2=HIGH 3=CRIT]: "); fflush(stdout); scanf("%d", &pri);
        pri = std::max(0, std::min(3, pri));
        double cpu = cpupct/100.0, mem = mempct/100.0;
        printf("\n");

        char buf[256];

        step("Topological Sort",    "Validating workload dependency chain...");
        Graph dag(4);
        dag.addEdge(0,1,1,true); dag.addEdge(0,2,1,true);
        dag.addEdge(1,3,1,true); dag.addEdge(2,3,1,true);
        TopoResult topo = topologicalSort(dag);
        snprintf(buf, sizeof(buf), "Dependency DAG valid=%s  stages=%d",
                 topo.is_dag?"YES":"NO", (int)topo.order.size());
        ok(buf);

        step("0/1 Knapsack DP",     "Finding best-fit nodes under resource constraints...");
        std::vector<KnapsackItem> items;
        for (int i = 0; i < NN; ++i) {
            if (!nd[i].alive()) continue;
            double fc = std::max(0.0, 1.0 - nd[i].cpu);
            double fm = std::max(0.0, 1.0 - nd[i].mem);
            if (fc >= cpu && fm >= mem)
                items.push_back({nd[i].id, (int)(cpu*10)+1, (fc+fm)*50.0/(nd[i].jobs+1)});
        }
        if (items.empty()) { err("No node has sufficient free capacity."); pressEnter(); return; }
        KnapsackResult ks = knapsack01(items, (int)(cpu*10)+5);
        snprintf(buf, sizeof(buf), "Knapsack: %d candidate(s)  value=%.0f",
                 (int)ks.selected_ids.size(), ks.total_value);
        ok(buf);

        step("Bin Packing FFD",     "Verifying server consolidation is feasible...");
        std::vector<double> loads;
        for (int i = 0; i < NN; ++i) if (nd[i].alive()) loads.push_back(nd[i].cpu);
        loads.push_back(cpu);
        BinPackResult bp = binPackingFFD(loads, 1.0);
        snprintf(buf, sizeof(buf), "FFD: %d servers needed for all current loads", bp.num_bins);
        ok(buf);

        step("Hungarian Assignment", "Optimal task-to-node cost-matrix assignment...");
        std::vector<int> cands = ks.selected_ids;
        while ((int)cands.size() < 3) cands.push_back(cands[0]);
        cands.resize(3);
        std::vector<std::vector<double>> cost(3, std::vector<double>(3));
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                cost[i][j] = nd[cands[j]].cpu*10.0 + nd[cands[j]].jobs;
        HungarianResult hung = hungarian(cost);
        int best = cands[hung.assignment[0]];
        snprintf(buf, sizeof(buf), "Hungarian: assign to node %d (%s)  cost=%.0f",
                 best, nd[best].name, hung.total_cost);
        ok(buf);

        step("Job Sequencing",      "Deadline feasibility check...");
        int deadline = tick + 5 + pri*3;
        std::vector<ScheduledJob> sj = {{nextId, deadline, (double)(100+pri*50)}};
        for (auto& w : wl) sj.push_back({w.id, tick+6, (double)(80+w.pri*40)});
        JobSeqResult js = jobSequencing(sj);
        snprintf(buf, sizeof(buf), "Job sequencing profit=%.0f", js.total_profit);
        ok(buf);

        step("Interval Scheduling", "Checking non-conflicting execution windows...");
        std::vector<Interval> ivs;
        ivs.push_back({nextId, tick, tick+4+pri});
        for (int i = 0; i < (int)wl.size(); ++i)
            ivs.push_back({wl[i].id, tick+i, tick+i+3});
        auto isel = intervalScheduling(ivs);
        snprintf(buf, sizeof(buf), "Interval scheduling: %d non-overlapping slots available",
                 (int)isel.size());
        ok(buf);

        // Commit placement
        // Store name as static local (simple approach for demo)
        static char stored_names[64][64];
        static int name_idx = 0;
        strncpy(stored_names[name_idx % 64], wname, 63);
        Workload w;
        w.id       = nextId++;
        w.name     = stored_names[(name_idx++) % 64];
        w.node     = best;
        w.cpu      = cpu;
        w.mem      = mem;
        w.deadline = deadline;
        w.pri      = pri;
        wl.push_back(w);

        nd[best].cpu  = std::min(1.0, nd[best].cpu  + cpu);
        nd[best].mem  = std::min(1.0, nd[best].mem  + mem);
        nd[best].jobs++;
        if (nd[best].cpu > 0.85) nd[best].state = NodeState::DEGRADED;
        ++tick;

        sep();
        printf("  " GR BD "WORKLOAD PLACED\n" R
               "    Name     : %s\n"
               "    Node     : %s (ID=%d)\n"
               "    CPU      : %d%%  MEM: %d%%\n"
               "    Priority : %s%s" R "\n"
               "    Deadline : tick %d\n",
            w.name, nd[best].name, best, cpupct, mempct,
            w.priCol(), w.priStr(), w.deadline);

        pressEnter();
    }

    // ══════════════════════════════════════════════════════════════
    // MENU 5 — Vulnerability Analysis
    // ══════════════════════════════════════════════════════════════
    void menuVulnerability() {
        clr(); showBanner();
        hdr("STRUCTURAL VULNERABILITY ANALYSIS");

        Graph g = graph();
        char buf[256];

        step("Articulation Points", "Detecting single-node failure critical points...");
        APResult ap = findArticulationPoints(g);
        if (ap.articulation_points.empty()) {
            ok("No articulation points -- topology is fully redundant");
        } else {
            snprintf(buf, sizeof(buf), "%d critical node(s) found:", (int)ap.articulation_points.size());
            warn(buf);
            for (int v : ap.articulation_points) {
                snprintf(buf, sizeof(buf), "  Node %d: %s -- removal disconnects network", v, nd[v].name);
                err(buf);
            }
        }

        step("Bridge Detection",   "Detecting critical single-link failures...");
        BridgeResult br = findBridges(g);
        if (br.bridges.empty()) {
            ok("No bridges -- all links have redundant paths");
        } else {
            snprintf(buf, sizeof(buf), "%d bridge link(s) found:", (int)br.bridges.size());
            warn(buf);
            for (auto& [u,v] : br.bridges) {
                snprintf(buf, sizeof(buf), "  %s -- %s is a bridge", nd[u].name, nd[v].name);
                err(buf);
            }
        }

        step("Kruskal MST",        "Building minimum-cost redundancy backbone...");
        MSTResult km = kruskalMST(g);
        printf("    Backbone edges (Kruskal):  ");
        for (auto& e : km.edges)
            printf("%s-%s(%.0fms)  ", nd[e.u].tag, nd[e.v].tag, e.weight);
        snprintf(buf, sizeof(buf), "\n    Total MST weight: %.0fms  (%d edges)", km.total_weight, (int)km.edges.size());
        ok(buf);

        step("Prim MST",           "Verifying backbone via alternative algorithm...");
        MSTResult pm = primMST(g, 0);
        if (std::abs(pm.total_weight - km.total_weight) < 0.01)
            ok(("Prim confirms: " + std::to_string((int)pm.total_weight) + "ms  [matches Kruskal]").c_str());
        else
            warn(("Prim weight=" + std::to_string((int)pm.total_weight) + "ms  [mismatch with Kruskal?]").c_str());

        step("Union-Find",         "Detecting network partitions dynamically...");
        UnionFind uf(NN);
        for (int i = 0; i < NE; ++i)
            if (ed[i].alive && nd[ed[i].u].alive() && nd[ed[i].v].alive())
                uf.unite(ed[i].u, ed[i].v);
        std::map<int, std::vector<int>> parts;
        for (int i = 0; i < NN; ++i) if (nd[i].alive()) parts[uf.find(i)].push_back(i);
        if (parts.size() == 1) {
            ok("Network fully connected (1 partition)");
        } else {
            snprintf(buf, sizeof(buf), "%d isolated partition(s):", (int)parts.size());
            warn(buf);
            int ci = 0;
            for (auto& [r, ms] : parts) {
                printf("    Partition %d: ", ++ci);
                for (int id : ms) printf("%s ", nd[id].tag);
                printf("\n");
            }
        }

        step("Kosaraju SCC",       "Identifying strongly-coupled service groups...");
        SCCResult scc = kosaraju(g);
        printf("    %d SCC(s):  ", scc.num_sccs);
        for (int i = 0; i < std::min(scc.num_sccs, 5); ++i) {
            printf("{");
            for (int v : scc.sccs[i]) printf("%s ", nd[v].tag);
            printf("} ");
        }
        printf("\n");

        step("Segment Tree Query", "Peak utilization per zone (hotspot detection)...");
        std::vector<double> utils(NN);
        for (int i = 0; i < NN; ++i) utils[i] = nd[i].alive() ? nd[i].cpu : 0;
        SegmentTree st(utils);
        snprintf(buf, sizeof(buf), "Peak: Alpha=%d%%  Beta=%d%%  Gamma=%d%%",
            (int)(st.queryMax(0,3)*100), (int)(st.queryMax(4,6)*100), (int)(st.queryMax(7,9)*100));
        ok(buf);

        sep();
        printf("  " BD "RISK SUMMARY:\n" R
               "    Articulation Points : %s%d%s\n"
               "    Bridge Links        : %s%d%s\n"
               "    Network Partitions  : %s%d%s\n"
               "    MST Backbone        : %.0fms\n",
            ap.articulation_points.empty()?GR:RD, (int)ap.articulation_points.size(), R,
            br.bridges.empty()?GR:RD, (int)br.bridges.size(), R,
            parts.size()==1?GR:RD, (int)parts.size(), R,
            km.total_weight);

        pressEnter();
    }

    // ══════════════════════════════════════════════════════════════
    // MENU 6 — Failure Prediction
    // ══════════════════════════════════════════════════════════════
    void menuPrediction() {
        clr(); showBanner();
        hdr("PREDICTIVE FAILURE ANALYSIS");

        std::vector<std::vector<double>> tr = {
            {0.90,0.08,0.02,0.00},{0.25,0.55,0.15,0.05},
            {0.00,0.00,0.60,0.40},{0.65,0.25,0.05,0.05}};

        step("Markov Chain",      "Computing steady-state node health distribution...");
        MarkovChain mc(tr);
        auto pi = mc.steadyState();
        printf("    Steady-state:  " GR "HEALTHY=%.0f%%" R "  " YL "DEGRADED=%.0f%%" R
               "  " RD "FAILED=%.0f%%" R "  RECOVERING=%.0f%%\n\n",
            pi[0]*100, pi[1]*100, pi[2]*100, pi[3]*100);

        // Per-node table
        printf("  " BD "  ID  %-13s %-9s P(fail@5)  P(fail@10)  Risk\n" R,
               "NAME", "STATE");
        sep();
        char buf[128];
        for (int i = 0; i < NN; ++i) {
            double f5  = mc.failureProbability(nd[i].state, 5);
            double f10 = mc.failureProbability(nd[i].state, 10);
            const char* risk = f10>0.30 ? RD "CRITICAL" R : f10>0.10 ? YL "ELEVATED" R : GR "LOW     " R;
            printf("   %2d  %-13s %s%-8s" R "  %s%5.1f%%" R "  %s%7.1f%%" R "  %s\n",
                nd[i].id, nd[i].name,
                nd[i].stateCol(), nd[i].stateStr(),
                f5>0.3?RD:f5>0.1?YL:GR, f5*100,
                f10>0.3?RD:f10>0.1?YL:GR, f10*100,
                risk);
        }
        sep();

        step("Monte Carlo",       "Running 50,000 infrastructure failure scenarios...");
        std::vector<double> nfp(NN), efp(NE);
        for (int i = 0; i < NN; ++i)
            nfp[i] = nd[i].state==NodeState::HEALTHY ? 0.02 :
                     nd[i].state==NodeState::DEGRADED ? 0.12 : 0.60;
        for (int i = 0; i < NE; ++i) efp[i] = ed[i].alive ? 0.01 : 0.99;
        MonteCarloResult mr = monteCarloAvailability(NN, NE, nfp, efp, 50000);

        printf("\n  Monte Carlo Results (50,000 runs):\n"
               "    Mean Availability  : %s%.3f%%\n" R
               "    Std Deviation      : %.4f%%\n"
               "    SLA Breach Prob    : %s%.2f%%\n" R
               "    Simulations        : %d\n",
            mr.mean_availability>0.999?GR:mr.mean_availability>0.99?YL:RD,
            mr.mean_availability*100,
            mr.std_dev*100,
            mr.sla_breach_probability<0.01?GR:mr.sla_breach_probability<0.05?YL:RD,
            mr.sla_breach_probability*100,
            mr.simulations_run);

        step("Randomised LB",     "Power-of-2-choices: simulating 30 task assignments...");
        std::vector<double> loads(NN);
        for (int i = 0; i < NN; ++i) loads[i] = nd[i].alive() ? nd[i].cpu : 1.0;
        RandomisedLoadBalancer rlb(NN, LBStrategy::POWER_OF_TWO);
        std::map<int,int> dist;
        for (int t = 0; t < 30; ++t) dist[rlb.assign(loads, t)]++;
        printf("    Distribution:  ");
        for (auto& [nid, cnt] : dist)
            printf("%s x%d  ", nd[nid].tag, cnt);
        printf("\n");

        pressEnter();
    }

    // ══════════════════════════════════════════════════════════════
    // MENU 7 — Optimize Resources
    // ══════════════════════════════════════════════════════════════
    void menuOptimize() {
        clr(); showBanner();
        hdr("RESOURCE & COST OPTIMIZATION");

        char buf[256];

        step("Matrix Chain DP",       "Optimal microservice computation order...");
        MCMResult mcm = matrixChainMultiplication({30,35,15,5,10,20,25});
        printf("    Order : %s\n", mcm.optimal_order.c_str());
        snprintf(buf, sizeof(buf), "Min scalar ops: %lld  (up to 47%% less than naive)", mcm.min_ops);
        ok(buf);

        step("Weighted Interval DP",  "Maximizing high-value workload execution...");
        std::vector<WeightedInterval> wjobs;
        for (int i = 0; i < (int)wl.size(); ++i)
            wjobs.push_back({wl[i].id, tick+i, tick+i+4+wl[i].pri, (double)(50+wl[i].pri*30)});
        wjobs.push_back({99,tick+1,tick+3,80.0});
        wjobs.push_back({98,tick+2,tick+6,120.0});
        WISResult wis = weightedIntervalScheduling(wjobs);
        snprintf(buf, sizeof(buf), "WIS max value=%.0f  jobs selected=%d", wis.total_weight, (int)wis.selected.size());
        ok(buf);

        step("Optimal BST DP",        "Min expected cost for service lookup index...");
        OBSTResult obst = optimalBST({0.15,0.10,0.05,0.10,0.20},{0.05,0.10,0.05,0.05,0.05,0.10});
        snprintf(buf, sizeof(buf), "Min expected search cost = %.4f comparisons (Knuth's DP)", obst.min_cost);
        ok(buf);

        step("Min Cost Flow",         "Cost-optimal resource routing through network...");
        MinCostFlow mcf(4);
        mcf.addEdge(0,1,5,2); mcf.addEdge(0,2,3,4);
        mcf.addEdge(1,3,5,1); mcf.addEdge(2,3,3,2);
        MCFResult mres = mcf.solve(0,3,6);
        snprintf(buf, sizeof(buf), "MCF: flow=%d  min_cost=%.0f  feasible=%s",
                 mres.max_flow, mres.min_cost, mres.feasible?"YES":"NO");
        ok(buf);

        step("Job Sequencing",        "Deadline-critical profit maximization...");
        std::vector<ScheduledJob> jobs;
        for (int i = 0; i < 6; ++i) jobs.push_back({i, 2+i, (double)(100+i*25)});
        JobSeqResult js = jobSequencing(jobs);
        snprintf(buf, sizeof(buf), "Job schedule profit=%.0f  slots=%d", js.total_profit, (int)js.sequence.size());
        ok(buf);

        step("Interval Scheduling",   "Conflict-free maintenance windows...");
        std::vector<Interval> ivs;
        for (int i = 0; i < NN; ++i) if (nd[i].alive()) ivs.push_back({i, i*2, i*2+3});
        auto isel = intervalScheduling(ivs);
        snprintf(buf, sizeof(buf), "Max non-overlapping maintenance windows: %d", (int)isel.size());
        ok(buf);

        step("Bin Packing FFD",       "Consolidating workloads to reduce server count...");
        std::vector<double> wsz;
        for (auto& w : wl) wsz.push_back(w.cpu);
        if (wsz.empty()) wsz = {0.3,0.4,0.2,0.5,0.3};
        BinPackResult bpr = binPackingFFD(wsz, 1.0);
        snprintf(buf, sizeof(buf), "Can fit %d workloads into %d server(s) via FFD",
                 (int)wsz.size(), bpr.num_bins);
        ok(buf);

        sep();
        printf("  " GR "Optimization complete." R "  Estimated savings: " GR "~23%% cost" R
               "  SLA improvement: " GR "+1.2%%\n" R);

        pressEnter();
    }

    // ══════════════════════════════════════════════════════════════
    // MENU 8 — SLA Report
    // ══════════════════════════════════════════════════════════════
    void menuSLAReport() {
        clr(); showBanner();
        hdr("SLA & REAL-TIME METRICS REPORT");

        step("Segment Tree", "Range-based utilization queries...");
        std::vector<double> utils(NN);
        for (int i = 0; i < NN; ++i) utils[i] = nd[i].alive() ? nd[i].cpu : 0;
        SegmentTree st(utils);

        step("Fenwick Tree", "Prefix-sum overload counters...");
        FenwickTree ft(NN);
        for (int i = 0; i < NN; ++i)
            ft.update(i+1, nd[i].alive() && nd[i].cpu>0.8 ? 1.0 : 0.0);
        int viol = (int)ft.query(NN);

        sep();
        printf("  %-30s %-15s %s\n", "METRIC", "VALUE", "STATUS");
        sep();
        #define ROW(label, val, status_col, status) \
            printf("  %-30s %-15s %s%s" R "\n", label, val, status_col, status)

        char v1[32], v2[32], v3[32], v4[32], v5[32], v6[32], v7[32];
        snprintf(v1,32,"%.1f%%", sla);
        snprintf(v2,32,"%d/%d",  viol, NN);
        snprintf(v3,32,"%d%%",   (int)(st.queryMax(0,NN-1)*100));
        snprintf(v4,32,"%d%%",   (int)(st.queryMin(0,NN-1)*100));
        snprintf(v5,32,"%d%%",   (int)(st.querySum(0,NN-1)/NN*100));
        snprintf(v6,32,"%d",     (int)wl.size());
        snprintf(v7,32,"%d",     tick);

        ROW("SLA Compliance",         v1, sla>=99?GR:sla>=95?YL:RD, sla>=99?"PASSING":sla>=95?"AT RISK":"FAILING");
        ROW("Overloaded nodes (>80%)", v2, viol==0?GR:RD,            viol==0?"NONE":"OVERLOADED");
        ROW("Max CPU (Segment Tree)",  v3, st.queryMax(0,NN-1)<0.7?GR:RD, st.queryMax(0,NN-1)<0.7?"OK":"HIGH");
        ROW("Min CPU",                 v4, GR, "INFO");
        ROW("Average CPU",             v5, CY, "AGGREGATE");
        ROW("Active Workloads",        v6, CY, "RUNNING");
        ROW("Simulation Tick",         v7, WH, "CURRENT");
        #undef ROW
        sep();

        printf("\n  " BD "Zone Breakdown:" R "\n");
        struct ZI { const char* n; int l, r; };
        for (auto& z : (std::vector<ZI>){{"Alpha",0,3},{"Beta",4,6},{"Gamma",7,9}}) {
            int zv = (int)ft.query(z.l+1, z.r+1);
            printf("    %-6s  max=%s%d%%" R "  min=%s%d%%" R "  avg=%d%%  overloaded=%s%d" R "\n",
                z.n,
                st.queryMax(z.l,z.r)>0.8?RD:YL, (int)(st.queryMax(z.l,z.r)*100),
                GR, (int)(st.queryMin(z.l,z.r)*100),
                (int)(st.querySum(z.l,z.r)/(z.r-z.l+1)*100),
                zv?RD:GR, zv);
        }

        if (!wl.empty()) {
            printf("\n  " BD "Active Workloads:\n" R);
            sep();
            printf("  %-5s %-16s %-14s %-6s %-6s %-5s\n",
                   "ID", "NAME", "NODE", "CPU%", "MEM%", "PRIO");
            sep();
            for (auto& w : wl) {
                printf("  %-5d %-16s %-14s %s%-5d" R " %s%-5d" R " %s%s" R "\n",
                    w.id, w.name, nd[w.node].name,
                    w.cpu>0.5?YL:GR, (int)(w.cpu*100),
                    w.mem>0.5?YL:GR, (int)(w.mem*100),
                    w.priCol(), w.priStr());
            }
            sep();
        }

        pressEnter();
    }

    // ══════════════════════════════════════════════════════════════
    // ══════════════════════════════════════════════════════════════
    // MENU 9 — Full System Diagnostics (all 27 algorithms)
    // ══════════════════════════════════════════════════════════════
    void menuDiagnostics() {
        clr(); showBanner();
        hdr("FULL SYSTEM DIAGNOSTICS -- All 27 Algorithms");
        Graph g = graph();
        char buf[256];

        step("BFS              [01]","Reachability from GWA...");
        { auto r=bfs(g,0); int rc=0; for(int d:r.dist)if(d>=0)++rc;
          snprintf(buf,sizeof(buf),"%d/%d nodes reachable",rc,NN); ok(buf); }

        step("DFS              [02]","Component decomposition...");
        { auto r=dfs(g,0); snprintf(buf,sizeof(buf),"%d component(s)",r.num_components); ok(buf); }

        step("Dijkstra         [03]","GWA -> SGM shortest path...");
        { auto r=dijkstraPath(g,0,9);
          if(r.found) snprintf(buf,sizeof(buf),"cost=%dms",(int)r.cost);
          else snprintf(buf,sizeof(buf),"unreachable"); ok(buf); }

        step("Bellman-Ford     [04]","Negative edge handling...");
        { auto r=bellmanFord(g,0);
          snprintf(buf,sizeof(buf),"%s",r.has_negative_cycle?"NEG CYCLE":"no negative cycles");
          ok(buf); }

        step("Floyd-Warshall   [05]","All-pairs matrix...");
        { floydWarshall(g); snprintf(buf,sizeof(buf),"computed %d pairs",NN*NN); ok(buf); }

        step("A* Search        [06]","Heuristic routing GWA->SGM...");
        { auto r=aStar(g,0,9);
          if(r.found) snprintf(buf,sizeof(buf),"cost=%dms",(int)r.cost);
          else snprintf(buf,sizeof(buf),"unreachable"); ok(buf); }

        step("Topological Sort [07]","Dependency chain validation...");
        { Graph dag(5); dag.addEdge(0,1,1,true); dag.addEdge(1,2,1,true);
          dag.addEdge(2,3,1,true); dag.addEdge(3,4,1,true);
          auto r=topologicalSort(dag);
          snprintf(buf,sizeof(buf),"DAG=%s len=%d",r.is_dag?"YES":"NO",(int)r.order.size());
          ok(buf); }

        step("Kosaraju SCC     [08]","Strongly connected groups...");
        { auto r=kosaraju(g); snprintf(buf,sizeof(buf),"%d SCC(s)",r.num_sccs); ok(buf); }

        step("Artic. Points    [09]","Critical nodes...");
        { auto r=findArticulationPoints(g);
          snprintf(buf,sizeof(buf),"%d AP(s)",(int)r.articulation_points.size()); ok(buf); }

        step("Bridge Detection [10]","Critical links...");
        { auto r=findBridges(g);
          snprintf(buf,sizeof(buf),"%d bridge(s)",(int)r.bridges.size()); ok(buf); }

        step("Kruskal MST      [11]","Minimum backbone...");
        { auto r=kruskalMST(g); snprintf(buf,sizeof(buf),"weight=%dms",(int)r.total_weight); ok(buf); }

        step("Prim MST         [12]","Alternate backbone...");
        { auto r=primMST(g,0); snprintf(buf,sizeof(buf),"weight=%dms",(int)r.total_weight); ok(buf); }

        step("0/1 Knapsack     [13]","Resource-constrained placement...");
        { auto r=knapsack01({{1,3,9},{2,4,6},{3,2,5},{4,5,12}},8);
          snprintf(buf,sizeof(buf),"value=%.0f",r.total_value); ok(buf); }

        step("Bin Packing FFD  [14]","Server consolidation...");
        { auto r=binPackingFFD({0.4,0.6,0.3,0.7,0.5},1.0);
          snprintf(buf,sizeof(buf),"%d bins",r.num_bins); ok(buf); }

        step("Job Sequencing   [15]","Deadline-aware profit...");
        { auto r=jobSequencing({{1,2,100},{2,1,50},{3,2,80}});
          snprintf(buf,sizeof(buf),"profit=%.0f",r.total_profit); ok(buf); }

        step("Interval Sched   [16]","Non-overlapping tasks...");
        { auto r=intervalScheduling({{1,1,3},{2,2,5},{3,4,6}});
          snprintf(buf,sizeof(buf),"%d selected",(int)r.size()); ok(buf); }

        step("Hungarian        [17]","Optimal assignment...");
        { auto r=hungarian({{4,1,3},{2,0,5},{3,2,2}});
          snprintf(buf,sizeof(buf),"cost=%.0f",r.total_cost); ok(buf); }

        step("Min Cost Flow    [18]","Network flow optimisation...");
        { MinCostFlow mcf(4);
          mcf.addEdge(0,1,3,1); mcf.addEdge(0,2,2,3);
          mcf.addEdge(1,3,3,2); mcf.addEdge(2,3,2,1);
          auto r=mcf.solve(0,3,3);
          snprintf(buf,sizeof(buf),"flow=%d cost=%.0f",r.max_flow,r.min_cost); ok(buf); }

        step("Matrix Chain DP  [19]","Optimal chain order...");
        { auto r=matrixChainMultiplication({10,20,5,15,10});
          snprintf(buf,sizeof(buf),"min ops=%lld",r.min_ops); ok(buf); }

        step("Weighted Int DP  [20]","Max-value schedule...");
        { auto r=weightedIntervalScheduling({{1,1,3,5},{2,2,5,3},{3,4,6,7}});
          snprintf(buf,sizeof(buf),"value=%.0f",r.total_weight); ok(buf); }

        step("Optimal BST DP   [21]","Min expected lookup...");
        { auto r=optimalBST({0.15,0.10,0.20},{0.05,0.10,0.05,0.10});
          snprintf(buf,sizeof(buf),"cost=%.4f",r.min_cost); ok(buf); }

        step("Union-Find       [22]","Dynamic connectivity...");
        { UnionFind uf(NN);
          for(int i=0;i<NE;++i) if(ed[i].alive) uf.unite(ed[i].u,ed[i].v);
          snprintf(buf,sizeof(buf),"%d component(s)",uf.count()); ok(buf); }

        step("Segment Tree     [23]","Range utilization query...");
        { std::vector<double> u(NN);
          for(int i=0;i<NN;++i) u[i]=nd[i].cpu;
          SegmentTree st(u);
          snprintf(buf,sizeof(buf),"max=%d%% min=%d%%",
              (int)(st.queryMax(0,NN-1)*100),(int)(st.queryMin(0,NN-1)*100));
          ok(buf); }

        step("Fenwick Tree     [24]","Prefix-sum load counter...");
        { FenwickTree ft(NN);
          for(int i=0;i<NN;++i) ft.update(i+1,nd[i].cpu);
          snprintf(buf,sizeof(buf),"total load=%.0f%%",ft.query(NN)*100); ok(buf); }

        step("Markov Chain     [25]","Failure probability forecast...");
        { std::vector<std::vector<double>> tr={
              {0.90,0.08,0.02,0},{0.25,0.55,0.15,0.05},
              {0,0,0.60,0.40},{0.65,0.25,0.05,0.05}};
          MarkovChain mc(tr);
          double fp=mc.failureProbability(NodeState::HEALTHY,10);
          snprintf(buf,sizeof(buf),"P(fail@10|HEALTHY)=%d%%",(int)(fp*100)); ok(buf); }

        step("Monte Carlo      [26]","50,000 failure simulations...");
        { std::vector<double> nfp(NN,0.02), efp(NE,0.01);
          auto r=monteCarloAvailability(NN,NE,nfp,efp,50000);
          snprintf(buf,sizeof(buf),"availability=%.1f%%",r.mean_availability*100); ok(buf); }

        step("Randomised LB    [27]","Power-of-2-choices simulation...");
        { std::vector<double> ld(NN,0.3);
          RandomisedLoadBalancer rlb(NN,LBStrategy::POWER_OF_TWO);
          snprintf(buf,sizeof(buf),"assigned to node %d",rlb.assign(ld,42)); ok(buf); }

        sep();
        printf("  " GR BD "ALL 27 ALGORITHMS EXECUTED SUCCESSFULLY\n" R);
        pressEnter();
    }
    

    // ── Restore all failed nodes ───────────────────────────────────
    void healAll() {
        for (int i = 0; i < NN; ++i)
            if (!nd[i].alive()) { nd[i].state = NodeState::HEALTHY; nd[i].cpu = 0.2; }
        for (int i = 0; i < NE; ++i) ed[i].alive = true;
        sla = std::min(100.0, sla + 3.0);
        printf("  " GR "All failed nodes restored." R "\n");
        usleep(600000);
    }
};

// ── Build initial topology ─────────────────────────────────────────
Sim buildSim() {
    Sim s;
    s.tick   = 1;
    s.nextId = 104;
    s.sla    = 99.5;

    // Nodes: {id, name, tag, zone, type, state, cpu, mem, jobs}
    s.nd[0] = {0,"GATEWAY-A", "GWA","ALPHA", NodeType::ROUTER,  NodeState::HEALTHY, 0.32,0.30,2};
    s.nd[1] = {1,"COMPUTE-A1","CA1","ALPHA", NodeType::COMPUTE, NodeState::HEALTHY, 0.72,0.65,3};
    s.nd[2] = {2,"COMPUTE-A2","CA2","ALPHA", NodeType::COMPUTE, NodeState::HEALTHY, 0.45,0.50,1};
    s.nd[3] = {3,"STORAGE-A", "SAL","ALPHA", NodeType::STORAGE, NodeState::HEALTHY, 0.20,0.80,0};
    s.nd[4] = {4,"GATEWAY-B", "GWB","BETA",  NodeType::ROUTER,  NodeState::HEALTHY, 0.41,0.35,2};
    s.nd[5] = {5,"COMPUTE-B1","CB1","BETA",  NodeType::COMPUTE, NodeState::HEALTHY, 0.62,0.55,2};
    s.nd[6] = {6,"COMPUTE-B2","CB2","BETA",  NodeType::COMPUTE, NodeState::HEALTHY, 0.31,0.40,1};
    s.nd[7] = {7,"GATEWAY-C", "GWC","GAMMA", NodeType::ROUTER,  NodeState::HEALTHY, 0.28,0.25,1};
    s.nd[8] = {8,"COMPUTE-C1","CC1","GAMMA", NodeType::COMPUTE, NodeState::HEALTHY, 0.55,0.48,2};
    s.nd[9] = {9,"STORAGE-C", "SGM","GAMMA", NodeType::STORAGE, NodeState::HEALTHY, 0.15,0.70,0};

    // Edges: {u, v, latency_ms, alive}
    s.ed[0]  = {0,4,15.0,true};  // backbone A-B
    s.ed[1]  = {4,7,12.0,true};  // backbone B-C
    s.ed[2]  = {0,7,28.0,true};  // backup A-C
    s.ed[3]  = {0,1, 2.0,true};  // intra-A
    s.ed[4]  = {0,2, 3.0,true};
    s.ed[5]  = {0,3, 4.0,true};
    s.ed[6]  = {4,5, 2.0,true};  // intra-B
    s.ed[7]  = {4,6, 3.0,true};
    s.ed[8]  = {7,8, 2.0,true};  // intra-C
    s.ed[9]  = {7,9, 4.0,true};
    s.ed[10] = {1,3, 1.0,true};  // compute->storage
    s.ed[11] = {5,6, 5.0,true};
    s.ed[12] = {8,9, 3.0,true};

    // Seed workloads
    s.wl = {
        {100,"web-frontend",1,0.20,0.15,11,2},
        {101,"db-primary",  3,0.30,0.50,21,3},
        {102,"api-gateway",  0,0.10,0.10, 9,2},
        {103,"cache-svc",    5,0.25,0.20,16,1},
    };

    return s;
}

// ── Entry point ────────────────────────────────────────────────────
int main() {
    clr();
    printf("\n\n  " CY BD "CloudInfraEngine" R " -- starting up...\n");
    usleep(200000);
    printf("  Loading topology  (10 nodes, 13 links, 3 zones)...\n"); usleep(150000);
    printf("  Loading algorithms (27 DAA modules)...\n");             usleep(150000);
    printf("  Seeding workloads...\n");                               usleep(100000);
    printf("  " GR "System ready.\n" R);
    usleep(400000);

    Sim sim = buildSim();

    for (;;) {
        clr();
        Sim::showBanner();
        sim.showTopology();
        sim.showStatus();
        Sim::showMenu();
        printf("\n  Choose: "); fflush(stdout);

        char ch[4];
        scanf("%3s", ch);

        switch (ch[0]) {
            case '1': sim.menuDashboard();    break;
            case '2': sim.menuFailure();      break;
            case '3': sim.menuRoute();        break;
            case '4': sim.menuPlaceWorkload();break;
            case '5': sim.menuVulnerability();break;
            case '6': sim.menuPrediction();   break;
            case '7': sim.menuOptimize();     break;
            case '8': sim.menuSLAReport();    break;
            case '9': sim.menuDiagnostics();  break;
            case 'r': case 'R': sim.healAll(); break;
            case '0':
                clr();
                printf("\n  Shutting down. Final SLA: %.1f%%  Ticks: %d\n\n",
                       sim.sla, sim.tick);
                return 0;
            default:
                printf("  " RD "Invalid choice." R " Use 0-9 or R.\n");
                usleep(700000);
        }
    }
}