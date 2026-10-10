#include "selection/randomised_quickselect.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace {

std::size_t failures = 0;
std::size_t checks = 0;

constexpr std::uint32_t dataSeed = 14487692U;
constexpr std::array<std::uint32_t, 4> pivotSeeds{
    0U, 1U, 42U, std::numeric_limits<std::uint32_t>::max(),
};

void fail(const std::string_view context, const std::size_t size,
          const std::size_t k, const std::string_view message) {
    std::cerr << context << " size=" << size << " rank=" << k << ": "
              << message << '\n';
    ++failures;
}

// A null generator exercises the convenience overload.
void checkSelection(const std::string_view context, std::vector<int>& values,
                    const std::size_t k, std::mt19937* generator) {
    ++checks;
    const std::size_t originalSize = values.size();
    std::vector<int> oracle = values;
    std::sort(oracle.begin(), oracle.end());

    try {
        const int actual = generator == nullptr
                               ? selection::randomisedSelect(values, k)
                               : selection::randomisedSelect(values, k, *generator);
        if (actual != oracle[k]) {
            fail(context, originalSize, k,
                 "expected " + std::to_string(oracle[k]) + ", got " +
                     std::to_string(actual));
        }
    } catch (const std::exception& error) {
        fail(context, originalSize, k,
             "unexpected exception: " + std::string(error.what()));
    } catch (...) {
        fail(context, originalSize, k, "unexpected non-standard exception");
    }

    if (values.size() != originalSize) {
        fail(context, originalSize, k, "input size changed");
    }
    std::vector<int> after = values;
    std::sort(after.begin(), after.end());
    if (after != oracle) {
        fail(context, originalSize, k, "input multiset changed");
    }
}

void checkInvalid(const std::string_view context, std::vector<int> values,
                  const std::size_t k, std::mt19937* generator) {
    ++checks;
    const std::vector<int> original = values;
    const std::mt19937 originalGenerator =
        generator == nullptr ? std::mt19937{} : *generator;

    try {
        if (generator == nullptr) {
            static_cast<void>(selection::randomisedSelect(values, k));
        } else {
            static_cast<void>(selection::randomisedSelect(values, k, *generator));
        }
        fail(context, original.size(), k, "expected std::out_of_range");
    } catch (const std::out_of_range& error) {
        if (typeid(error) != typeid(std::out_of_range)) {
            fail(context, original.size(), k,
                 "expected exact std::out_of_range type");
        }
    } catch (const std::exception& error) {
        fail(context, original.size(), k,
             "wrong exception type: " + std::string(error.what()));
    } catch (...) {
        fail(context, original.size(), k, "wrong non-standard exception type");
    }

    if (values != original) {
        fail(context, original.size(), k, "invalid call changed input");
    }
    if (generator != nullptr && *generator != originalGenerator) {
        fail(context, original.size(), k, "invalid call advanced pivot generator");
    }
}

void checkRanks(const std::string& name, const std::vector<int>& original,
                const std::vector<std::size_t>& ranks,
                const bool includeConvenience = false) {
    if (includeConvenience) {
        const std::string context = name + " convenience overload";
        for (const std::size_t k : ranks) {
            std::vector<int> values = original;
            checkSelection(context, values, k, nullptr);
        }
    }

    for (const std::uint32_t seed : pivotSeeds) {
        const std::string context = name + " pivot_seed=" + std::to_string(seed);
        for (const std::size_t k : ranks) {
            std::vector<int> values = original;
            std::mt19937 generator(seed);
            checkSelection(context, values, k, &generator);
        }
    }
}

void checkEveryRank(const std::string& name, const std::vector<int>& original,
                    const bool includeConvenience = false) {
    std::vector<std::size_t> ranks(original.size());
    std::iota(ranks.begin(), ranks.end(), std::size_t{0});
    checkRanks(name, original, ranks, includeConvenience);
}

void testNamedInputs() {
    checkEveryRank("singleton", {42}, true);
    checkEveryRank("two ascending", {1, 2}, true);
    checkEveryRank("two descending", {2, 1}, true);
    checkEveryRank("two equal", {7, 7}, true);
    checkEveryRank("unsorted odd", {9, 1, 4, 7, 3}, true);
    checkEveryRank("unsorted even", {9, 1, 4, 7, 3, 2}, true);
    checkEveryRank("ascending", {-4, -3, -2, -1, 0, 1, 2, 3}, true);
    checkEveryRank("descending", {3, 2, 1, 0, -1, -2, -3, -4}, true);
    checkEveryRank("integer extremes",
                   {INT_MAX, -2, INT_MIN, 0, 5, -8, INT_MAX, INT_MIN}, true);
    checkEveryRank("all equal", std::vector<int>(24, 8), true);
    checkEveryRank("many duplicates",
                   {5, 1, 5, 3, 5, 2, 5, 4, 5, 1, 5, 2, 5, 0, 5, 0}, true);
    checkEveryRank("small mixed", {20, 3, 14, 7, 0, 18, 5, 11, 1, 16, 9}, true);
}

void testInvalidInputs() {
    const std::vector<std::vector<int>> inputs{{}, {42}, {3, 1, 2}};
    for (const std::vector<int>& values : inputs) {
        const std::array<std::size_t, 3> ranks{
            values.size(), values.size() + 1,
            std::numeric_limits<std::size_t>::max(),
        };
        for (const std::size_t k : ranks) {
            checkInvalid("invalid convenience overload", values, k, nullptr);
            for (const std::uint32_t seed : pivotSeeds) {
                std::mt19937 generator(seed);
                checkInvalid("invalid pivot_seed=" + std::to_string(seed),
                             values, k, &generator);
            }
        }
    }
}

void testExhaustiveInputs() {
    constexpr std::array<int, 3> alphabet{-1, 0, 1};
    std::size_t combinations = 1;
    for (std::size_t size = 1; size <= 6; ++size) {
        combinations *= alphabet.size();
        for (std::size_t encoding = 0; encoding < combinations; ++encoding) {
            std::vector<int> values(size);
            std::size_t remaining = encoding;
            for (int& value : values) {
                value = alphabet[remaining % alphabet.size()];
                remaining /= alphabet.size();
            }
            checkEveryRank("exhaustive encoding=" + std::to_string(encoding),
                           values);
        }
    }
}

void testGeneratedInputs() {
    std::mt19937 dataGenerator(dataSeed);
    std::uniform_int_distribution<int> narrow(-20, 20);
    std::uniform_int_distribution<int> broad(-1000000, 1000000);
    for (std::size_t size = 1; size <= 80; ++size) {
        for (int trial = 0; trial < 4; ++trial) {
            std::vector<int> values(size);
            for (int& value : values) {
                value = trial % 2 == 0 ? narrow(dataGenerator)
                                       : broad(dataGenerator);
            }
            checkEveryRank("generated data_seed=" + std::to_string(dataSeed) +
                               " trial=" + std::to_string(trial),
                           values);
        }
    }
}

void testLargeInputs() {
    std::mt19937 dataGenerator(dataSeed);
    std::uniform_int_distribution<int> duplicates(-3, 3);
    for (const std::size_t size : {std::size_t{1000}, std::size_t{10000}}) {
        const std::vector<std::size_t> ranks{
            0, 1, size / 4 - 1, size / 2 - 1,
            size / 2, 3 * size / 4 - 1, size - 2, size - 1,
        };
        const std::string context = "large data_seed=" + std::to_string(dataSeed) +
                                    " trial=0 ";
        std::vector<int> values(size);
        std::iota(values.begin(), values.end(), -static_cast<int>(size / 2));
        checkRanks(context + "ascending", values, ranks);
        std::reverse(values.begin(), values.end());
        checkRanks(context + "descending", values, ranks);
        std::shuffle(values.begin(), values.end(), dataGenerator);
        checkRanks(context + "shuffled distinct", values, ranks);
        for (int& value : values) {
            value = duplicates(dataGenerator);
        }
        checkRanks(context + "many duplicates", values, ranks);
        std::fill(values.begin(), values.end(), -7);
        checkRanks(context + "all equal", values, ranks);
    }
}

void testRepeatedSelections() {
    const std::vector<int> original{
        INT_MAX, 5, 1, 5, -3, INT_MIN, 0, 8, 5, -3,
        2, 7, 0, 2, 9, -1, 5, INT_MIN, INT_MAX, 4,
    };
    for (const std::uint32_t seed : pivotSeeds) {
        std::vector<int> values = original;
        std::mt19937 generator(seed);
        const std::string context = "repeated continuing pivot_seed=" +
                                    std::to_string(seed);
        const std::vector<std::size_t> ranks{
            0, values.size() - 1, values.size() / 2 - 1, values.size() / 2,
            0, values.size() / 2, values.size() - 1,
        };
        for (const std::size_t k : ranks) {
            checkSelection(context, values, k, &generator);
        }
        for (std::size_t k = 0; k < values.size(); ++k) {
            checkSelection(context + " ascending rank pass", values, k, &generator);
        }
        for (std::size_t k = values.size(); k > 0; --k) {
            checkSelection(context + " descending rank pass", values, k - 1,
                           &generator);
        }
    }

    std::vector<int> values = original;
    for (std::size_t k = 0; k < values.size(); ++k) {
        checkSelection("repeated convenience overload", values, k, nullptr);
    }
}

}  // namespace

int main() {
    testNamedInputs();
    testInvalidInputs();
    testExhaustiveInputs();
    testGeneratedInputs();
    testLargeInputs();
    testRepeatedSelections();

    if (failures != 0) {
        std::cerr << failures << " failure(s) across " << checks
                  << " selection checks.\n";
        return 1;
    }

    std::cout << "All randomised Quickselect tests passed (" << checks
              << " selection checks).\n";
    return 0;
}
