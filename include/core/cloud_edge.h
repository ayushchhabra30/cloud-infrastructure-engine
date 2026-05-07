#pragma once
#include <string>
#include "constants.h"

namespace CloudInfra {

struct CloudEdge {
    int    id;
    int    src;             // source node id
    int    dst;             // destination node id
    EdgeType type;

    double latency_ms;      // propagation latency
    double bandwidth_gbps;  // link capacity
    double cost;            // monetary / energy cost per GB
    double utilization;     // current load [0.0 – 1.0]

    double failure_prob;
    bool   is_alive;

    // Derived weight used by routing algorithms
    double weight() const { return latency_ms; }
    double capacity_remaining() const {
        return bandwidth_gbps * (1.0 - utilization);
    }

    CloudEdge() = default;
    CloudEdge(int id, int src, int dst, EdgeType type,
              double lat, double bw, double cost = 1.0)
        : id(id), src(src), dst(dst), type(type),
          latency_ms(lat), bandwidth_gbps(bw), cost(cost),
          utilization(0), failure_prob(LINK_FAILURE_PROB), is_alive(true) {}
};

} // namespace CloudInfra
