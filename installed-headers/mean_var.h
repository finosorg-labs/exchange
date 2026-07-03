/**
 * @file mean_var.h
 * @brief Batch mean and variance computation with SIMD optimization
 *
 * Provides high-performance batch statistical functions for computing mean,
 * variance, and standard deviation using Welford's numerically stable algorithm.
 * Supports SIMD acceleration (AVX2, SSE4.2) with runtime dispatch.
 *
 * Key features:
 * - Numerically stable Welford's algorithm
 * - Batch processing for multiple datasets
 * - SIMD optimization for large datasets
 * - Support for both sample and population variance
 * - Thread-safe (no global state)
 *
 * Performance targets:
 * - 5000 groups × 250 elements: < 0.3ms (vs Go > 1.5ms)
 * - Throughput: > 1 billion elements/second on AVX2
 */

#ifndef FC_STATS_MEAN_VAR_H
#define FC_STATS_MEAN_VAR_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute mean of a single array
 *
 * Uses Kahan summation for numerical stability in SIMD implementations.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be > 0)
 * @param mean Output mean value (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 */
FC_API fc_status_t fc_stats_mean_f64(const double* data, size_t n, double* mean);

/**
 * @brief Compute variance of a single array
 *
 * Scalar implementation uses Welford's online algorithm for numerical stability.
 * SIMD implementations use two-pass algorithm (mean first, then variance) for
 * better vectorization efficiency. Both approaches provide good numerical stability.
 * Computes sample variance by default (divides by n-1).
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be >= 2 for sample variance)
 * @param variance Output variance value (must not be NULL)
 * @param sample If 1, compute sample variance (n-1); if 0, population variance (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 */
FC_API fc_status_t
fc_stats_variance_f64(const double* data, size_t n, double* variance, int sample);

/**
 * @brief Compute mean and variance simultaneously
 *
 * More efficient than calling mean and variance separately.
 * Scalar implementation uses Welford's online algorithm for numerical stability.
 * SIMD implementations use two-pass algorithm for better vectorization.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be >= 2 for sample variance)
 * @param mean Output mean value (must not be NULL)
 * @param variance Output variance value (must not be NULL)
 * @param sample If 1, compute sample variance (n-1); if 0, population variance (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 */
FC_API fc_status_t fc_stats_mean_variance_f64(
    const double* data,
    size_t n,
    double* mean,
    double* variance,
    int sample
);

/**
 * @brief Compute standard deviation of a single array
 *
 * Convenience function that computes sqrt(variance).
 * Scalar implementation uses Welford's algorithm internally.
 * SIMD implementations use two-pass algorithm.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param n Number of elements (must be >= 2 for sample stddev)
 * @param stddev Output standard deviation value (must not be NULL)
 * @param sample If 1, compute sample stddev (n-1); if 0, population stddev (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 */
FC_API fc_status_t fc_stats_stddev_f64(const double* data, size_t n, double* stddev, int sample);

/**
 * @brief Batch compute mean for multiple arrays
 *
 * Processes multiple independent datasets in a single call.
 * Uses SIMD optimization when beneficial (n >= 64).
 *
 * Time complexity: O(num_arrays * n)
 * Space complexity: O(1)
 *
 * @param data Array of input arrays (must not be NULL, each array must not be NULL)
 * @param n Number of elements in each array (must be > 0)
 * @param num_arrays Number of arrays to process (must be > 0)
 * @param means Output array of mean values (must not be NULL, size >= num_arrays)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note All input arrays must have the same length n
 */
FC_API fc_status_t
fc_stats_mean_batch_f64(const double* const* data, size_t n, size_t num_arrays, double* means);

/**
 * @brief Batch compute variance for multiple arrays
 *
 * Processes multiple independent datasets in a single call.
 * Uses SIMD optimization when beneficial (n >= 64).
 *
 * Time complexity: O(num_arrays * n)
 * Space complexity: O(1)
 *
 * @param data Array of input arrays (must not be NULL, each array must not be NULL)
 * @param n Number of elements in each array (must be >= 2 for sample variance)
 * @param num_arrays Number of arrays to process (must be > 0)
 * @param variances Output array of variance values (must not be NULL, size >= num_arrays)
 * @param sample If 1, compute sample variance (n-1); if 0, population variance (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note All input arrays must have the same length n
 */
FC_API fc_status_t fc_stats_variance_batch_f64(
    const double* const* data,
    size_t n,
    size_t num_arrays,
    double* variances,
    int sample
);

/**
 * @brief Batch compute mean and variance for multiple arrays
 *
 * Most efficient batch operation, processes multiple datasets simultaneously.
 * Uses SIMD optimization when beneficial (n >= 64).
 *
 * Time complexity: O(num_arrays * n)
 * Space complexity: O(1)
 *
 * @param data Array of input arrays (must not be NULL, each array must not be NULL)
 * @param n Number of elements in each array (must be >= 2 for sample variance)
 * @param num_arrays Number of arrays to process (must be > 0)
 * @param means Output array of mean values (must not be NULL, size >= num_arrays)
 * @param variances Output array of variance values (must not be NULL, size >= num_arrays)
 * @param sample If 1, compute sample variance (n-1); if 0, population variance (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note All input arrays must have the same length n
 */
FC_API fc_status_t fc_stats_mean_variance_batch_f64(
    const double* const* data,
    size_t n,
    size_t num_arrays,
    double* means,
    double* variances,
    int sample
);

FC_END_DECLS

#endif /* FC_STATS_MEAN_VAR_H */
