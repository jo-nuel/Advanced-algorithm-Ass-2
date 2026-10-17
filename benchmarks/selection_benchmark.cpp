#include "benchmark/benchmark_cases.hpp"
#include "benchmark/benchmark_runner.hpp"

#include <exception>
#include <fstream>
#include <iostream>
#include <string_view>

int main(const int argc, const char* const argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: selection_benchmark <smoke|full> <output.csv>\n";
        return 2;
    }

    try {
        const selection_benchmark::BenchmarkMode mode =
            selection_benchmark::parseBenchmarkMode(std::string_view(argv[1]));
        const std::vector<selection_benchmark::BenchmarkCase> cases =
            selection_benchmark::buildBenchmarkCases(mode);

        std::ofstream output(argv[2]);
        if (!output) {
            std::cerr << "Could not open the benchmark output file.\n";
            return 2;
        }

        selection_benchmark::writeBenchmarkResults(output, cases);
        if (!output) {
            std::cerr << "Writing the benchmark output failed.\n";
            return 2;
        }

        std::cout << "Recorded " << cases.size() << " benchmark cases in "
                  << argv[2] << ".\n";
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}

