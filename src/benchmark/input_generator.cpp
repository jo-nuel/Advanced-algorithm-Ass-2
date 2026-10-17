#include "benchmark/input_generator.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace selection_benchmark {

namespace {

std::vector<int> sortedValues(const std::size_t size) {
    std::vector<int> values(size);
    std::iota(values.begin(), values.end(), -static_cast<int>(size / 2));
    return values;
}

}  // namespace

std::string_view inputFamilyName(const InputFamily family) {
    switch (family) {
        case InputFamily::RandomUnique:
            return "random-unique";
        case InputFamily::RandomDuplicates:
            return "random-duplicates";
        case InputFamily::Sorted:
            return "sorted";
        case InputFamily::ReverseSorted:
            return "reverse-sorted";
        case InputFamily::NearlySorted:
            return "nearly-sorted";
        case InputFamily::AllEqual:
            return "all-equal";
    }
    throw std::invalid_argument("unknown input family");
}

std::vector<int> generateInput(const InputFamily family, const std::size_t size,
                               const std::uint32_t seed) {
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("input size exceeds the supported integer range");
    }

    std::mt19937 generator(seed);

    switch (family) {
        case InputFamily::RandomUnique: {
            std::vector<int> values = sortedValues(size);
            std::shuffle(values.begin(), values.end(), generator);
            return values;
        }
        case InputFamily::RandomDuplicates: {
            std::vector<int> values(size);
            std::uniform_int_distribution<int> repeatedValue(-3, 3);
            for (int& value : values) {
                value = repeatedValue(generator);
            }
            return values;
        }
        case InputFamily::Sorted:
            return sortedValues(size);
        case InputFamily::ReverseSorted: {
            std::vector<int> values = sortedValues(size);
            std::reverse(values.begin(), values.end());
            return values;
        }
        case InputFamily::NearlySorted: {
            std::vector<int> values = sortedValues(size);
            if (size < 2) {
                return values;
            }

            std::vector<std::size_t> adjacentPairs(size / 2);
            std::iota(adjacentPairs.begin(), adjacentPairs.end(), std::size_t{0});
            std::shuffle(adjacentPairs.begin(), adjacentPairs.end(), generator);

            const std::size_t swaps = std::max(std::size_t{1}, size / 100);
            for (std::size_t swap = 0; swap < swaps; ++swap) {
                const std::size_t first = 2 * adjacentPairs[swap];
                std::swap(values[first], values[first + 1]);
            }
            return values;
        }
        case InputFamily::AllEqual:
            return std::vector<int>(size, -7);
    }

    throw std::invalid_argument("unknown input family");
}

}  // namespace selection_benchmark

