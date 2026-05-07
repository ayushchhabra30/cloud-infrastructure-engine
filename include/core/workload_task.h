#pragma once
#include <string>
#include "constants.h"

namespace CloudInfra {

struct WorkloadTask {
    int    id;
    std::string name;
    Priority priority;

    // Resource requirements
    double req_cpu;         // cores needed
    double req_mem_gb;
    double req_bw_gbps;
    double req_storage_tb;

    // Scheduling
    int    deadline;        // simulation tick deadline
    int    duration;        // ticks needed to complete
    double value;           // business value / profit

    // Dependency graph (task ids this task depends on)
    std::vector<int> depends_on;

    // State
    int    assigned_node;   // -1 if unassigned
    bool   completed;
    int    start_tick;

    WorkloadTask() = default;
    WorkloadTask(int id, std::string name, Priority pri,
                 double cpu, double mem, double bw, double storage,
                 int deadline, int duration, double value)
        : id(id), name(std::move(name)), priority(pri),
          req_cpu(cpu), req_mem_gb(mem), req_bw_gbps(bw), req_storage_tb(storage),
          deadline(deadline), duration(duration), value(value),
          assigned_node(-1), completed(false), start_tick(-1) {}

    bool can_run_on(const struct CloudNode& n) const;
};

} // namespace CloudInfra
