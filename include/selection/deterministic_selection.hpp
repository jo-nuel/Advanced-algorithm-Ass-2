#pragma once

#include <cstddef>
#include <vector>

namespace selection {

// Rearranges values and returns the kth smallest value using zero-based k.
// Throws std::out_of_range when values is empty or k is outside the array.
int deterministicSelect(std::vector<int>& values, std::size_t k);

}  // namespace selection

