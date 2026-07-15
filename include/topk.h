/**
 * @file topk.h
 * @brief High-performance Top-K selection algorithms
 *
 * Provides optimized selection of K largest/smallest elements from arrays:
 * - QuickSelect for average O(n) complexity
 * - Heap-based approach for small K (K < 100)
 * - Partial sorting for medium K (100 <= K < n/10)
 * - Full sort for large K (K >= n/10)
 *
 * Algorithm selection strategy:
 * - Small K (< 100): Min/Max heap
 * - Medium K (100 to n/10): QuickSelect with partial sort
 * - Large K (>= n/10): Full sort (more cache-friendly)
 */

#ifndef FC_TOPK_H
#define FC_TOPK_H

#include <error.h>
#include <platform.h>
#include <stddef.h>
#include <stdint.h>

FC_BEGIN_DECLS

/**
 * @brief Select K largest elements from float64 array
 *
 * Partially sorts the array so that the K largest elements are at the beginning
 * in descending order. The remaining elements are unordered.
 *
 * Uses adaptive algorithm selection:
 * - K < 100: Heap-based selection O(n log K)
 * - 100 <= K < n/10: QuickSelect O(n) average
 * - K >= n/10: Full sort O(n log n)
 *
 * Time complexity: O(n) average for small/medium K, O(n log n) for large K
 * Space complexity: O(1) in-place operation
 *
 * @param data Array to select from (modified in-place)
 * @param n Number of elements
 * @param k Number of largest elements to select (must be <= n)
 * @return FC_OK on success, FC_ERR_INVALID_ARG if k > n or data is NULL
 *
 * Example:
 *   double data[] = {3.0, 1.0, 4.0, 1.0, 5.0, 9.0, 2.0, 6.0};
 *   fc_topk_largest_f64(data, 8, 3);
 *   // First 3 elements are now: [9.0, 6.0, 5.0] (in descending order)
 */
FC_API fc_status_t fc_topk_largest_f64(double* data, size_t n, size_t k);

/**
 * @brief Select K smallest elements from float64 array
 *
 * Partially sorts the array so that the K smallest elements are at the beginning
 * in sorted order. The remaining elements are unordered.
 *
 * @param data Array to select from (modified in-place)
 * @param n Number of elements
 * @param k Number of smallest elements to select (must be <= n)
 * @return FC_OK on success, FC_ERR_INVALID_ARG if k > n or data is NULL
 *
 * Example:
 *   double data[] = {3.0, 1.0, 4.0, 1.0, 5.0, 9.0, 2.0, 6.0};
 *   fc_topk_smallest_f64(data, 8, 3);
 *   // First 3 elements are now: [1.0, 1.0, 2.0] (in sorted order)
 */
FC_API fc_status_t fc_topk_smallest_f64(double* data, size_t n, size_t k);

/**
 * @brief Select K largest elements from float32 array
 *
 * Same algorithm selection as fc_topk_largest_f64 but optimized for 32-bit floats.
 *
 * @param data Array to select from (modified in-place)
 * @param n Number of elements
 * @param k Number of largest elements to select (must be <= n)
 * @return FC_OK on success, FC_ERR_INVALID_ARG if k > n or data is NULL
 */
FC_API fc_status_t fc_topk_largest_f32(float* data, size_t n, size_t k);

/**
 * @brief Select K smallest elements from float32 array
 *
 * @param data Array to select from (modified in-place)
 * @param n Number of elements
 * @param k Number of smallest elements to select (must be <= n)
 * @return FC_OK on success, FC_ERR_INVALID_ARG if k > n or data is NULL
 */
FC_API fc_status_t fc_topk_smallest_f32(float* data, size_t n, size_t k);

/**
 * @brief Select K largest elements with indices from float64 array
 *
 * Returns both the K largest values and their original indices.
 * The output arrays must be pre-allocated by the caller.
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param k Number of largest elements to select (must be <= n)
 * @param out_values Output array for K largest values (must have size >= k)
 * @param out_indices Output array for original indices (must have size >= k)
 * @return FC_OK on success, FC_ERR_INVALID_ARG if parameters are invalid
 *
 * Example:
 *   double data[] = {3.0, 1.0, 4.0, 1.0, 5.0};
 *   double values[2];
 *   size_t indices[2];
 *   fc_topk_largest_f64_indexed(data, 5, 2, values, indices);
 *   // values: [5.0, 4.0], indices: [4, 2]
 */
FC_API fc_status_t fc_topk_largest_f64_indexed(
    const double* data,
    size_t n,
    size_t k,
    double* out_values,
    size_t* out_indices
);

/**
 * @brief Select K smallest elements with indices from float64 array
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param k Number of smallest elements to select (must be <= n)
 * @param out_values Output array for K smallest values (must have size >= k)
 * @param out_indices Output array for original indices (must have size >= k)
 * @return FC_OK on success, FC_ERR_INVALID_ARG if parameters are invalid
 */
FC_API fc_status_t fc_topk_smallest_f64_indexed(
    const double* data,
    size_t n,
    size_t k,
    double* out_values,
    size_t* out_indices
);

/**
 * @brief Select K largest elements with indices from float32 array
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param k Number of largest elements to select (must be <= n)
 * @param out_values Output array for K largest values (must have size >= k)
 * @param out_indices Output array for original indices (must have size >= k)
 * @return FC_OK on success, FC_ERR_INVALID_ARG if parameters are invalid
 */
FC_API fc_status_t fc_topk_largest_f32_indexed(
    const float* data,
    size_t n,
    size_t k,
    float* out_values,
    size_t* out_indices
);

/**
 * @brief Select K smallest elements with indices from float32 array
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param k Number of smallest elements to select (must be <= n)
 * @param out_values Output array for K smallest values (must have size >= k)
 * @param out_indices Output array for original indices (must have size >= k)
 * @return FC_OK on success, FC_ERR_INVALID_ARG if parameters are invalid
 */
FC_API fc_status_t fc_topk_smallest_f32_indexed(
    const float* data,
    size_t n,
    size_t k,
    float* out_values,
    size_t* out_indices
);

FC_END_DECLS

#endif /* FC_TOPK_H */
