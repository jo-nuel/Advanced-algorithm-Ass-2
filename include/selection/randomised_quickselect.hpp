#pragma once

#include <cstddef>
#include <random>
#include <vector>

namespace selection {

// Returns the kth smallest value, with zero-based k and duplicates counted as
// separate elements. The input may be rearranged, but its size and multiset of
// values are preserved; the resulting vector is not guaranteed to be sorted.
// Throws std::out_of_range for empty input or k >= values.size(), before changing
// the input. Expected running time is O(n), worst-case running time is O(n^2),
// and auxiliary algorithm storage is O(1).
// Initialises a local generator and delegates to the generator overload.
int randomisedSelect(std::vector<int>& values, std::size_t k);

// Has the same selection contract and uses the supplied generator's current
// state without reseeding it. A fixed seed supports reproducible testing in the
// same standard-library environment; identical pivot sequences across different
// standard-library implementations are not guaranteed.
int randomisedSelect(std::vector<int>& values, std::size_t k,
                     std::mt19937& generator);

}  // namespace selection
