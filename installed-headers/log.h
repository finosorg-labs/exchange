#ifndef FC_MATH_LOG_H
#define FC_MATH_LOG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute natural logarithm for float64 array
 *
 * Computes log(x) for each element in the input array using vectorized
 * Remez polynomial approximation. Achieves <1 ULP accuracy.
 *
 * @param input Input array
 * @param output Output array (logarithms)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - log(NaN) = NaN
 *       - log(+Inf) = +Inf
 *       - log(x < 0) = NaN
 *       - log(0) = -Inf
 *       - log(1) = 0
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_log_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute natural logarithm for float32 array
 *
 * Computes log(x) for each element in the input array using vectorized
 * Remez polynomial approximation. Achieves <1 ULP accuracy.
 *
 * @param input Input array
 * @param output Output array (logarithms)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - log(NaN) = NaN
 *       - log(+Inf) = +Inf
 *       - log(x < 0) = NaN
 *       - log(0) = -Inf
 *       - log(1) = 0
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_log_f32(const float* input, float* output, size_t n);

#ifdef __cplusplus
}
#endif

#endif
