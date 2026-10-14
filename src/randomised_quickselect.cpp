#include "selection/randomised_quickselect.hpp"

#include <stdexcept>
#include <utility>

namespace selection {

namespace {

struct EqualRange {
    std::size_t begin;
    std::size_t end;
};

void validateRank(const std::vector<int>& values, const std::size_t k) {
    if (values.empty() || k >= values.size()) {
        throw std::out_of_range("k must refer to an element in the array");
    }
}

EqualRange partitionAroundPivot(std::vector<int>& values, const std::size_t begin,
                                const std::size_t end, const int pivot) {
    std::size_t smallerEnd = begin;
    std::size_t current = begin;
    std::size_t greaterBegin = end;

    // [begin, smallerEnd) is less than the pivot, [smallerEnd, current) is equal,
    // [current, greaterBegin) is unclassified, and [greaterBegin, end) is greater.
    while (current < greaterBegin) {
        if (values[current] < pivot) {
            std::swap(values[smallerEnd], values[current]);
            ++smallerEnd;
            ++current;
        } else if (values[current] > pivot) {
            --greaterBegin;
            std::swap(values[current], values[greaterBegin]);
            // The swapped-in value is unclassified, so inspect it before advancing.
        } else {
            ++current;
        }
    }

    return {smallerEnd, greaterBegin};
}

}  // namespace

int randomisedSelect(std::vector<int>& values, const std::size_t k) {
    // Validate before requesting entropy so invalid input always throws out_of_range.
    validateRank(values, k);
    std::mt19937 generator(std::random_device{}());
    return randomisedSelect(values, k, generator);
}

int randomisedSelect(std::vector<int>& values, const std::size_t k,
                     std::mt19937& generator) {
    validateRank(values, k);

    std::size_t begin = 0;
    std::size_t end = values.size();

    // k stays an absolute index, and the active range always satisfies begin <= k < end.
    while (true) {
        std::uniform_int_distribution<std::size_t> chooseIndex(begin, end - 1);
        // Copy the pivot value so partition swaps cannot change the chosen pivot.
        const int pivot = values[chooseIndex(generator)];
        const EqualRange equal = partitionAroundPivot(values, begin, end, pivot);

        // Continue only in the partition containing k. The equal region contains
        // the chosen pivot, so excluding it strictly reduces the active range.
        if (k < equal.begin) {
            end = equal.begin;
        } else if (k >= equal.end) {
            begin = equal.end;
        } else {
            // All pivot duplicates are handled together, including all-equal input.
            return pivot;
        }
    }
}

}  // namespace selection
