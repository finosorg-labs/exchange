/**
 * @file float_sort.h
 * @brief High-performance floating-point sorting algorithms
 *
 * Provides optimized sorting for float32 and float64 arrays with:
 * - Multi-level algorithm selection (insertion sort, introsort, radix sort)
 * - Proper NaN handling (IEEE 754 compliant)
 * - Batch operations for high throughput
 *
 * Algorithm selection strategy:
 * - Small arrays (< 32): Insertion sort
 * - Medium arrays (32-10000): Introsort (quicksort + heapsort)
 * - Large arrays (> 10000): Radix sort
 */

#ifndef FC_FLOAT_SORT_H
#define FC_FLOAT_SORT_H

#include <error.h>
#include <platform.h>
#include <stddef.h>
#include <stdint.h>

FC_BEGIN_DECLS

/**
 * @brief Sort order enumeration
 */
typedef enum { FC_SORT_ASCENDING = 0, FC_SORT_DESCENDING = 1 } fc_sort_order_t;

/**
 * @brief NaN handling strategy
 */
typedef enum {
    FC_SORT_NAN_FIRST  = 0, /**< Place NaN values at the beginning */
    FC_SORT_NAN_LAST   = 1, /**< Place NaN values at the end */
    FC_SORT_NAN_REJECT = 2  /**< Return error if NaN is encountered */
} fc_sort_nan_policy_t;

/**
 * @brief Sort 64-bit floating-point array in-place
 *
 * Automatically selects optimal algorithm based on array size:
 * - Small (< 32): Insertion sort
 * - Medium (32-10000): Introsort
 * - Large (> 10000): Radix sort with SIMD optimization
 *
 * NaN values are handled according to IEEE 754:
 * - By default, NaN values are placed at the end
 * - All NaN values compare as equal
 * - Negative zero equals positive zero
 *
 * Time complexity: O(n log n) average, O(n) for nearly sorted data
 * Space complexity: O(log n) stack space for introsort
 *
 * @param data Array to sort (modified in-place)
 * @param n Number of elements
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_float64(double* data, size_t n, fc_sort_order_t order);

/**
 * @brief Sort 64-bit floating-point array with custom NaN handling
 *
 * @param data Array to sort (modified in-place)
 * @param n Number of elements
 * @param order Sort order (ascending or descending)
 * @param nan_policy How to handle NaN values
 * @return FC_OK on success, FC_ERR_INVALID_ARG if NaN found with REJECT policy
 */
FC_API fc_status_t
fc_sort_float64_ex(double* data, size_t n, fc_sort_order_t order, fc_sort_nan_policy_t nan_policy);

/**
 * @brief Sort 32-bit floating-point array in-place
 *
 * Same algorithm selection as fc_sort_float64 but optimized for 32-bit floats.
 *
 * @param data Array to sort (modified in-place)
 * @param n Number of elements
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_float32(float* data, size_t n, fc_sort_order_t order);

/**
 * @brief Sort 32-bit floating-point array with custom NaN handling
 *
 * @param data Array to sort (modified in-place)
 * @param n Number of elements
 * @param order Sort order (ascending or descending)
 * @param nan_policy How to handle NaN values
 * @return FC_OK on success, FC_ERR_INVALID_ARG if NaN found with REJECT policy
 */
FC_API fc_status_t
fc_sort_float32_ex(float* data, size_t n, fc_sort_order_t order, fc_sort_nan_policy_t nan_policy);

/**
 * @brief Check if array is sorted
 *
 * @param data Array to check
 * @param n Number of elements
 * @param order Expected sort order
 * @return 1 if sorted, 0 otherwise
 */
FC_API int fc_sort_is_sorted_float64(const double* data, size_t n, fc_sort_order_t order);

/**
 * @brief Check if float32 array is sorted
 *
 * @param data Array to check
 * @param n Number of elements
 * @param order Expected sort order
 * @return 1 if sorted, 0 otherwise
 */
FC_API int fc_sort_is_sorted_float32(const float* data, size_t n, fc_sort_order_t order);

FC_END_DECLS

#endif /* FC_FLOAT_SORT_H */
