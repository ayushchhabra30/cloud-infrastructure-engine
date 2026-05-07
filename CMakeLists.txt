cmake_minimum_required(VERSION 3.14)
project(CloudInfraEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ──────────────────────────────────────────────
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

include_directories(${CMAKE_SOURCE_DIR})

# ── Collect all algorithm source files ──────────────────────────
set(ALGO_SOURCES
    src/graph_core/traversals.cpp
    src/routing/dijkstra.cpp
    src/vulnerability/articulation_bridges.cpp
    src/scheduling/knapsack_allocator.cpp
    src/prediction_dp/dp_optimizers.cpp
)

# ── Main simulation executable ──────────────────────────────────
add_executable(CloudInfraEngine src/main.cpp ${ALGO_SOURCES})
target_include_directories(CloudInfraEngine PRIVATE ${CMAKE_SOURCE_DIR})

# ── Test executables ─────────────────────────────────────────────
enable_testing()

add_executable(test_graph_core  tests/test_graph_core.cpp  ${ALGO_SOURCES})
add_executable(test_routing     tests/test_routing.cpp     ${ALGO_SOURCES})
add_executable(test_scheduling  tests/test_scheduling.cpp  ${ALGO_SOURCES})

target_include_directories(test_graph_core  PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(test_routing     PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(test_scheduling  PRIVATE ${CMAKE_SOURCE_DIR})

add_test(NAME GraphCore  COMMAND test_graph_core)
add_test(NAME Routing    COMMAND test_routing)
add_test(NAME Scheduling COMMAND test_scheduling)

# ── Install ──────────────────────────────────────────────────────
install(TARGETS CloudInfraEngine DESTINATION bin)
