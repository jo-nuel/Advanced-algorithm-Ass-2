#include "selection/deterministic_selection.hpp"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void expectEqual(const std::string_view name, std::vector<int> values,
                 const std::size_t k, const int expected) {
    try {
        const int actual = selection::deterministicSelect(values, k);
        if (actual != expected) {
            std::cerr << name << ": expected " << expected << ", got " << actual
                      << '\n';
            ++failures;
        }
    } catch (const std::exception& error) {
        std::cerr << name << ": unexpected exception: " << error.what() << '\n';
        ++failures;
    }
}

void expectOutOfRange(const std::string_view name, std::vector<int> values,
                      const std::size_t k) {
    try {
        static_cast<void>(selection::deterministicSelect(values, k));
        std::cerr << name << ": expected std::out_of_range\n";
        ++failures;
    } catch (const std::out_of_range&) {
        return;
    } catch (const std::exception& error) {
        std::cerr << name << ": wrong exception: " << error.what() << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    expectEqual("one value", {42}, 0, 42);
    expectEqual("unsorted values", {9, 1, 4, 7, 3}, 2, 4);
    expectEqual("negative values", {-2, -8, 5, 0}, 1, -2);
    expectEqual("duplicate values", {4, 2, 4, 1, 4}, 3, 4);
    expectEqual("larger array", {12, 4, 9, 1, 15, 7, 2, 10, 6}, 4, 7);
    expectEqual("many equal values", {5, 1, 5, 3, 5, 2, 5, 4, 5}, 6, 5);
    expectEqual("all equal values", {8, 8, 8, 8, 8, 8, 8, 8}, 5, 8);

    const std::vector<int> values{8, 3, 6, 1, 5};
    const std::vector<int> sorted{1, 3, 5, 6, 8};
    for (std::size_t k = 0; k < values.size(); ++k) {
        expectEqual("every small k", values, k, sorted[k]);
    }

    const std::vector<int> descending{11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    for (std::size_t k = 0; k < descending.size(); ++k) {
        expectEqual("every descending k", descending, k, static_cast<int>(k + 1));
    }

    const std::vector<int> severalGroups{
        20, 3,  14, 7,  0, 18, 5, 11, 1,  16, 9,
        2,  19, 6,  13, 4, 17, 8,  15, 10, 12,
    };
    for (std::size_t k = 0; k < severalGroups.size(); ++k) {
        expectEqual("groups of five with remainder", severalGroups, k,
                    static_cast<int>(k));
    }

    expectOutOfRange("empty input", {}, 0);
    expectOutOfRange("k past the end", {1, 2, 3}, 3);

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All deterministic selection tests passed.\n";
    return 0;
}

