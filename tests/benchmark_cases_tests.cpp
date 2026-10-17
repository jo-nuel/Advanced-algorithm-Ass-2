#include "benchmark/benchmark_cases.hpp"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void check(const bool condition, const std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void checkCases(const std::vector<selection_benchmark::BenchmarkCase>& cases,
                const std::size_t expectedCount) {
    check(cases.size() == expectedCount, "benchmark case count is incorrect");
    for (const selection_benchmark::BenchmarkCase& benchmarkCase : cases) {
        check(benchmarkCase.size > 0, "benchmark case has an empty input");
        check(benchmarkCase.rank < benchmarkCase.size,
              "benchmark case has an invalid rank");
    }

    for (std::size_t start = 0; start + 5 <= cases.size(); start += 5) {
        const selection_benchmark::BenchmarkCase& first = cases[start];
        for (std::size_t offset = 1; offset < 5; ++offset) {
            const selection_benchmark::BenchmarkCase& current =
                cases[start + offset];
            check(current.family == first.family && current.size == first.size,
                  "five-rank block does not share an input family and size");
            check(current.inputSeed == first.inputSeed,
                  "five-rank block does not share an input seed");
            check(current.pivotSeed == first.pivotSeed,
                  "five-rank block does not share a base pivot seed");
        }
    }
}

}  // namespace

int main() {
    using selection_benchmark::BenchmarkMode;

    check(selection_benchmark::parseBenchmarkMode("smoke") == BenchmarkMode::Smoke,
          "smoke mode was not recognised");
    check(selection_benchmark::parseBenchmarkMode("full") == BenchmarkMode::Full,
          "full mode was not recognised");

    try {
        static_cast<void>(selection_benchmark::parseBenchmarkMode("quick"));
        check(false, "invalid benchmark mode should throw");
    } catch (const std::invalid_argument&) {
    } catch (...) {
        check(false, "invalid benchmark mode threw the wrong exception");
    }

    const std::vector<selection_benchmark::BenchmarkCase> smoke =
        selection_benchmark::buildBenchmarkCases(BenchmarkMode::Smoke);
    const std::vector<selection_benchmark::BenchmarkCase> full =
        selection_benchmark::buildBenchmarkCases(BenchmarkMode::Full);

    checkCases(smoke, 2 * 6 * 5);
    checkCases(full, 11 * 6 * 5);
    check(!full.empty() && full.back().size == 1000000,
          "full benchmark does not reach one million values");

    if (failures != 0) {
        std::cerr << failures << " benchmark case test(s) failed.\n";
        return 1;
    }

    std::cout << "All benchmark case tests passed.\n";
    return 0;
}

