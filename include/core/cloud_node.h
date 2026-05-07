#pragma once
#include <string>
#include <vector>
#include "constants.h"

namespace CloudInfra {

struct CloudNode {
    int    id;
    std::string name;
    NodeType type;

    // Capacity
    double cpu_cores;       // total CPU cores
    double memory_gb;       // total RAM in GB
    double storage_tb;      // total storage in TB
    double bandwidth_gbps;  // NIC bandwidth

    // Current utilization [0.0 – 1.0]
    double cpu_util;
    double mem_util;
    double bw_util;

    // Reliability
    double availability;    // e.g. 0.999
    double failure_prob;    // per-tick probability of failure
    bool   is_alive;

    // Geographic / zone info
    std::string zone;
    double latitude;
    double longitude;

    CloudNode() = default;
    CloudNode(int id, std::string name, NodeType type,
              double cpu, double mem, double storage, double bw,
              std::string zone = "default")
        : id(id), name(std::move(name)), type(type),
          cpu_cores(cpu), memory_gb(mem), storage_tb(storage), bandwidth_gbps(bw),
          cpu_util(0), mem_util(0), bw_util(0),
          availability(SLA_AVAILABILITY_MIN), failure_prob(NODE_FAILURE_PROB),
          is_alive(true), zone(std::move(zone)), latitude(0), longitude(0) {}

    double free_cpu()  const { return cpu_cores   * (1.0 - cpu_util); }
    double free_mem()  const { return memory_gb   * (1.0 - mem_util); }
    bool   overloaded() const {
        return cpu_util > OVERLOAD_THRESHOLD || mem_util > OVERLOAD_THRESHOLD;
    }
};

} // namespace CloudInfra
