#ifndef FC_MATH_CUMSUM_H
#define FC_MATH_CUMSUM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute cumulative sum of float64 array
 *
 * Computes the cumulative sum (prefix sum) of the input array.
 * Result[i] = sum(input[0:i+1])
 *
 * @param input Input array
 * @param output Output array (cumulative sums)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note NaN values propagate through the computation
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_cumsum_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute cumulative sum of float32 array
 *
 * Computes the cumulative sum (prefix sum) of the input array.
 * Result[i] = sum(input[0:i+1])
 *
 * @param input Input array
 * @param output Output array (cumulative sums)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note NaN values propagate through the computation
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_cumsum_f32(const float* input, float* output, size_t n);

/**
 * @brief Compute cumulative sum of float64 array using Kahan summation algorithm
 *
 * Computes the cumulative sum (prefix sum) using compensated summation
 * to reduce floating-point rounding errors. This provides higher numerical
 * accuracy at the cost of approximately 4x computation time.
 *
 * @param input Input array
 * @param output Output array (cumulative sums)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note NaN values propagate through the computation
 * @note Thread-safe (no shared state)
 * @note Recommended for financial calculations requiring high precision
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_cumsum_kahan_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute cumulative sum of float32 array using Kahan summation algorithm
 *
 * Computes the cumulative sum (prefix sum) using compensated summation
 * to reduce floating-point rounding errors. This provides higher numerical
 * accuracy at the cost of approximately 4x computation time.
 *
 * @param input Input array
 * @param output Output array (cumulative sums)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note NaN values propagate through the computation
 * @note Thread-safe (no shared state)
 * @note Recommended for financial calculations requiring high precision
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_cumsum_kahan_f32(const float* input, float* output, size_t n);

#ifdef __cplusplus
}
#endif

#endif
