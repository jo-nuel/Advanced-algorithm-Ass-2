#include "benchmark/input_generator.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
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

void checkSameMultiset(std::vector<int> first, std::vector<int> second,
                       const std::string_view message) {
    std::sort(first.begin(), first.end());
    std::sort(second.begin(), second.end());
    check(first == second, message);
}

}  // namespace

int main() {
    using selection_benchmark::InputFamily;
    using selection_benchmark::generateInput;

    check(selection_benchmark::benchmarkCsvHeader ==
              "family,n,k,input_seed,pivot_seed,algorithm,repetition,time_ns,result",
          "CSV header changed unexpectedly");

    check(selection_benchmark::inputFamilyName(InputFamily::RandomUnique) ==
              "random-unique",
          "random unique family name is wrong");
    check(selection_benchmark::inputFamilyName(InputFamily::RandomDuplicates) ==
              "random-duplicates",
          "random duplicates family name is wrong");
    check(selection_benchmark::inputFamilyName(InputFamily::NearlySorted) ==
              "nearly-sorted",
          "nearly sorted family name is wrong");

    constexpr std::array families{
        InputFamily::RandomUnique,     InputFamily::RandomDuplicates,
        InputFamily::Sorted,           InputFamily::ReverseSorted,
        InputFamily::NearlySorted,     InputFamily::AllEqual,
    };
    for (const InputFamily family : families) {
        check(generateInput(family, 0, 10U).empty(),
              "zero-sized input should be empty");
    }

    const std::vector<int> sorted = generateInput(InputFamily::Sorted, 100, 10U);
    check(std::is_sorted(sorted.begin(), sorted.end()),
          "sorted family is not sorted");

    const std::vector<int> reversed =
        generateInput(InputFamily::ReverseSorted, 100, 10U);
    check(std::is_sorted(reversed.rbegin(), reversed.rend()),
          "reverse-sorted family is not descending");
    checkSameMultiset(sorted, reversed,
                      "sorted and reverse-sorted families differ in values");

    const std::vector<int> firstRandom =
        generateInput(InputFamily::RandomUnique, 100, 42U);
    const std::vector<int> repeatedRandom =
        generateInput(InputFamily::RandomUnique, 100, 42U);
    const std::vector<int> differentRandom =
        generateInput(InputFamily::RandomUnique, 100, 43U);
    check(firstRandom == repeatedRandom,
          "the same seed did not reproduce the random input");
    check(firstRandom != differentRandom,
          "different seeds produced the same random ordering");
    checkSameMultiset(sorted, firstRandom,
                      "random unique family changed the source values");

    const std::vector<int> duplicates =
        generateInput(InputFamily::RandomDuplicates, 1000, 50U);
    check(std::all_of(duplicates.begin(), duplicates.end(),
                      [](const int value) { return value >= -3 && value <= 3; }),
          "duplicate family produced a value outside its documented range");

    const std::vector<int> nearlySorted =
        generateInput(InputFamily::NearlySorted, 100, 60U);
    check(nearlySorted != sorted, "nearly sorted family made no swaps");
    checkSameMultiset(sorted, nearlySorted,
                      "nearly sorted family changed the source values");

    const std::vector<int> allEqual =
        generateInput(InputFamily::AllEqual, 100, 70U);
    check(std::all_of(allEqual.begin(), allEqual.end(),
                      [](const int value) { return value == -7; }),
          "all-equal family contains different values");

    try {
        static_cast<void>(generateInput(
            InputFamily::Sorted,
            static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1U, 1U));
        check(false, "oversized input should throw std::length_error");
    } catch (const std::length_error&) {
    } catch (...) {
        check(false, "oversized input threw the wrong exception");
    }

    if (failures != 0) {
        std::cerr << failures << " benchmark input test(s) failed.\n";
        return 1;
    }

    std::cout << "All benchmark input tests passed.\n";
    return 0;
}

