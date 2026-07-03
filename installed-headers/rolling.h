/**
 * @file rolling.h
 * @brief Rolling window statistics with SIMD optimization
 *
 * Provides high-performance rolling (sliding) window statistics for time series data.
 * Supports mean, variance, standard deviation, min, max, and sum computations over
 * fixed-size windows with SIMD acceleration.
 *
 * Key features:
 * - Fixed-size rolling windows
 * - Numerically stable variance computation (extended Welford's algorithm)
 * - SIMD optimization for large datasets
 * - Support for both sample and population variance
 * - Thread-safe (no global state)
 *
 * Performance targets:
 * - 1M elements with window=250: < 5ms (vs Go > 25ms)
 * - Throughput: > 200 million elements/second on AVX2
 *
 * Time complexity: O(n) for all operations
 * Space complexity: O(1) auxiliary space (output array provided by caller)
 */

#ifndef FC_STATS_ROLLING_H
#define FC_STATS_ROLLING_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute rolling window mean
 *
 * Computes the mean over a sliding window of fixed size.
 * For positions where the full window extends before the start of the array,
 * only available elements are used (partial window).
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be > 0)
 * @param window_size Window size (must be > 0 and <= n)
 * @param output Output array for rolling means (must not be NULL, size >= n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note output[i] contains mean of elements [max(0, i-window_size+1), i]
 */
FC_API fc_status_t
fc_stats_rolling_mean_f64(const double* data, size_t n, size_t window_size, double* output);

/**
 * @brief Compute rolling window variance
 *
 * Computes the variance over a sliding window of fixed size using
 * an extended Welford's algorithm for numerical stability.
 * For positions where the full window extends before the start of the array,
 * only available elements are used (partial window).
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be > 0)
 * @param window_size Window size (must be > 1 and <= n)
 * @param output Output array for rolling variances (must not be NULL, size >= n)
 * @param sample If 1, compute sample variance (n-1); if 0, population variance (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note output[i] contains variance of elements [max(0, i-window_size+1), i]
 * @note For windows with < 2 elements, variance is set to 0.0
 */
FC_API fc_status_t fc_stats_rolling_variance_f64(
    const double* data,
    size_t n,
    size_t window_size,
    double* output,
    int sample
);

/**
 * @brief Compute rolling window standard deviation
 *
 * Computes the standard deviation (sqrt of variance) over a sliding window.
 * Convenience function that internally calls fc_stats_rolling_variance_f64.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be > 0)
 * @param window_size Window size (must be > 1 and <= n)
 * @param output Output array for rolling stddevs (must not be NULL, size >= n)
 * @param sample If 1, compute sample stddev (n-1); if 0, population stddev (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note output[i] contains stddev of elements [max(0, i-window_size+1), i]
 */
FC_API fc_status_t fc_stats_rolling_stddev_f64(
    const double* data,
    size_t n,
    size_t window_size,
    double* output,
    int sample
);

/**
 * @brief Compute rolling window sum
 *
 * Computes the sum over a sliding window of fixed size.
 * Uses Kahan summation for numerical stability in SIMD implementations.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be > 0)
 * @param window_size Window size (must be > 0 and <= n)
 * @param output Output array for rolling sums (must not be NULL, size >= n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note output[i] contains sum of elements [max(0, i-window_size+1), i]
 */
FC_API fc_status_t
fc_stats_rolling_sum_f64(const double* data, size_t n, size_t window_size, double* output);

/**
 * @brief Compute rolling window minimum
 *
 * Computes the minimum value over a sliding window of fixed size.
 * Uses a deque-based algorithm for O(n) time complexity.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be > 0)
 * @param window_size Window size (must be > 0 and <= n)
 * @param output Output array for rolling minimums (must not be NULL, size >= n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note output[i] contains min of elements [max(0, i-window_size+1), i]
 */
FC_API fc_status_t
fc_stats_rolling_min_f64(const double* data, size_t n, size_t window_size, double* output);

/**
 * @brief Compute rolling window maximum
 *
 * Computes the maximum value over a sliding window of fixed size.
 * Uses a deque-based algorithm for O(n) time complexity.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be > 0)
 * @param window_size Window size (must be > 0 and <= n)
 * @param output Output array for rolling maximums (must not be NULL, size >= n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note output[i] contains max of elements [max(0, i-window_size+1), i]
 */
FC_API fc_status_t
fc_stats_rolling_max_f64(const double* data, size_t n, size_t window_size, double* output);

FC_END_DECLS

#endif /* FC_STATS_ROLLING_H */
