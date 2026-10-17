#pragma once

#include "benchmark/input_generator.hpp"

#include <string_view>
#include <vector>

namespace selection_benchmark {

enum class BenchmarkMode {
    Smoke,
    Full,
};

BenchmarkMode parseBenchmarkMode(std::string_view text);

std::vector<BenchmarkCase> buildBenchmarkCases(BenchmarkMode mode);

}  // namespace selection_benchmark

