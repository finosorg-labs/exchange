/**
 * @file moments.h
 * @brief Higher-order statistical moments (skewness and kurtosis)
 *
 * Provides online algorithms for computing skewness and kurtosis using
 * extended Welford's method, as well as batch computation functions.
 * All batch functions automatically use SIMD optimization when available.
 */

#ifndef FC_STATS_MOMENTS_H
#define FC_STATS_MOMENTS_H

#include "error.h"
#include "platform.h"
#include <math.h>

FC_BEGIN_DECLS

/**
 * @brief State for online higher-order moments computation
 *
 * Maintains running statistics for mean, variance, skewness, and kurtosis
 * using Welford's extended algorithm.
 */
typedef struct {
    size_t n;    /**< Number of samples */
    double mean; /**< Running mean */
    double m2;   /**< Second central moment (variance numerator) */
    double m3;   /**< Third central moment (skewness numerator) */
    double m4;   /**< Fourth central moment (kurtosis numerator) */
} fc_moments_state_t;

/**
 * @brief Initialize moments state
 *
 * @param[out] state Moments state to initialize
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe (caller must synchronize)
 */
static inline void fc_stats_moments_init(fc_moments_state_t* state) {
    state->n    = 0;
    state->mean = 0.0;
    state->m2   = 0.0;
    state->m3   = 0.0;
    state->m4   = 0.0;
}

/**
 * @brief Update moments with a single value
 *
 * @param[in,out] state Moments state to update
 * @param[in] value New value to incorporate
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe (caller must synchronize)
 */
void fc_stats_moments_update(fc_moments_state_t* state, double value);

/**
 * @brief Update moments with a batch of values
 *
 * Automatically dispatches to the best SIMD implementation based on runtime detection.
 * Falls back to scalar implementation if SIMD is not available.
 *
 * @param[in,out] state Moments state to update
 * @param[in] values Array of values (length n)
 * @param[in] n Number of values
 *
 * Time complexity: O(n)
 * Thread safety: Not thread-safe (caller must synchronize)
 */
void fc_stats_moments_update_batch(fc_moments_state_t* state, const double* values, size_t n);

/**
 * @brief Compute sample skewness from moments state
 *
 * Skewness = sqrt(n) * m3 / (m2^1.5)
 *
 * @param[in] state Moments state
 * @return Sample skewness, or NaN if n < 3
 *
 * Time complexity: O(1)
 * Thread safety: Thread-safe (read-only)
 */
static inline double fc_stats_moments_skewness(const fc_moments_state_t* state) {
    if (state->n < 3) {
        return NAN;
    }

    if (state->m2 == 0.0) {
        return NAN;
    }

    double sqrt_m2 = sqrt(state->m2);
    return sqrt((double) state->n) * state->m3 / (state->m2 * sqrt_m2);
}

/**
 * @brief Compute sample kurtosis from moments state
 *
 * Kurtosis = n * m4 / (m2^2)
 *
 * @param[in] state Moments state
 * @return Sample kurtosis, or NaN if n < 4
 *
 * Time complexity: O(1)
 * Thread safety: Thread-safe (read-only)
 */
static inline double fc_stats_moments_kurtosis(const fc_moments_state_t* state) {
    if (state->n < 4) {
        return NAN;
    }

    if (state->m2 == 0.0) {
        return NAN;
    }

    return (double) state->n * state->m4 / (state->m2 * state->m2);
}

/**
 * @brief Compute sample excess kurtosis from moments state
 *
 * Excess kurtosis = kurtosis - 3
 * (Normal distribution has excess kurtosis of 0)
 *
 * @param[in] state Moments state
 * @return Sample excess kurtosis, or NaN if n < 4
 *
 * Time complexity: O(1)
 * Thread safety: Thread-safe (read-only)
 */
static inline double fc_stats_moments_excess_kurtosis(const fc_moments_state_t* state) {
    double kurt = fc_stats_moments_kurtosis(state);
    if (isnan(kurt)) {
        return NAN;
    }
    return kurt - 3.0;
}

/**
 * @brief Compute skewness for multiple groups in batch
 *
 * Automatically uses SIMD optimization when available for best performance.
 *
 * @param[out] out Output array for skewness values (length n_groups)
 * @param[in] data Input data organized as n_groups × group_size
 * @param[in] n_groups Number of groups
 * @param[in] group_size Number of elements per group
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n_groups × group_size)
 * Thread safety: Thread-safe if output arrays don't overlap
 */
fc_status_t fc_stats_skewness_f64(
    double* out,
    const double* data,
    size_t n_groups,
    size_t group_size
);

/**
 * @brief Compute kurtosis for multiple groups in batch
 *
 * Automatically uses SIMD optimization when available for best performance.
 *
 * @param[out] out Output array for kurtosis values (length n_groups)
 * @param[in] data Input data organized as n_groups × group_size
 * @param[in] n_groups Number of groups
 * @param[in] group_size Number of elements per group
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n_groups × group_size)
 * Thread safety: Thread-safe if output arrays don't overlap
 */
fc_status_t fc_stats_kurtosis_f64(
    double* out,
    const double* data,
    size_t n_groups,
    size_t group_size
);

/**
 * @brief Compute excess kurtosis for multiple groups in batch
 *
 * Automatically uses SIMD optimization when available for best performance.
 *
 * @param[out] out Output array for excess kurtosis values (length n_groups)
 * @param[in] data Input data organized as n_groups × group_size
 * @param[in] n_groups Number of groups
 * @param[in] group_size Number of elements per group
 * @return FC_OK on success, error code otherwise
 *
 * Time complexity: O(n_groups × group_size)
 * Thread safety: Thread-safe if output arrays don't overlap
 */
fc_status_t fc_stats_excess_kurtosis_f64(
    double* out,
    const double* data,
    size_t n_groups,
    size_t group_size
);

FC_END_DECLS

#endif
