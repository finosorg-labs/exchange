/**
 * @file welford.h
 * @brief Welford's online algorithm for streaming statistics
 *
 * Provides numerically stable online computation of mean, variance, and
 * standard deviation for streaming data. Supports incremental updates
 * without storing the entire dataset.
 *
 * Key features:
 * - Single-pass online algorithm
 * - Numerically stable (Welford's method)
 * - Constant memory usage O(1)
 * - Support for both sample and population variance
 * - Thread-safe per-instance (no shared state)
 *
 * Use cases:
 * - Real-time statistics for streaming data
 * - Memory-constrained environments
 * - Incremental data processing
 *
 * Time complexity: O(1) per update
 * Space complexity: O(1)
 */

#ifndef FC_STATS_WELFORD_H
#define FC_STATS_WELFORD_H

#include "error.h"
#include "platform.h"
#include <math.h>

FC_BEGIN_DECLS

/**
 * @brief Welford online statistics state
 *
 * Maintains running statistics using Welford's algorithm.
 * All fields are internal and should not be accessed directly.
 */
typedef struct {
    size_t count; /**< Number of samples processed */
    double mean;  /**< Running mean */
    double m2;    /**< Sum of squared differences from mean */
} fc_welford_state_t;

/**
 * @brief Initialize Welford state
 *
 * Resets the state to initial values (count=0, mean=0, m2=0).
 * Must be called before first use.
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 *
 * @param state State structure to initialize (must not be NULL)
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if state is NULL
 *
 * @note Thread-safe per instance
 */
static inline fc_status_t fc_welford_init(fc_welford_state_t* state) {
    if (state == NULL) {
        return FC_ERR_INVALID_ARG;
    }

    state->count = 0;
    state->mean  = 0.0;
    state->m2    = 0.0;

    return FC_OK;
}

/**
 * @brief Update statistics with a single value
 *
 * Incrementally updates mean and variance with a new data point.
 * Uses Welford's numerically stable algorithm:
 *   count = count + 1
 *   delta = x - mean
 *   mean = mean + delta / count
 *   delta2 = x - mean
 *   m2 = m2 + delta * delta2
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 *
 * @param state State structure to update (must not be NULL)
 * @param value New data point to incorporate
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe per instance (different threads can use different states)
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if value is NaN
 */
FC_API fc_status_t fc_welford_update(fc_welford_state_t* state, double value);

/**
 * @brief Update statistics with multiple values
 *
 * Batch version of fc_welford_update for processing multiple values.
 * More efficient than calling fc_welford_update in a loop.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param state State structure to update (must not be NULL)
 * @param values Array of values to incorporate (must not be NULL)
 * @param n Number of values (must be > 0)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe per instance
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any value is NaN
 */
FC_API fc_status_t
fc_welford_update_batch(fc_welford_state_t* state, const double* values, size_t n);

/**
 * @brief Get current mean
 *
 * Returns the current mean of all values processed so far.
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 *
 * @param state State structure (must not be NULL)
 * @param mean Output mean value (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe (read-only operation)
 * @note Returns 0.0 if no values have been processed
 */
static inline fc_status_t fc_welford_mean(const fc_welford_state_t* state, double* mean) {
    if (state == NULL || mean == NULL) {
        return FC_ERR_INVALID_ARG;
    }

    if (state->count == 0) {
        *mean = 0.0;
        return FC_OK;
    }

    *mean = state->mean;
    return FC_OK;
}

/**
 * @brief Get current variance
 *
 * Returns the current variance of all values processed so far.
 * Supports both sample variance (n-1) and population variance (n).
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 *
 * @param state State structure (must not be NULL)
 * @param variance Output variance value (must not be NULL)
 * @param sample If 1, compute sample variance (n-1); if 0, population variance (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe (read-only operation)
 * @note Returns 0.0 if insufficient data (count < 2 for sample variance)
 */
static inline fc_status_t fc_welford_variance(
    const fc_welford_state_t* state,
    double* variance,
    int sample
) {
    if (state == NULL || variance == NULL) {
        return FC_ERR_INVALID_ARG;
    }

    if (state->count == 0) {
        *variance = 0.0;
        return FC_OK;
    }

    if (sample && state->count < 2) {
        *variance = 0.0;
        return FC_OK;
    }

    if (sample) {
        *variance = state->m2 / (state->count - 1);
    } else {
        *variance = state->m2 / state->count;
    }

    return FC_OK;
}

/**
 * @brief Get current standard deviation
 *
 * Returns the current standard deviation (sqrt of variance).
 * Supports both sample and population standard deviation.
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 *
 * @param state State structure (must not be NULL)
 * @param stddev Output standard deviation value (must not be NULL)
 * @param sample If 1, compute sample stddev (n-1); if 0, population stddev (n)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe (read-only operation)
 * @note Returns 0.0 if insufficient data (count < 2 for sample stddev)
 */
static inline fc_status_t fc_welford_stddev(
    const fc_welford_state_t* state,
    double* stddev,
    int sample
) {
    if (state == NULL || stddev == NULL) {
        return FC_ERR_INVALID_ARG;
    }

    double var         = 0.0;
    fc_status_t status = fc_welford_variance(state, &var, sample);
    if (status != FC_OK) {
        *stddev = 0.0;
        return status;
    }

    *stddev = sqrt(var);
    return FC_OK;
}

/**
 * @brief Get current count
 *
 * Returns the number of values processed so far.
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 *
 * @param state State structure (must not be NULL)
 * @param count Output count value (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe (read-only operation)
 */
static inline fc_status_t fc_welford_count(const fc_welford_state_t* state, size_t* count) {
    if (state == NULL || count == NULL) {
        return FC_ERR_INVALID_ARG;
    }

    *count = state->count;
    return FC_OK;
}

/**
 * @brief Merge two Welford states
 *
 * Combines statistics from two independent streams using Chan's parallel
 * algorithm. Useful for parallel processing or combining partial results.
 *
 * Algorithm:
 *   count_combined = count_a + count_b
 *   delta = mean_b - mean_a
 *   mean_combined = mean_a + delta * count_b / count_combined
 *   m2_combined = m2_a + m2_b + delta^2 * count_a * count_b / count_combined
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 *
 * @param state_a First state (will be modified to contain merged result)
 * @param state_b Second state (remains unchanged)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe if state_a and state_b are different instances
 * @note After merge, state_a contains the combined statistics
 */
FC_API fc_status_t fc_welford_merge(fc_welford_state_t* state_a, const fc_welford_state_t* state_b);

/**
 * @brief Reset Welford state to initial values
 *
 * Equivalent to calling fc_welford_init. Resets count, mean, and m2 to zero.
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 *
 * @param state State structure to reset (must not be NULL)
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if state is NULL
 *
 * @note Thread-safe per instance
 */
static inline fc_status_t fc_welford_reset(fc_welford_state_t* state) {
    return fc_welford_init(state);
}

/**
 * @brief Update multiple independent data streams in parallel (SIMD-optimized)
 *
 * Updates N independent Welford states simultaneously with one new value per stream.
 * This is the primary use case for SIMD optimization: each stream's update is
 * independent, allowing true vectorization.
 *
 * SIMD acceleration:
 * - SSE4.2: processes 2 streams in parallel
 * - AVX2: processes 4 streams in parallel
 * - AVX-512: processes 8 streams in parallel
 *
 * Use case: Real-time statistics for multiple assets (e.g., 5000 stocks)
 *
 * Time complexity: O(n_streams) with SIMD speedup
 * Space complexity: O(1)
 *
 * @param states Array of state structures to update (must not be NULL)
 * @param values Array of new values, one per stream (must not be NULL)
 * @param n_streams Number of streams (must be > 0)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe if different threads use different state arrays
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any value is NaN
 * @note For best performance, ensure n_streams is a multiple of SIMD width
 * @note Automatically selects optimal SIMD implementation based on CPU features
 */
FC_API fc_status_t
fc_welford_update_streams(fc_welford_state_t* states, const double* values, size_t n_streams);

FC_END_DECLS

#endif /* FC_STATS_WELFORD_H */
