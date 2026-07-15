/**
 * @file winsorize.h
 * @brief Winsorization (tail trimming) for outlier handling
 *
 * Provides functions to winsorize data by replacing extreme values with
 * specified percentile values. This is useful for reducing the impact of
 * outliers in statistical analysis without completely removing them.
 *
 * Key features:
 * - In-place and out-of-place winsorization
 * - Symmetric and asymmetric percentile limits
 * - Batch processing for multiple datasets
 * - SIMD optimization for large datasets
 * - Thread-safe (no global state)
 *
 * Performance targets:
 * - Single array (100K elements): < 0.5ms
 * - Batch processing: > 200 million elements/second
 */

#ifndef FC_STATS_WINSORIZE_H
#define FC_STATS_WINSORIZE_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Winsorize array by replacing extreme values with percentile limits
 *
 * Replaces values below the lower percentile with the lower percentile value,
 * and values above the upper percentile with the upper percentile value.
 * This is an out-of-place operation (input array is not modified).
 *
 * Time complexity: O(n log n) due to sorting for percentile computation
 * Space complexity: O(n) for temporary sorted copy
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be > 0)
 * @param lower_pct Lower percentile (0.0 to 1.0, must be < upper_pct)
 * @param upper_pct Upper percentile (0.0 to 1.0, must be > lower_pct)
 * @param output Output array (must not be NULL, size >= n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note For symmetric winsorization, use lower_pct = p and upper_pct = 1-p
 * @note Example: lower_pct=0.05, upper_pct=0.95 for 5% symmetric winsorization
 */
FC_API fc_status_t fc_stats_winsorize_f64(
    const double* data,
    size_t n,
    double lower_pct,
    double upper_pct,
    double* output
);

/**
 * @brief In-place winsorization
 *
 * Same as fc_stats_winsorize_f64 but modifies the input array in place.
 * More memory efficient as it doesn't require output buffer.
 *
 * Time complexity: O(n log n)
 * Space complexity: O(n) for temporary sorted copy
 *
 * @param data Input/output array (must not be NULL, will be modified)
 * @param n Number of elements (must be > 0)
 * @param lower_pct Lower percentile (0.0 to 1.0, must be < upper_pct)
 * @param upper_pct Upper percentile (0.0 to 1.0, must be > lower_pct)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe (operates on caller-provided buffer)
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 */
FC_API fc_status_t
fc_stats_winsorize_inplace_f64(double* data, size_t n, double lower_pct, double upper_pct);

/**
 * @brief Batch winsorize multiple arrays
 *
 * Processes multiple independent datasets in a single call.
 * Each array is winsorized independently with the same percentile limits.
 *
 * Time complexity: O(num_arrays * n log n)
 * Space complexity: O(n) for temporary sorted copy (reused across arrays)
 *
 * @param data Array of input arrays (must not be NULL, each array must not be NULL)
 * @param n Number of elements in each array (must be > 0)
 * @param num_arrays Number of arrays to process (must be > 0)
 * @param lower_pct Lower percentile (0.0 to 1.0, must be < upper_pct)
 * @param upper_pct Upper percentile (0.0 to 1.0, must be > upper_pct)
 * @param output Array of output arrays (must not be NULL, each array must not be NULL, size >= n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note All input arrays must have the same length n
 */
FC_API fc_status_t fc_stats_winsorize_batch_f64(
    const double* const* data,
    size_t n,
    size_t num_arrays,
    double lower_pct,
    double upper_pct,
    double* const* output
);

FC_END_DECLS

#endif /* FC_STATS_WINSORIZE_H */
