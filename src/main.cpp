#include <iostream>
#include <vector>
#include <string>

int main() {
    // A quick check for modern C++ features
    std::vector<std::string> messages = {
        "CMake is configured correctly!",
        "Compiler: Check.",
        "Linker: Check.",
        "Ready to build your project."
    };

    std::cout << "--- Build Success ---" << std::endl;

    for (const auto& msg : messages) {
        std::cout << "[INFO] " << msg << std::endl;
    }

    return 0;
}