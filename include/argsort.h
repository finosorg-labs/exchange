/**
 * @file argsort.h
 * @brief Index sorting (argsort) algorithms
 *
 * Provides functions to compute the indices that would sort an array,
 * without modifying the original array. This is useful for maintaining
 * correspondence between sorted data and original positions.
 *
 * Example:
 *   Input:  [3.0, 1.0, 2.0]
 *   Output: [1, 2, 0]  (indices that would sort the array)
 *
 * Features:
 * - Stable and unstable variants
 * - Ascending and descending order
 * - Proper NaN handling (IEEE 754 compliant)
 * - SIMD optimizations for comparison-heavy operations
 */

#ifndef FC_ARGSORT_H
#define FC_ARGSORT_H

#include <error.h>
#include <platform.h>
#include <stddef.h>
#include <stdint.h>

FC_BEGIN_DECLS

/**
 * @brief Sort order enumeration
 */
typedef enum { FC_ARGSORT_ASCENDING = 0, FC_ARGSORT_DESCENDING = 1 } fc_argsort_order_t;

/**
 * @brief Stability mode for sorting
 */
typedef enum {
    FC_ARGSORT_UNSTABLE = 0, /**< Faster, does not preserve relative order of equal elements */
    FC_ARGSORT_STABLE   = 1  /**< Preserves relative order of equal elements */
} fc_argsort_stability_t;

/**
 * @brief Compute indices that would sort a 64-bit float array
 *
 * Returns the permutation indices that would sort the input array.
 * The original array is not modified.
 *
 * NaN handling (IEEE 754 compliant):
 * - NaN values are placed at the end for ascending order
 * - NaN values are placed at the end for descending order
 * - All NaN values compare as equal
 *
 * Time complexity: O(n log n)
 * Space complexity: O(n) for temporary storage
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param indices Output array of indices (must be pre-allocated, size n)
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 *
 * @note This is an unstable sort (faster but doesn't preserve relative order)
 */
FC_API fc_status_t
fc_sort_argsort_f64(const double* data, size_t n, size_t* indices, fc_argsort_order_t order);

/**
 * @brief Compute indices that would sort a 64-bit float array (stable version)
 *
 * Same as fc_sort_argsort_f64 but preserves the relative order of equal elements.
 * Slightly slower than the unstable version.
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param indices Output array of indices (must be pre-allocated, size n)
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_sort_argsort_f64_stable(const double* data, size_t n, size_t* indices, fc_argsort_order_t order);

/**
 * @brief Compute indices that would sort a 32-bit float array
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param indices Output array of indices (must be pre-allocated, size n)
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_sort_argsort_f32(const float* data, size_t n, size_t* indices, fc_argsort_order_t order);

/**
 * @brief Compute indices that would sort a 32-bit float array (stable version)
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param indices Output array of indices (must be pre-allocated, size n)
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_sort_argsort_f32_stable(const float* data, size_t n, size_t* indices, fc_argsort_order_t order);

/**
 * @brief Compute indices that would sort a 64-bit integer array
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param indices Output array of indices (must be pre-allocated, size n)
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_sort_argsort_i64(const int64_t* data, size_t n, size_t* indices, fc_argsort_order_t order);

/**
 * @brief Compute indices that would sort a 64-bit integer array (stable version)
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param indices Output array of indices (must be pre-allocated, size n)
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_argsort_i64_stable(
    const int64_t* data,
    size_t n,
    size_t* indices,
    fc_argsort_order_t order
);

/**
 * @brief Compute indices that would sort a 32-bit integer array
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param indices Output array of indices (must be pre-allocated, size n)
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_sort_argsort_i32(const int32_t* data, size_t n, size_t* indices, fc_argsort_order_t order);

/**
 * @brief Compute indices that would sort a 32-bit integer array (stable version)
 *
 * @param data Input array (not modified)
 * @param n Number of elements
 * @param indices Output array of indices (must be pre-allocated, size n)
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_argsort_i32_stable(
    const int32_t* data,
    size_t n,
    size_t* indices,
    fc_argsort_order_t order
);

FC_END_DECLS

#endif /* FC_ARGSORT_H */
