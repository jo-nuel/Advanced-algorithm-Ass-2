#include "selection/randomised_quickselect.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace {

std::size_t failures = 0;
std::size_t checks = 0;

constexpr std::uint32_t referenceDataSeed = 14487692U;
constexpr std::uint32_t matrixRandomSeed = 20261016U;
constexpr std::uint32_t matrixNearlySortedSeed = 20261017U;
constexpr std::uint32_t matrixDuplicateSeed = 20261018U;
constexpr std::uint32_t broadStressSeed = 20261019U;
constexpr std::uint32_t largeStressSeed = 20261020U;
constexpr std::array<std::uint32_t, 4> pivotSeeds{
    0U, 1U, 42U, std::numeric_limits<std::uint32_t>::max(),
};

void fail(const std::string_view context, const std::size_t size,
          const std::size_t k, const std::string_view message) {
    std::cerr << context << " size=" << size << " k=" << k << ": "
              << message << '\n';
    ++failures;
}

// A null generator exercises the convenience overload.
void checkSelection(const std::string_view context, std::vector<int>& values,
                    const std::size_t k, std::mt19937* generator,
                    const std::optional<int> explicitExpected = std::nullopt) {
    ++checks;
    const std::size_t originalSize = values.size();
    if (k >= originalSize) {
        fail(context, originalSize, k, "valid-case rank is outside the fixture");
        return;
    }
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
        if (explicitExpected && actual != *explicitExpected) {
            fail(context, originalSize, k,
                 "explicit expected " + std::to_string(*explicitExpected) +
                     ", got " + std::to_string(actual));
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
        const std::string context = name + " pivot_seed=convenience";
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

void checkExplicitExpected(const std::string& name,
                           const std::vector<int>& original,
                           const std::size_t k, const int expected) {
    for (const std::uint32_t seed : pivotSeeds) {
        std::vector<int> values = original;
        std::mt19937 generator(seed);
        checkSelection(name + " pivot_seed=" + std::to_string(seed), values, k,
                       &generator, expected);
    }
}

void checkInvalidCase(const std::string& name, const std::vector<int>& original,
                      const std::size_t k) {
    checkInvalid(name + " pivot_seed=convenience", original, k, nullptr);
    for (const std::uint32_t seed : pivotSeeds) {
        std::mt19937 generator(seed);
        checkInvalid(name + " pivot_seed=" + std::to_string(seed), original, k,
                     &generator);
    }
}

// Exact reference fixtures from Jonathan's deterministic suite at
// 1b70859b1f5e161b3a12725c46d4d192fa86e2ca. Quickselect repeats valid calls
// across pivot seeds; the complete workloads therefore are not identical.
void testReferenceFixtures() {
    const std::string context = "group=reference trial=0 data_seed=not_applicable family=";
    checkExplicitExpected(context + "one-value", {42}, 0, 42);
    checkExplicitExpected(context + "unsorted", {9, 1, 4, 7, 3}, 2, 4);
    checkExplicitExpected(context + "negative", {-2, -8, 5, 0}, 1, -2);
    checkExplicitExpected(context + "duplicates", {4, 2, 4, 1, 4}, 3, 4);
    checkExplicitExpected(context + "larger-array",
                          {12, 4, 9, 1, 15, 7, 2, 10, 6}, 4, 7);
    checkExplicitExpected(context + "many-equal", {5, 1, 5, 3, 5, 2, 5, 4, 5},
                          6, 5);
    checkExplicitExpected(context + "all-equal", {8, 8, 8, 8, 8, 8, 8, 8}, 5, 8);

    const std::vector<int> values{8, 3, 6, 1, 5};
    const std::vector<int> expected{1, 3, 5, 6, 8};
    for (std::size_t k = 0; k < values.size(); ++k) {
        checkExplicitExpected(context + "every-small-k", values, k, expected[k]);
    }

    const std::vector<int> descending{11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    for (std::size_t k = 0; k < descending.size(); ++k) {
        checkExplicitExpected(context + "every-descending-k", descending, k,
                              static_cast<int>(k + 1));
    }

    const std::vector<int> severalGroups{
        20, 3,  14, 7,  0, 18, 5, 11, 1,  16, 9,
        2,  19, 6,  13, 4, 17, 8,  15, 10, 12,
    };
    for (std::size_t k = 0; k < severalGroups.size(); ++k) {
        checkExplicitExpected(context + "groups-of-five-with-remainder",
                              severalGroups, k, static_cast<int>(k));
    }

    checkInvalidCase(context + "empty-input", {}, 0);
    checkInvalidCase(context + "k-past-end", {1, 2, 3}, 3);
}

void testReferenceGeneratedInputs() {
    // Match the reference data calls and their order exactly. Reproduction of
    // distributions and shuffle assumes the same standard-library environment;
    // sharing a seed alone does not guarantee identical arrays across libraries.
    std::mt19937 referenceDataGenerator(referenceDataSeed);
    std::uniform_int_distribution<int> repeatedValues(-20, 20);
    for (std::size_t size = 1; size <= 80; ++size) {
        for (int trial = 0; trial < 4; ++trial) {
            std::vector<int> generated(size);
            for (int& value : generated) {
                value = repeatedValues(referenceDataGenerator);
            }
            checkEveryRank("group=reference family=random-narrow data_seed=" +
                               std::to_string(referenceDataSeed) + " trial=" +
                               std::to_string(trial),
                           generated);
        }
    }

    // Continue the same data RNG, without extra generation or shuffle calls.
    // Selection helpers use fresh pivot RNGs and never receive this generator.
    std::vector<int> largeShuffle(1000);
    std::iota(largeShuffle.begin(), largeShuffle.end(), -500);
    std::shuffle(largeShuffle.begin(), largeShuffle.end(), referenceDataGenerator);
    for (const std::size_t k : {0U, 1U, 249U, 499U, 500U, 749U, 998U, 999U}) {
        checkExplicitExpected("group=reference family=shuffled-distinct trial=0 data_seed=" +
                                  std::to_string(referenceDataSeed),
                              largeShuffle, k, static_cast<int>(k) - 500);
    }
}

// Additional Quickselect stress tests and proposed benchmark-family fixtures.
// These are correctness checks, not timing benchmarks or additional coverage
// currently claimed for the deterministic suite.
void testNamedInputs() {
    const std::string context =
        "group=stress trial=0 data_seed=not_applicable family=";
    checkEveryRank(context + "singleton", {42}, true);
    checkEveryRank(context + "two-ascending", {1, 2}, true);
    checkEveryRank(context + "two-descending", {2, 1}, true);
    checkEveryRank(context + "two-equal", {7, 7}, true);
    checkEveryRank(context + "unsorted-odd", {9, 1, 4, 7, 3}, true);
    checkEveryRank(context + "unsorted-even", {9, 1, 4, 7, 3, 2}, true);
    checkEveryRank(context + "ascending", {-4, -3, -2, -1, 0, 1, 2, 3}, true);
    checkEveryRank(context + "descending", {3, 2, 1, 0, -1, -2, -3, -4}, true);
    checkEveryRank(context + "integer-extremes",
                   {INT_MAX, -2, INT_MIN, 0, 5, -8, INT_MAX, INT_MIN}, true);
    checkEveryRank(context + "all-equal", std::vector<int>(24, 8), true);
    checkEveryRank(context + "many-duplicates",
                   {5, 1, 5, 3, 5, 2, 5, 4, 5, 1, 5, 2, 5, 0, 5, 0}, true);
    checkEveryRank(context + "small-mixed",
                   {20, 3, 14, 7, 0, 18, 5, 11, 1, 16, 9}, true);
}

void testInvalidInputs() {
    const std::vector<std::vector<int>> inputs{{}, {42}, {3, 1, 2}};
    for (const std::vector<int>& values : inputs) {
        const std::array<std::size_t, 3> ranks{
            values.size(), values.size() + 1,
            std::numeric_limits<std::size_t>::max(),
        };
        for (const std::size_t k : ranks) {
            checkInvalidCase("group=stress family=invalid trial=0 data_seed=not_applicable",
                             values, k);
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
            checkEveryRank("group=stress family=exhaustive data_seed=not_applicable trial=" +
                               std::to_string(encoding),
                           values);
        }
    }
}

void testBroadRangeStress() {
    std::mt19937 dataGenerator(broadStressSeed);
    std::uniform_int_distribution<int> broad(-1000000, 1000000);
    for (std::size_t size = 1; size <= 80; ++size) {
        for (int trial = 0; trial < 4; ++trial) {
            std::vector<int> values(size);
            for (int& value : values) {
                value = broad(dataGenerator);
            }
            checkEveryRank("group=broad-stress family=random-broad data_seed=" +
                               std::to_string(broadStressSeed) +
                               " trial=" + std::to_string(trial),
                           values);
        }
    }
}

void testLargeInputs() {
    std::mt19937 dataGenerator(largeStressSeed);
    std::uniform_int_distribution<int> duplicates(-3, 3);
    for (const std::size_t size : {std::size_t{1000}, std::size_t{10000}}) {
        const std::vector<std::size_t> ranks{
            0, 1, size / 4 - 1, size / 2 - 1,
            size / 2, 3 * size / 4 - 1, size - 2, size - 1,
        };
        const std::string context = "group=large-stress data_seed=" +
                                    std::to_string(largeStressSeed) +
                                    " trial=0 family=";
        std::vector<int> values(size);
        std::iota(values.begin(), values.end(), -static_cast<int>(size / 2));
        checkRanks(context + "ascending", values, ranks);
        std::reverse(values.begin(), values.end());
        checkRanks(context + "descending", values, ranks);
        std::shuffle(values.begin(), values.end(), dataGenerator);
        checkRanks(context + "shuffled-distinct", values, ranks);
        for (int& value : values) {
            value = duplicates(dataGenerator);
        }
        checkRanks(context + "many-duplicates", values, ranks);
        std::fill(values.begin(), values.end(), -7);
        checkRanks(context + "all-equal", values, ranks);
    }
}

std::vector<std::size_t> supplementaryRanks(const std::size_t size) {
    if (size <= 80) {
        std::vector<std::size_t> ranks(size);
        std::iota(ranks.begin(), ranks.end(), std::size_t{0});
        return ranks;
    }

    std::vector<std::size_t> ranks{
        0, 1, size / 4 - 1, size / 4, (size - 1) / 2, size / 2,
        3 * size / 4 - 1, 3 * size / 4, size - 2, size - 1,
    };
    ranks.erase(std::remove_if(ranks.begin(), ranks.end(),
                               [size](const std::size_t k) { return k >= size; }),
                ranks.end());
    std::sort(ranks.begin(), ranks.end());
    ranks.erase(std::unique(ranks.begin(), ranks.end()), ranks.end());
    return ranks;
}

void testSupplementaryMatrix() {
    constexpr std::array<std::size_t, 16> sizes{
        1, 2, 3, 4, 5, 8, 16, 32, 64, 80,
        128, 256, 512, 1000, 4096, 10000,
    };
    for (const std::size_t size : sizes) {
        const std::vector<std::size_t> ranks = supplementaryRanks(size);
        const std::string context = "group=supplementary-matrix trial=0 family=";

        // Family-local RNGs reset for each size, so adding another family or
        // pivot-seed repetition cannot change this fixture's generated values.
        std::mt19937 randomGenerator(matrixRandomSeed);
        std::uniform_int_distribution<int> broad(-1000000, 1000000);
        std::vector<int> randomValues(size);
        for (int& value : randomValues) {
            value = broad(randomGenerator);
        }
        checkRanks(context + "random-broad data_seed=" +
                       std::to_string(matrixRandomSeed),
                   randomValues, ranks, true);

        std::vector<int> ascending(size);
        std::iota(ascending.begin(), ascending.end(), -static_cast<int>(size / 2));
        checkRanks(context + "sorted data_seed=not_applicable", ascending, ranks,
                   true);

        std::vector<int> descending = ascending;
        std::reverse(descending.begin(), descending.end());
        checkRanks(context + "reverse-sorted data_seed=not_applicable", descending,
                   ranks, true);

        std::vector<int> nearlySorted = ascending;
        if (size >= 2) {
            // Exactly max(1, floor(n/100)) disjoint adjacent swaps: shuffle IDs
            // of pairs (0,1), (2,3), ... and use that many distinct pairs. This
            // is about 1% swaps (about 2% positions); a singleton is unchanged.
            std::vector<std::size_t> pairs(size / 2);
            std::iota(pairs.begin(), pairs.end(), std::size_t{0});
            std::mt19937 nearlySortedGenerator(matrixNearlySortedSeed);
            std::shuffle(pairs.begin(), pairs.end(), nearlySortedGenerator);
            const std::size_t swaps = std::max(std::size_t{1}, size / 100);
            for (std::size_t swap = 0; swap < swaps; ++swap) {
                const std::size_t begin = 2 * pairs[swap];
                std::swap(nearlySorted[begin], nearlySorted[begin + 1]);
            }
        }
        checkRanks(context + "nearly-sorted data_seed=" +
                       std::to_string(matrixNearlySortedSeed),
                   nearlySorted, ranks, true);

        std::mt19937 duplicateGenerator(matrixDuplicateSeed);
        std::uniform_int_distribution<int> narrow(-3, 3);
        std::vector<int> duplicateValues(size);
        for (int& value : duplicateValues) {
            value = narrow(duplicateGenerator);
        }
        checkRanks(context + "many-duplicates data_seed=" +
                       std::to_string(matrixDuplicateSeed),
                   duplicateValues, ranks, true);

        checkRanks(context + "all-equal data_seed=not_applicable",
                   std::vector<int>(size, -7), ranks, true);
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
        const std::string context =
            "group=repeated family=mixed trial=0 data_seed=not_applicable pivot_seed=" +
            std::to_string(seed);
        const std::vector<std::size_t> ranks{
            0, values.size() - 1, values.size() / 2 - 1, values.size() / 2,
            0, values.size() / 2, values.size() - 1,
        };
        for (const std::size_t k : ranks) {
            checkSelection(context, values, k, &generator);
        }
        for (std::size_t k = 0; k < original.size(); ++k) {
            checkSelection(context + " ascending rank pass", values, k, &generator);
        }
        for (std::size_t k = original.size(); k > 0; --k) {
            checkSelection(context + " descending rank pass", values, k - 1,
                           &generator);
        }
    }

    std::vector<int> values = original;
    for (std::size_t k = 0; k < original.size(); ++k) {
        checkSelection("group=repeated family=mixed trial=0 data_seed=not_applicable pivot_seed=convenience",
                       values, k, nullptr);
    }
}

}  // namespace

int main() {
    testReferenceFixtures();
    testReferenceGeneratedInputs();
    testNamedInputs();
    testInvalidInputs();
    testExhaustiveInputs();
    testBroadRangeStress();
    testLargeInputs();
    testSupplementaryMatrix();
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
