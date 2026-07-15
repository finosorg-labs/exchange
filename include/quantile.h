/**
 * @file quantile.h
 * @brief Quantile computation using QuickSelect algorithm
 *
 * Provides efficient quantile (percentile) computation for double-precision arrays.
 * Uses QuickSelect (Hoare's selection algorithm) for O(n) average-case performance.
 */

#ifndef FC_QUANTILE_H
#define FC_QUANTILE_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute a single quantile from an array
 *
 * Uses QuickSelect algorithm for O(n) average-case performance.
 * The input array will be partially sorted (modified in-place).
 *
 * @param data Input array (will be modified)
 * @param n Number of elements
 * @param q Quantile to compute (0.0 to 1.0, e.g., 0.5 for median)
 * @param result Output quantile value
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n) average, O(n^2) worst case
 * @note Space complexity: O(1)
 * @note Thread safety: Not thread-safe (modifies input array)
 * @note The input array is modified (partially sorted)
 * @note For q=0.5, this computes the median
 * @note Uses linear interpolation for non-integer positions
 */
fc_status_t fc_stats_quantile_f64(double* data, size_t n, double q, double* result);

/**
 * @brief Compute multiple quantiles from an array
 *
 * More efficient than calling fc_stats_quantile_f64 multiple times.
 * The input array will be partially sorted (modified in-place).
 *
 * @param data Input array (will be modified)
 * @param n Number of elements
 * @param quantiles Array of quantile values to compute (0.0 to 1.0)
 * @param num_quantiles Number of quantiles to compute
 * @param results Output array for quantile values (must have space for num_quantiles)
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n * k) where k is num_quantiles
 * @note Space complexity: O(1)
 * @note Thread safety: Not thread-safe (modifies input array)
 * @note The input array is modified (partially sorted)
 * @note Quantiles should be sorted in ascending order for best performance
 */
fc_status_t fc_stats_quantiles_f64(
    double* data,
    size_t n,
    const double* quantiles,
    size_t num_quantiles,
    double* results
);

/**
 * @brief Compute median (50th percentile)
 *
 * Convenience function for computing the median.
 * The input array will be partially sorted (modified in-place).
 *
 * @param data Input array (will be modified)
 * @param n Number of elements
 * @param result Output median value
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n) average
 * @note Space complexity: O(1)
 * @note Thread safety: Not thread-safe (modifies input array)
 */
fc_status_t fc_stats_median_f64(double* data, size_t n, double* result);

/**
 * @brief Compute quartiles (Q1, Q2/median, Q3)
 *
 * Computes the three quartiles: 25th, 50th, and 75th percentiles.
 * The input array will be partially sorted (modified in-place).
 *
 * @param data Input array (will be modified)
 * @param n Number of elements
 * @param q1 Output first quartile (25th percentile)
 * @param q2 Output second quartile (50th percentile, median)
 * @param q3 Output third quartile (75th percentile)
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n)
 * @note Space complexity: O(1)
 * @note Thread safety: Not thread-safe (modifies input array)
 */
fc_status_t fc_stats_quartiles_f64(double* data, size_t n, double* q1, double* q2, double* q3);

FC_END_DECLS

#endif /* FC_QUANTILE_H */
