#pragma once

#include "benchmark/input_generator.hpp"

#include <cstddef>
#include <ostream>
#include <span>

namespace selection_benchmark {

inline constexpr std::size_t benchmarkRepetitions = 7;

// Generates each input once, performs one untimed warm-up per algorithm, then
// writes every timed repetition to CSV. Input copies and RNG setup are untimed.
void writeBenchmarkResults(std::ostream& output,
                           std::span<const BenchmarkCase> cases);

}  // namespace selection_benchmark

