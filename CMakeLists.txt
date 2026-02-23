# Require a reasonably modern version of CMake (3.15+ is standard for Mac/Win)
cmake_minimum_required(VERSION 3.15)

# Define the project name and specify C++ as the language
project(CloudInfraEngine VERSION 1.0.0 LANGUAGES CXX)

# Enforce C++17 standard (Perfect for modern algorithms, widely supported)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disables OS-specific extensions for true cross-platform code

# ------------------------------------------------------------------------------
# DIRECTORY MANAGEMENT (Crucial for Windows/Mac parity)
# This forces the compiled .exe (Win) or binary (Mac) into a clean "bin" folder
# instead of cluttering up the build directory.
# ------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ------------------------------------------------------------------------------
# SOURCE FILE DISCOVERY
# ------------------------------------------------------------------------------
# We gather all .cpp files inside the subdirectories of src/ (e.g., src/routing/)
# Note: We deliberately EXCLUDE src/main.cpp here so it doesn't get compiled twice.
file(GLOB_RECURSE ALGO_SOURCES "src/*/*.cpp")

# ------------------------------------------------------------------------------
# TARGET: CloudAlgorithms (Static Library)
# ------------------------------------------------------------------------------
# We build all the team's algorithms into a single reusable library
add_library(CloudAlgorithms STATIC ${ALGO_SOURCES})

# Tell the compiler where to find the header files (.h)
# PUBLIC means anything linking to this library also gets access to include/
target_include_directories(CloudAlgorithms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ------------------------------------------------------------------------------
# COMPILER WARNINGS (Platform Specific)
# ------------------------------------------------------------------------------
# This ensures that both Windows and Mac compilers catch bad code (like unused variables)
if(MSVC)
    # Windows (Visual Studio / MSVC) warning flags
    target_compile_options(CloudAlgorithms PRIVATE /W4)
else()
    # macOS / Linux (Clang / GCC) warning flags
    target_compile_options(CloudAlgorithms PRIVATE -Wall -Wextra -pedantic)
endif()

# ------------------------------------------------------------------------------
# TARGET: Main Executable
# ------------------------------------------------------------------------------
# Compile the main simulation loop
add_executable(CloudInfraEngine src/main.cpp)

# Link the algorithm library to the main executable
target_link_libraries(CloudInfraEngine PRIVATE CloudAlgorithms)