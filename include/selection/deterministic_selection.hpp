#pragma once

#include <cstddef>
#include <vector>

namespace selection {

// Returns the kth smallest value, with zero-based k and duplicates counted as
// separate elements. The input may be rearranged, but its size and multiset of
// values are preserved; the resulting vector is not guaranteed to be sorted.
// Throws std::out_of_range for empty input or k >= values.size(), before changing
// the input. Worst-case running time is O(n), with O(log n) recursive stack space.
int deterministicSelect(std::vector<int>& values, std::size_t k);

}  // namespace selection

