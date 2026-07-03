#ifndef FC_MATH_CUMMAX_H
#define FC_MATH_CUMMAX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute cumulative maximum of float64 array
 *
 * Computes the cumulative maximum (running maximum) of the input array.
 * Result[i] = max(input[0:i+1])
 *
 * @param input Input array
 * @param output Output array (cumulative maximums)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note NaN values propagate: if any input is NaN, all subsequent outputs are NaN
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_cummax_f64(const double* input, double* output, size_t n);

/**
 * @brief Compute cumulative maximum of float32 array
 *
 * Computes the cumulative maximum (running maximum) of the input array.
 * Result[i] = max(input[0:i+1])
 *
 * @param input Input array
 * @param output Output array (cumulative maximums)
 * @param n Number of elements
 * @return 0 on success, -1 on error
 *
 * @note Output array must be pre-allocated with size n
 * @note NaN values propagate: if any input is NaN, all subsequent outputs are NaN
 * @note Thread-safe (no shared state)
 *
 * Time complexity: O(n)
 * Space complexity: O(1) auxiliary space
 */
int fc_math_cummax_f32(const float* input, float* output, size_t n);

#ifdef __cplusplus
}
#endif

#endif
