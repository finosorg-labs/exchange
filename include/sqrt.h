#ifndef FC_MATH_SQRT_H
#define FC_MATH_SQRT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute square root for float64 array
 *
 * Computes sqrt(x) for each element in the input array using vectorized
 * hardware square root instructions.
 *
 * @param input Input array
 * @param output Output array (square roots)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - sqrt(NaN) = NaN
 *       - sqrt(+Inf) = +Inf
 *       - sqrt(x < 0) = NaN (domain error)
 *       - sqrt(0) = 0
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_sqrt_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute square root for float32 array
 *
 * Computes sqrt(x) for each element in the input array using vectorized
 * hardware square root instructions.
 *
 * @param input Input array
 * @param output Output array (square roots)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - sqrt(NaN) = NaN
 *       - sqrt(+Inf) = +Inf
 *       - sqrt(x < 0) = NaN (domain error)
 *       - sqrt(0) = 0
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_sqrt_f32(const float* input, float* output, size_t n);

#ifdef __cplusplus
}
#endif

#endif
