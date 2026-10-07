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
                const std::size_t k) {
    while (true) {
        if (end - begin <= 5) {
            std::sort(values.begin() + static_cast<std::ptrdiff_t>(begin),
                      values.begin() + static_cast<std::ptrdiff_t>(end));
            return values[k];
        }

        // This pivot is temporary while the partitioning step is tested.
        const int pivot = values[begin + (end - begin) / 2];
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

