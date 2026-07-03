/**
 * @file weighted.h
 * @brief Weighted statistical computations
 *
 * Provides weighted mean, population variance, and standard deviation for
 * double-precision data. Weights are non-negative and may be zero as long as
 * the total weight is positive.
 */

#ifndef FC_STATS_weighted_H
#define FC_STATS_weighted_H

#include "error.h"
#include "fc_bigfloat.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute weighted sum of a single array
 *
 * Computes Σ(data[i] * weights[i]) using Kahan compensated summation for
 * improved numerical stability.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param weights Weight array (must not be NULL, all weights must be >= 0)
 * @param n Number of elements (must be > 0)
 * @param sum Output weighted sum value (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Returns FC_ERR_NAN_INPUT if any data value or weight is NaN
 * @note Returns FC_ERR_INVALID_ARG if weights contain negative values or infinite values
 */
FC_API fc_status_t
fc_stats_weighted_sum_f64(const double* data, const double* weights, size_t n, double* sum);

/**
 * @brief Compute weighted mean of a single array
 *
 * Computes Σ(data[i] * weights[i]) / Σ(weights[i]) using Kahan compensated
 * summation for improved numerical stability.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param weights Weight array (must not be NULL, all weights must be >= 0)
 * @param n Number of elements (must be > 0)
 * @param mean Output weighted mean value (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Returns FC_ERR_NAN_INPUT if any data value or weight is NaN
 * @note Returns FC_ERR_INVALID_ARG if total weight is zero
 */
FC_API fc_status_t
fc_stats_weighted_mean_f64(const double* data, const double* weights, size_t n, double* mean);

/**
 * @brief Compute weighted population variance of a single array
 *
 * Computes Σ(weights[i] * (data[i] - mean)^2) / Σ(weights[i]). This function
 * intentionally computes population weighted variance only.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param weights Weight array (must not be NULL, all weights must be >= 0)
 * @param n Number of elements (must be > 0)
 * @param variance Output weighted population variance (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Returns FC_ERR_NAN_INPUT if any data value or weight is NaN
 * @note Returns FC_ERR_INVALID_ARG if total weight is zero
 */
FC_API fc_status_t fc_stats_weighted_variance_f64(
    const double* data,
    const double* weights,
    size_t n,
    double* variance
);

/**
 * @brief Compute weighted mean and population variance simultaneously
 *
 * More efficient than calling weighted mean and weighted variance separately.
 * The variance is population weighted variance:
 * Σ(weights[i] * (data[i] - mean)^2) / Σ(weights[i]).
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param weights Weight array (must not be NULL, all weights must be >= 0)
 * @param n Number of elements (must be > 0)
 * @param mean Output weighted mean value (must not be NULL)
 * @param variance Output weighted population variance (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Returns FC_ERR_NAN_INPUT if any data value or weight is NaN
 * @note Returns FC_ERR_INVALID_ARG if total weight is zero
 */
FC_API fc_status_t fc_stats_weighted_mean_variance_f64(
    const double* data,
    const double* weights,
    size_t n,
    double* mean,
    double* variance
);

/**
 * @brief Compute weighted population standard deviation of a single array
 *
 * Convenience function that computes sqrt(weighted population variance).
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param data Input array (must not be NULL)
 * @param weights Weight array (must not be NULL, all weights must be >= 0)
 * @param n Number of elements (must be > 0)
 * @param stddev Output weighted population standard deviation (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Returns FC_ERR_NAN_INPUT if any data value or weight is NaN
 * @note Returns FC_ERR_INVALID_ARG if total weight is zero
 */
FC_API fc_status_t
fc_stats_weighted_stddev_f64(const double* data, const double* weights, size_t n, double* stddev);

/**
 * @brief Compute weighted sum using fc_bigfloat accumulation
 *
 * Computes Σ(data[i] * weights[i]) with arbitrary-precision intermediate
 * products and sums. Set precision_bits to 0 to use the platform default precision.
 *
 * @param data Input array (must not be NULL, finite values only)
 * @param weights Weight array (must not be NULL, finite and >= 0)
 * @param n Number of elements (must be > 0)
 * @param sum Pre-created output bigfloat (must not be NULL)
 * @param precision_bits Precision in bits for intermediate values and output, or 0 for default
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 */
FC_API fc_status_t fc_stats_weighted_sum_bigfloat_f64(
    const double* data,
    const double* weights,
    size_t n,
    fc_bigfloat_t* sum,
    fc_uint64_t precision_bits
);

/**
 * @brief Compute weighted mean using fc_bigfloat accumulation
 *
 * Computes Σ(data[i] * weights[i]) / Σ(weights[i]) with arbitrary-precision
 * intermediate products and sums. Set precision_bits to 0 to use the platform
 * default precision.
 *
 * @param data Input array (must not be NULL, finite values only)
 * @param weights Weight array (must not be NULL, finite and >= 0)
 * @param n Number of elements (must be > 0)
 * @param mean Pre-created output bigfloat (must not be NULL)
 * @param precision_bits Precision in bits for intermediate values and output, or 0 for default
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 */
FC_API fc_status_t fc_stats_weighted_mean_bigfloat_f64(
    const double* data,
    const double* weights,
    size_t n,
    fc_bigfloat_t* mean,
    fc_uint64_t precision_bits
);

/**
 * @brief Compute weighted population variance using fc_bigfloat accumulation
 */
FC_API fc_status_t fc_stats_weighted_variance_bigfloat_f64(
    const double* data,
    const double* weights,
    size_t n,
    fc_bigfloat_t* variance,
    fc_uint64_t precision_bits
);

/**
 * @brief Compute weighted mean and population variance using fc_bigfloat accumulation
 */
FC_API fc_status_t fc_stats_weighted_mean_variance_bigfloat_f64(
    const double* data,
    const double* weights,
    size_t n,
    fc_bigfloat_t* mean,
    fc_bigfloat_t* variance,
    fc_uint64_t precision_bits
);

/**
 * @brief Compute weighted population standard deviation using fc_bigfloat accumulation
 */
FC_API fc_status_t fc_stats_weighted_stddev_bigfloat_f64(
    const double* data,
    const double* weights,
    size_t n,
    fc_bigfloat_t* stddev,
    fc_uint64_t precision_bits
);

/**
 * @brief Batch compute weighted sums for flat groups
 *
 * Processes flat data laid out as n_groups contiguous groups, each with
 * group_size elements. The weights array uses the same layout.
 *
 * Time complexity: O(n_groups * group_size)
 * Space complexity: O(1)
 *
 * @param sums Output array of weighted sums (must not be NULL, size >= n_groups)
 * @param data Flat input array (must not be NULL, size >= n_groups * group_size)
 * @param weights Flat weight array (must not be NULL, size >= n_groups * group_size)
 * @param n_groups Number of groups (must be > 0)
 * @param group_size Number of elements per group (must be > 0)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 */
FC_API fc_status_t fc_stats_weighted_sum_batch_f64(
    double* sums,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size
);

/**
 * @brief Batch compute weighted means for flat groups
 *
 * Processes flat data laid out as n_groups contiguous groups, each with
 * group_size elements. The weights array uses the same layout.
 *
 * Time complexity: O(n_groups * group_size)
 * Space complexity: O(1)
 *
 * @param means Output array of weighted means (must not be NULL, size >= n_groups)
 * @param data Flat input array (must not be NULL, size >= n_groups * group_size)
 * @param weights Flat weight array (must not be NULL, size >= n_groups * group_size)
 * @param n_groups Number of groups (must be > 0)
 * @param group_size Number of elements per group (must be > 0)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 */
FC_API fc_status_t fc_stats_weighted_mean_batch_f64(
    double* means,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size
);

/**
 * @brief Batch compute weighted population variances for flat groups
 *
 * Time complexity: O(n_groups * group_size)
 * Space complexity: O(1)
 *
 * @param variances Output array of weighted variances (must not be NULL, size >= n_groups)
 * @param data Flat input array (must not be NULL, size >= n_groups * group_size)
 * @param weights Flat weight array (must not be NULL, size >= n_groups * group_size)
 * @param n_groups Number of groups (must be > 0)
 * @param group_size Number of elements per group (must be > 0)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 */
FC_API fc_status_t fc_stats_weighted_variance_batch_f64(
    double* variances,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size
);

/**
 * @brief Batch compute weighted means and population variances for flat groups
 *
 * Time complexity: O(n_groups * group_size)
 * Space complexity: O(1)
 *
 * @param means Output array of weighted means (must not be NULL, size >= n_groups)
 * @param variances Output array of weighted variances (must not be NULL, size >= n_groups)
 * @param data Flat input array (must not be NULL, size >= n_groups * group_size)
 * @param weights Flat weight array (must not be NULL, size >= n_groups * group_size)
 * @param n_groups Number of groups (must be > 0)
 * @param group_size Number of elements per group (must be > 0)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 */
FC_API fc_status_t fc_stats_weighted_mean_variance_batch_f64(
    double* means,
    double* variances,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size
);

/**
 * @brief Batch compute weighted population standard deviations for flat groups
 *
 * Time complexity: O(n_groups * group_size)
 * Space complexity: O(1)
 *
 * @param stddevs Output array of weighted standard deviations (must not be NULL, size >= n_groups)
 * @param data Flat input array (must not be NULL, size >= n_groups * group_size)
 * @param weights Flat weight array (must not be NULL, size >= n_groups * group_size)
 * @param n_groups Number of groups (must be > 0)
 * @param group_size Number of elements per group (must be > 0)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 */
FC_API fc_status_t fc_stats_weighted_stddev_batch_f64(
    double* stddevs,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size
);

/**
 * @brief Batch compute weighted sums using fc_bigfloat accumulation
 *
 * Each output handle must be pre-created by the caller. Set precision_bits to 0
 * to use the platform default precision.
 */
FC_API fc_status_t fc_stats_weighted_sum_batch_bigfloat_f64(
    fc_bigfloat_t* const* sums,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size,
    fc_uint64_t precision_bits
);

/**
 * @brief Batch compute weighted means using fc_bigfloat accumulation
 *
 * Each output handle must be pre-created by the caller. Set precision_bits to 0
 * to use the platform default precision.
 */
FC_API fc_status_t fc_stats_weighted_mean_batch_bigfloat_f64(
    fc_bigfloat_t* const* means,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size,
    fc_uint64_t precision_bits
);

/**
 * @brief Batch compute weighted population variances using fc_bigfloat accumulation
 */
FC_API fc_status_t fc_stats_weighted_variance_batch_bigfloat_f64(
    fc_bigfloat_t* const* variances,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size,
    fc_uint64_t precision_bits
);

/**
 * @brief Batch compute weighted means and population variances using fc_bigfloat accumulation
 */
FC_API fc_status_t fc_stats_weighted_mean_variance_batch_bigfloat_f64(
    fc_bigfloat_t* const* means,
    fc_bigfloat_t* const* variances,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size,
    fc_uint64_t precision_bits
);

/**
 * @brief Batch compute weighted population standard deviations using fc_bigfloat accumulation
 */
FC_API fc_status_t fc_stats_weighted_stddev_batch_bigfloat_f64(
    fc_bigfloat_t* const* stddevs,
    const double* data,
    const double* weights,
    size_t n_groups,
    size_t group_size,
    fc_uint64_t precision_bits
);

/**
 * @brief Compute weighted median for a single dataset
 *
 * Weighted median is the value at which cumulative weight reaches 50% of total weight.
 * Unlike weighted mean, the median is robust against outliers and manipulation,
 * making it ideal for aggregating prices from multiple exchanges.
 *
 * Algorithm: Sort values by magnitude, compute cumulative weights, find the first
 * position where cumulative weight >= total_weight/2.
 *
 * Time complexity: O(n log n) due to sorting
 * Space complexity: O(n) for temporary index array
 *
 * @param data Input values (e.g., prices from multiple exchanges)
 * @param weights Corresponding weights (e.g., volume or credibility, must be >= 0)
 * @param n Number of elements (must be > 0)
 * @param result Output weighted median value (must not be NULL)
 *
 * @return FC_OK on success, error code on failure:
 *         FC_ERR_INVALID_ARG: NULL pointer or n == 0
 *         FC_ERR_NAN_INPUT: Data or weights contain NaN/Inf, or negative weights
 *         FC_ERR_INVALID_ARG: All weights are zero
 *         FC_ERR_OUT_OF_MEMORY: Memory allocation failed
 *
 * @note Thread-safe if different threads use different input arrays
 *
 * Example:
 *   Exchange prices: {50000, 50050, 49900, 50020, 51000}
 *   Volumes:         {100,   150,   120,   130,   10}
 *   Result: ~50020 (anti-manipulation aggregation)
 */
FC_API fc_status_t
fc_stats_weighted_median_f64(const double* data, const double* weights, size_t n, double* result);

/**
 * @brief Compute weighted median for multiple datasets in batch
 *
 * Batch processing for computing weighted medians across multiple independent datasets.
 * Each dataset can have different size. Commonly used for computing mark prices
 * across multiple trading contracts from multi-exchange data.
 *
 * Time complexity: O(sum of n_i log n_i) for all datasets
 * Space complexity: O(max n_i) for temporary arrays
 *
 * @param data Concatenated array of all value arrays
 * @param weights Concatenated array of all weight arrays
 * @param offsets Array of starting offsets for each dataset (length = batch_size)
 * @param sizes Array of sizes for each dataset (length = batch_size)
 * @param batch_size Number of datasets (must be > 0)
 * @param results Output array of weighted medians (must not be NULL, length = batch_size)
 *
 * @return FC_OK on success, error code on failure:
 *         FC_ERR_INVALID_ARG: NULL pointer, batch_size == 0, or any dataset size == 0
 *         FC_ERR_NAN_INPUT: Invalid data or weights in any dataset
 *         FC_ERR_INVALID_ARG: All weights zero in any dataset
 *         FC_ERR_OUT_OF_MEMORY: Memory allocation failed
 *
 * @note Each dataset is processed independently
 * @note Thread-safe if different threads use different input arrays
 *
 * Example:
 *   Dataset 0: data[0..2], weights[0..2], size=3
 *   Dataset 1: data[3..7], weights[3..7], size=5
 *   offsets = {0, 3}, sizes = {3, 5}, batch_size = 2
 */
FC_API fc_status_t fc_stats_weighted_median_batch_f64(
    const double* data,
    const double* weights,
    const size_t* offsets,
    const size_t* sizes,
    size_t batch_size,
    double* results
);

FC_END_DECLS

#endif /* FC_STATS_weighted_H */
