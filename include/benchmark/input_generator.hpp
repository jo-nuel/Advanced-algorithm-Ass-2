#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace selection_benchmark {

enum class InputFamily {
    RandomUnique,
    RandomDuplicates,
    Sorted,
    ReverseSorted,
    NearlySorted,
    AllEqual,
};

struct BenchmarkCase {
    InputFamily family;
    std::size_t size;
    std::size_t rank;
    std::uint32_t inputSeed;
    std::uint32_t pivotSeed;
};

inline constexpr std::uint32_t defaultInputSeed = 14487692U;
inline constexpr std::uint32_t defaultPivotSeed = 20261017U;

inline constexpr std::string_view benchmarkCsvHeader =
    "family,n,k,input_seed,pivot_seed,algorithm,repetition,time_ns,result";

std::string_view inputFamilyName(InputFamily family);

std::vector<int> generateInput(InputFamily family, std::size_t size,
                               std::uint32_t seed);

}  // namespace selection_benchmark

