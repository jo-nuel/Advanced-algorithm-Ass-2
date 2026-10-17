#include "benchmark/benchmark_cases.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace selection_benchmark {

namespace {

constexpr std::array inputFamilies{
    InputFamily::RandomUnique, InputFamily::RandomDuplicates,
    InputFamily::Sorted,       InputFamily::ReverseSorted,
    InputFamily::NearlySorted, InputFamily::AllEqual,
};

constexpr std::array<std::size_t, 2> smokeSizes{1000, 5000};
constexpr std::array<std::size_t, 11> fullSizes{
    1000,  2000,   4000,   8000,   16000, 32000,
    64000, 128000, 256000, 500000, 1000000,
};

std::array<std::size_t, 5> ranksForSize(const std::size_t size) {
    return {0, size / 4, size / 2, 3 * size / 4, size - 1};
}

template <std::size_t SizeCount>
std::vector<BenchmarkCase> buildCases(
    const std::array<std::size_t, SizeCount>& sizes) {
    std::vector<BenchmarkCase> cases;
    cases.reserve(sizes.size() * inputFamilies.size() * 5);

    for (std::size_t sizeIndex = 0; sizeIndex < sizes.size(); ++sizeIndex) {
        const std::size_t size = sizes[sizeIndex];
        for (std::size_t familyIndex = 0; familyIndex < inputFamilies.size();
             ++familyIndex) {
            const std::uint32_t inputSeed =
                defaultInputSeed + static_cast<std::uint32_t>(100 * familyIndex +
                                                               sizeIndex);
            const std::uint32_t pivotSeed =
                defaultPivotSeed + static_cast<std::uint32_t>(100 * familyIndex +
                                                               10 * sizeIndex);

            for (const std::size_t rank : ranksForSize(size)) {
                cases.push_back(
                    {inputFamilies[familyIndex], size, rank, inputSeed, pivotSeed});
            }
        }
    }

    return cases;
}

}  // namespace

BenchmarkMode parseBenchmarkMode(const std::string_view text) {
    if (text == "smoke") {
        return BenchmarkMode::Smoke;
    }
    if (text == "full") {
        return BenchmarkMode::Full;
    }
    throw std::invalid_argument("benchmark mode must be smoke or full");
}

std::vector<BenchmarkCase> buildBenchmarkCases(const BenchmarkMode mode) {
    switch (mode) {
        case BenchmarkMode::Smoke:
            return buildCases(smokeSizes);
        case BenchmarkMode::Full:
            return buildCases(fullSizes);
    }
    throw std::invalid_argument("unknown benchmark mode");
}

}  // namespace selection_benchmark

