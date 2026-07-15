#ifndef FC_MATH_EXP_H
#define FC_MATH_EXP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute exponential function for float64 array
 *
 * Computes exp(x) for each element in the input array using vectorized
 * Remez polynomial approximation. Achieves <1 ULP accuracy.
 *
 * @param input Input array
 * @param output Output array (exponentials)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - exp(NaN) = NaN
 *       - exp(+Inf) = +Inf
 *       - exp(-Inf) = 0
 *       - exp(x > 709.78) = +Inf (overflow)
 *       - exp(x < -745.13) = 0 (underflow)
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_exp_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute exponential function for float32 array
 *
 * Computes exp(x) for each element in the input array using vectorized
 * Remez polynomial approximation. Achieves <1 ULP accuracy.
 *
 * @param input Input array
 * @param output Output array (exponentials)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note Special value handling:
 *       - exp(NaN) = NaN
 *       - exp(+Inf) = +Inf
 *       - exp(-Inf) = 0
 *       - exp(x > 88.72) = +Inf (overflow)
 *       - exp(x < -103.97) = 0 (underflow)
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_exp_f32(const float* input, float* output, size_t n);

#ifdef __cplusplus
}
#endif

#endif
