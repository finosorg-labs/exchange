/**
 * @file rank.h
 * @brief Ranking computation with tie handling
 *
 * Provides efficient ranking (ordinal position) computation for double-precision arrays.
 * Supports multiple tie-handling methods commonly used in financial factor analysis.
 */

#ifndef FC_RANK_H
#define FC_RANK_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Tie handling methods for ranking
 */
typedef enum {
    FC_RANK_AVERAGE, /**< Average rank for tied values (default) */
    FC_RANK_MIN,     /**< Minimum rank for tied values */
    FC_RANK_MAX,     /**< Maximum rank for tied values */
    FC_RANK_FIRST,   /**< Rank by order of appearance (stable) */
    FC_RANK_DENSE    /**< Dense ranking (1,2,2,3 instead of 1,2,2,4) */
} fc_rank_method_t;

/**
 * @brief Compute ranks of array elements
 *
 * Assigns ranks to elements based on their sorted order.
 * Smaller values get lower ranks (rank 1 is the smallest).
 * Supports multiple tie-handling methods.
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param ranks Output array for ranks (must have space for n elements)
 * @param method Tie handling method
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n log n)
 * @note Space complexity: O(n) for temporary index array
 * @note Thread safety: Thread-safe (does not modify input)
 * @note NaN values are ranked last (highest rank)
 * @note For financial factor analysis, FC_RANK_AVERAGE is most common
 *
 * Example (FC_RANK_AVERAGE):
 *   Input:  [3.0, 1.0, 2.0, 2.0, 5.0]
 *   Output: [4.0, 1.0, 2.5, 2.5, 5.0]
 *
 * Example (FC_RANK_MIN):
 *   Input:  [3.0, 1.0, 2.0, 2.0, 5.0]
 *   Output: [4.0, 1.0, 2.0, 2.0, 5.0]
 *
 * Example (FC_RANK_DENSE):
 *   Input:  [3.0, 1.0, 2.0, 2.0, 5.0]
 *   Output: [3.0, 1.0, 2.0, 2.0, 4.0]
 */
fc_status_t fc_stats_rank_f64(const double* data, size_t n, double* ranks, fc_rank_method_t method);

/**
 * @brief Compute fractional ranks (normalized to [0, 1])
 *
 * Computes ranks and normalizes them to the range [0, 1].
 * Useful for cross-sectional factor analysis where different
 * universes have different sizes.
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param ranks Output array for normalized ranks (must have space for n elements)
 * @param method Tie handling method
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n log n)
 * @note Space complexity: O(n)
 * @note Thread safety: Thread-safe
 * @note Normalized ranks: (rank - 1) / (n - 1) for n > 1, 0.5 for n = 1
 *
 * Example (FC_RANK_AVERAGE):
 *   Input:  [3.0, 1.0, 2.0, 2.0, 5.0]
 *   Output: [0.75, 0.0, 0.375, 0.375, 1.0]
 */
fc_status_t fc_stats_rank_normalized_f64(
    const double* data,
    size_t n,
    double* ranks,
    fc_rank_method_t method
);

/**
 * @brief Compute ranks for multiple arrays (batch processing)
 *
 * Efficiently computes ranks for multiple independent arrays.
 * Useful for cross-sectional factor ranking across multiple factors.
 *
 * @param data Input arrays (num_arrays × n elements, row-major)
 * @param n Number of elements per array
 * @param num_arrays Number of arrays to rank
 * @param ranks Output arrays for ranks (num_arrays × n elements, row-major)
 * @param method Tie handling method
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(num_arrays × n log n)
 * @note Space complexity: O(n) temporary storage (reused across arrays)
 * @note Thread safety: Thread-safe
 * @note Each array is ranked independently
 *
 * Example use case: Rank 500 factors across 5000 stocks
 *   num_arrays = 500 (factors)
 *   n = 5000 (stocks)
 */
fc_status_t fc_stats_rank_batch_f64(
    const double* data,
    size_t n,
    size_t num_arrays,
    double* ranks,
    fc_rank_method_t method
);

FC_END_DECLS

#endif /* FC_RANK_H */
