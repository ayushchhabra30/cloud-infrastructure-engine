#pragma once
#include <limits>

namespace CloudInfra {

// SLA Thresholds
constexpr double SLA_LATENCY_MAX_MS     = 100.0;
constexpr double SLA_AVAILABILITY_MIN   = 0.999;   // 99.9%
constexpr double SLA_THROUGHPUT_MIN_GBPS = 1.0;

// Failure Probabilities
constexpr double NODE_FAILURE_PROB      = 0.02;
constexpr double LINK_FAILURE_PROB      = 0.01;
constexpr double CASCADING_FAIL_FACTOR  = 1.5;

// Graph / Simulation Limits
constexpr int    MAX_NODES              = 1024;
constexpr int    MAX_EDGES              = 8192;
constexpr double INF                    = std::numeric_limits<double>::infinity();
constexpr int    INF_INT                = std::numeric_limits<int>::max() / 2;

// Scheduling
constexpr int    MAX_TASKS              = 512;
constexpr int    MAX_RESOURCES          = 64;
constexpr double OVERLOAD_THRESHOLD     = 0.85;

// Monte Carlo
constexpr int    MC_ITERATIONS          = 100000;

// Node Types
enum class NodeType { COMPUTE, STORAGE, GATEWAY, ROUTER, EDGE };

// Edge Types
enum class EdgeType { FIBER, WIRELESS, SATELLITE, VIRTUAL };

// Task Priority
enum class Priority { LOW = 0, MEDIUM = 1, HIGH = 2, CRITICAL = 3 };

} // namespace CloudInfra
