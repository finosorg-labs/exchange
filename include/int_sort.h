/**
 * @file int_sort.h
 * @brief High-performance integer sorting algorithms
 *
 * Provides optimized radix sort for integer arrays with:
 * - SIMD-accelerated counting and histogram computation
 * - Optimized for signed and unsigned integers (32-bit and 64-bit)
 * - Batch operations for high throughput
 * - Proper handling of negative numbers in signed integer sorts
 *
 * Algorithm: LSD (Least Significant Digit) Radix Sort
 * - Processes integers byte-by-byte (8-bit radix)
 * - Uses counting sort for each digit pass
 * - Signed integers: XOR with sign bit to handle negatives
 * - Time complexity: O(n * k) where k = sizeof(type)
 * - Space complexity: O(n) for temporary buffer
 */

#ifndef FC_INT_SORT_H
#define FC_INT_SORT_H

#include <error.h>
#include <float_sort.h>
#include <platform.h>
#include <stddef.h>
#include <stdint.h>

FC_BEGIN_DECLS

/**
 * @brief Sort 64-bit signed integer array in-place using radix sort
 *
 * Uses LSD radix sort with 8-bit radix (256 buckets per pass).
 * Handles negative numbers by XORing with sign bit before sorting.
 * SIMD optimizations applied for histogram computation.
 *
 * Time complexity: O(8n) for 64-bit integers (8 passes)
 * Space complexity: O(n) for temporary buffer
 *
 * @param data Array to sort (modified in-place)
 * @param n Number of elements
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_int64(int64_t* data, size_t n, fc_sort_order_t order);

/**
 * @brief Sort 64-bit unsigned integer array in-place using radix sort
 *
 * Uses LSD radix sort with 8-bit radix (256 buckets per pass).
 * SIMD optimizations applied for histogram computation.
 *
 * Time complexity: O(8n) for 64-bit integers (8 passes)
 * Space complexity: O(n) for temporary buffer
 *
 * @param data Array to sort (modified in-place)
 * @param n Number of elements
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_uint64(uint64_t* data, size_t n, fc_sort_order_t order);

/**
 * @brief Sort 32-bit signed integer array in-place using radix sort
 *
 * Uses LSD radix sort with 8-bit radix (256 buckets per pass).
 * Handles negative numbers by XORing with sign bit before sorting.
 * SIMD optimizations applied for histogram computation.
 *
 * Time complexity: O(4n) for 32-bit integers (4 passes)
 * Space complexity: O(n) for temporary buffer
 *
 * @param data Array to sort (modified in-place)
 * @param n Number of elements
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_int32(int32_t* data, size_t n, fc_sort_order_t order);

/**
 * @brief Sort 32-bit unsigned integer array in-place using radix sort
 *
 * Uses LSD radix sort with 8-bit radix (256 buckets per pass).
 * SIMD optimizations applied for histogram computation.
 *
 * Time complexity: O(4n) for 32-bit integers (4 passes)
 * Space complexity: O(n) for temporary buffer
 *
 * @param data Array to sort (modified in-place)
 * @param n Number of elements
 * @param order Sort order (ascending or descending)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_uint32(uint32_t* data, size_t n, fc_sort_order_t order);

/**
 * @brief Check if int64 array is sorted
 *
 * @param data Array to check
 * @param n Number of elements
 * @param order Expected sort order
 * @return 1 if sorted, 0 otherwise
 */
FC_API int fc_sort_is_sorted_int64(const int64_t* data, size_t n, fc_sort_order_t order);

/**
 * @brief Check if uint64 array is sorted
 *
 * @param data Array to check
 * @param n Number of elements
 * @param order Expected sort order
 * @return 1 if sorted, 0 otherwise
 */
FC_API int fc_sort_is_sorted_uint64(const uint64_t* data, size_t n, fc_sort_order_t order);

/**
 * @brief Check if int32 array is sorted
 *
 * @param data Array to check
 * @param n Number of elements
 * @param order Expected sort order
 * @return 1 if sorted, 0 otherwise
 */
FC_API int fc_sort_is_sorted_int32(const int32_t* data, size_t n, fc_sort_order_t order);

/**
 * @brief Check if uint32 array is sorted
 *
 * @param data Array to check
 * @param n Number of elements
 * @param order Expected sort order
 * @return 1 if sorted, 0 otherwise
 */
FC_API int fc_sort_is_sorted_uint32(const uint32_t* data, size_t n, fc_sort_order_t order);

FC_END_DECLS

#endif /* FC_INT_SORT_H */
