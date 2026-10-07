#include "selection/deterministic_selection.hpp"

#include <algorithm>
#include <stdexcept>

namespace selection {

int deterministicSelect(std::vector<int>& values, const std::size_t k) {
    if (values.empty() || k >= values.size()) {
        throw std::out_of_range("k must refer to an element in the array");
    }

    if (values.size() > 5) {
        throw std::logic_error("selection for larger arrays is not implemented yet");
    }

    std::sort(values.begin(), values.end());
    return values[k];
}

}  // namespace selection

