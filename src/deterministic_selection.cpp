#include "selection/deterministic_selection.hpp"

#include <algorithm>
#include <stdexcept>

namespace selection {

namespace {

struct EqualRange {
    std::size_t begin;
    std::size_t end;
};

EqualRange partitionAroundPivot(std::vector<int>& values, const std::size_t begin,
                                const std::size_t end, const int pivot) {
    std::size_t smallerEnd = begin;
    std::size_t current = begin;
    std::size_t greaterBegin = end;

    while (current < greaterBegin) {
        if (values[current] < pivot) {
            std::swap(values[smallerEnd], values[current]);
            ++smallerEnd;
            ++current;
        } else if (values[current] > pivot) {
            --greaterBegin;
            std::swap(values[current], values[greaterBegin]);
        } else {
            ++current;
        }
    }

    return {smallerEnd, greaterBegin};
}

int selectRange(std::vector<int>& values, std::size_t begin, std::size_t end,
                std::size_t k);

int choosePivot(std::vector<int>& values, const std::size_t begin,
                const std::size_t end) {
    const std::size_t length = end - begin;
    if (length <= 5) {
        std::sort(values.begin() + static_cast<std::ptrdiff_t>(begin),
                  values.begin() + static_cast<std::ptrdiff_t>(end));
        return values[begin + length / 2];
    }

    std::size_t medianCount = 0;
    for (std::size_t groupBegin = begin; groupBegin < end; groupBegin += 5) {
        const std::size_t groupEnd = std::min(groupBegin + 5, end);
        std::sort(values.begin() + static_cast<std::ptrdiff_t>(groupBegin),
                  values.begin() + static_cast<std::ptrdiff_t>(groupEnd));

        // Store group medians together so the recursive call uses one prefix.
        const std::size_t groupMedian = groupBegin + (groupEnd - groupBegin) / 2;
        std::swap(values[begin + medianCount], values[groupMedian]);
        ++medianCount;
    }

    const std::size_t medianOfMedians = begin + medianCount / 2;
    return selectRange(values, begin, begin + medianCount, medianOfMedians);
}

int selectRange(std::vector<int>& values, std::size_t begin, std::size_t end,
                const std::size_t k) {
    while (true) {
        if (end - begin <= 5) {
            std::sort(values.begin() + static_cast<std::ptrdiff_t>(begin),
                      values.begin() + static_cast<std::ptrdiff_t>(end));
            return values[k];
        }

        const int pivot = choosePivot(values, begin, end);
        const EqualRange equal = partitionAroundPivot(values, begin, end, pivot);

        if (k < equal.begin) {
            end = equal.begin;
        } else if (k >= equal.end) {
            begin = equal.end;
        } else {
            return values[k];
        }
    }
}

}  // namespace

int deterministicSelect(std::vector<int>& values, const std::size_t k) {
    if (values.empty() || k >= values.size()) {
        throw std::out_of_range("k must refer to an element in the array");
    }

    return selectRange(values, 0, values.size(), k);
}

}  // namespace selection

