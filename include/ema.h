/**
 * @file ema.h
 * @brief Exponential Moving Average (EMA) computation
 *
 * Provides high-performance EMA calculation with SIMD optimization.
 * Uses runtime dispatch to select optimal implementation based on CPU features.
 */

#ifndef FC_STATS_EMA_H
#define FC_STATS_EMA_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute exponential moving average (EMA) of a time series
 *
 * Computes EMA using the formula:
 *   EMA[0] = data[0]
 *   EMA[i] = alpha * data[i] + (1 - alpha) * EMA[i-1]
 *
 * where alpha is the smoothing factor (0 < alpha <= 1).
 * Common choice: alpha = 2 / (period + 1)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary
 *
 * @param[in] data Input time series data (must not be NULL)
 * @param[in] n Number of elements (must be > 0)
 * @param[in] alpha Smoothing factor (must be in (0, 1])
 * @param[out] ema Output EMA values (must not be NULL, length n)
 * @return FC_OK on success, error code otherwise
 *
 * @note Output array can be the same as input array (in-place operation)
 * @note NaN values in input will propagate to output
 * @note Thread-safe (no shared state)
 */
fc_status_t fc_stats_ema_f64(const double* data, size_t n, double alpha, double* ema);

/**
 * @brief Compute EMA with custom initial value
 *
 * Same as fc_stats_ema_f64 but allows specifying the initial EMA value
 * instead of using data[0].
 *
 * @param[in] data Input time series data (must not be NULL)
 * @param[in] n Number of elements (must be > 0)
 * @param[in] alpha Smoothing factor (must be in (0, 1])
 * @param[in] init_value Initial EMA value
 * @param[out] ema Output EMA values (must not be NULL, length n)
 * @return FC_OK on success, error code otherwise
 */
fc_status_t fc_stats_ema_f64_init(
    const double* data,
    size_t n,
    double alpha,
    double init_value,
    double* ema
);

/**
 * @brief Compute batch EMA for multiple time series
 *
 * Computes EMA for multiple independent time series in parallel.
 * Each series has the same length and uses the same alpha.
 *
 * Time complexity: O(num_series * n)
 *
 * @param[in] data Input data (num_series x n, row-major)
 * @param[in] num_series Number of time series
 * @param[in] n Length of each series (must be > 0)
 * @param[in] alpha Smoothing factor (must be in (0, 1])
 * @param[out] ema Output EMA values (num_series x n, row-major)
 * @return FC_OK on success, error code otherwise
 *
 * @note data and ema can point to the same memory (in-place)
 */
fc_status_t fc_stats_ema_batch_f64(
    const double* data,
    size_t num_series,
    size_t n,
    double alpha,
    double* ema
);

FC_END_DECLS

#endif /* FC_STATS_EMA_H */
