#include "benchmark/benchmark_runner.hpp"

#include "selection/deterministic_selection.hpp"
#include "selection/randomised_quickselect.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace selection_benchmark {

namespace {

using Clock = std::chrono::steady_clock;

struct TimedResult {
    std::string_view algorithm;
    std::uint32_t pivotSeed;
    std::uint64_t timeNanoseconds;
    int result;
};

void checkResult(const int actual, const int expected,
                 const std::string_view algorithm) {
    if (actual != expected) {
        throw std::runtime_error(std::string(algorithm) +
                                 " returned an incorrect benchmark result");
    }
}

TimedResult timeDeterministic(const std::vector<int>& input, const std::size_t rank,
                              const int expected) {
    std::vector<int> values = input;

    const Clock::time_point start = Clock::now();
    const int result = selection::deterministicSelect(values, rank);
    const Clock::time_point finish = Clock::now();

    checkResult(result, expected, "median-of-medians");
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    return {"median-of-medians", 0U,
            static_cast<std::uint64_t>(elapsed.count()), result};
}

TimedResult timeRandomised(const std::vector<int>& input, const std::size_t rank,
                           const int expected, const std::uint32_t pivotSeed) {
    std::vector<int> values = input;
    std::mt19937 generator(pivotSeed);

    const Clock::time_point start = Clock::now();
    const int result = selection::randomisedSelect(values, rank, generator);
    const Clock::time_point finish = Clock::now();

    checkResult(result, expected, "randomised-quickselect");
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    return {"randomised-quickselect", pivotSeed,
            static_cast<std::uint64_t>(elapsed.count()), result};
}

void warmUp(const BenchmarkCase& benchmarkCase, const std::vector<int>& input,
            const int expected) {
    std::vector<int> deterministicValues = input;
    checkResult(selection::deterministicSelect(deterministicValues,
                                                benchmarkCase.rank),
                expected, "median-of-medians warm-up");

    std::vector<int> randomisedValues = input;
    std::mt19937 generator(benchmarkCase.pivotSeed);
    checkResult(selection::randomisedSelect(randomisedValues, benchmarkCase.rank,
                                             generator),
                expected, "randomised-quickselect warm-up");
}

void writeRow(std::ostream& output, const BenchmarkCase& benchmarkCase,
              const std::size_t repetition, const TimedResult& timed) {
    output << inputFamilyName(benchmarkCase.family) << ',' << benchmarkCase.size
           << ',' << benchmarkCase.rank << ',' << benchmarkCase.inputSeed << ',';

    if (timed.algorithm == "median-of-medians") {
        output << "not-applicable";
    } else {
        output << timed.pivotSeed;
    }

    output << ',' << timed.algorithm << ',' << repetition << ','
           << timed.timeNanoseconds << ',' << timed.result << '\n';
}

void runCase(std::ostream& output, const BenchmarkCase& benchmarkCase) {
    if (benchmarkCase.size == 0 || benchmarkCase.rank >= benchmarkCase.size) {
        throw std::invalid_argument("benchmark rank must refer to an input value");
    }

    const std::vector<int> input = generateInput(
        benchmarkCase.family, benchmarkCase.size, benchmarkCase.inputSeed);
    std::vector<int> oracle = input;
    std::sort(oracle.begin(), oracle.end());
    const int expected = oracle[benchmarkCase.rank];

    warmUp(benchmarkCase, input, expected);

    for (std::size_t repetition = 0; repetition < benchmarkRepetitions;
         ++repetition) {
        const std::uint32_t pivotSeed =
            benchmarkCase.pivotSeed + static_cast<std::uint32_t>(repetition);

        if (repetition % 2 == 0) {
            writeRow(output, benchmarkCase, repetition,
                     timeDeterministic(input, benchmarkCase.rank, expected));
            writeRow(output, benchmarkCase, repetition,
                     timeRandomised(input, benchmarkCase.rank, expected, pivotSeed));
        } else {
            writeRow(output, benchmarkCase, repetition,
                     timeRandomised(input, benchmarkCase.rank, expected, pivotSeed));
            writeRow(output, benchmarkCase, repetition,
                     timeDeterministic(input, benchmarkCase.rank, expected));
        }
    }
}

}  // namespace

void writeBenchmarkResults(std::ostream& output,
                           const std::span<const BenchmarkCase> cases) {
    output << benchmarkCsvHeader << '\n';
    for (const BenchmarkCase& benchmarkCase : cases) {
        runCase(output, benchmarkCase);
    }
}

}  // namespace selection_benchmark

