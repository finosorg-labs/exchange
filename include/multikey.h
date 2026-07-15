/**
 * @file multikey.h
 * @brief Multi-key sorting for records with multiple fields
 *
 * Provides efficient sorting of structured records by multiple keys with
 * configurable priority and sort order. Optimized for financial data such as
 * order books (sort by price, then time, then order ID).
 *
 * Features:
 * - Support for up to 8 keys per sort operation
 * - Multiple data types: int32, int64, uint32, uint64, float, double
 * - Per-key sort order (ascending/descending)
 * - Stable sorting (preserves relative order of equal elements)
 * - Optimized comparison functions with SIMD where applicable
 *
 * Typical use cases:
 * - Order book sorting: (price DESC, time ASC, order_id ASC)
 * - Trade matching: (symbol ASC, price DESC, time ASC)
 * - Portfolio sorting: (sector ASC, market_cap DESC, ticker ASC)
 */

#ifndef FC_MULTIKEY_H
#define FC_MULTIKEY_H

#include <error.h>
#include <platform.h>
#include <stddef.h>
#include <stdint.h>

/* Import fc_sort_order_t from float_sort.h */
#include "float_sort.h"

FC_BEGIN_DECLS

/**
 * @brief Maximum number of keys supported in a single sort operation
 */
#define FC_SORT_MAX_KEYS 8

/**
 * @brief Supported data types for sort keys
 */
typedef enum {
    FC_SORT_KEY_INT32  = 0, /**< 32-bit signed integer */
    FC_SORT_KEY_INT64  = 1, /**< 64-bit signed integer */
    FC_SORT_KEY_UINT32 = 2, /**< 32-bit unsigned integer */
    FC_SORT_KEY_UINT64 = 3, /**< 64-bit unsigned integer */
    FC_SORT_KEY_FLOAT  = 4, /**< 32-bit floating-point */
    FC_SORT_KEY_DOUBLE = 5  /**< 64-bit floating-point */
} fc_sort_key_type_t;

/**
 * @brief Specification for a single sort key
 *
 * Describes one field in the record structure to use as a sort key.
 */
typedef struct {
    size_t offset;           /**< Byte offset of the field in the record structure */
    fc_sort_key_type_t type; /**< Data type of the field */
    fc_sort_order_t order; /**< Sort order for this key (FC_SORT_ASCENDING or FC_SORT_DESCENDING) */
} fc_sort_key_spec_t;

/**
 * @brief Sort an array of records by multiple keys (in-place)
 *
 * Performs stable multi-key sorting of structured records. Keys are evaluated
 * in order: if two records are equal on key[0], they are compared on key[1],
 * and so on. Records that are equal on all keys maintain their original
 * relative order (stable sort).
 *
 * Algorithm: Timsort (adaptive merge sort with insertion sort for small runs)
 * Time complexity: O(n log n) worst case, O(n) for nearly sorted data
 * Space complexity: O(n) temporary buffer for merging
 *
 * Example usage:
 * @code
 * typedef struct {
 *     double price;
 *     int64_t time;
 *     uint32_t order_id;
 * } Order;
 *
 * Order orders[1000];
 * fc_sort_key_spec_t keys[] = {
 *     {offsetof(Order, price), FC_SORT_KEY_DOUBLE, FC_SORT_DESCENDING},
 *     {offsetof(Order, time), FC_SORT_KEY_INT64, FC_SORT_ASCENDING},
 *     {offsetof(Order, order_id), FC_SORT_KEY_UINT32, FC_SORT_ASCENDING}
 * };
 * fc_sort_multikey(orders, 1000, sizeof(Order), keys, 3);
 * @endcode
 *
 * @param data Pointer to array of records (modified in-place)
 * @param count Number of records in the array
 * @param record_size Size of each record in bytes
 * @param keys Array of key specifications (evaluated in order)
 * @param num_keys Number of keys (must be 1-8)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_multikey(
    void* data,
    size_t count,
    size_t record_size,
    const fc_sort_key_spec_t* keys,
    size_t num_keys
);

/**
 * @brief Sort records by multiple keys and return sorted indices
 *
 * Similar to fc_sort_multikey but does not modify the original data.
 * Instead, returns an array of indices representing the sorted order.
 *
 * Time complexity: O(n log n)
 * Space complexity: O(n) for index array + O(n) for merge buffer
 *
 * Example usage:
 * @code
 * Order orders[1000];
 * size_t indices[1000];
 * fc_sort_multikey_indexed(orders, 1000, sizeof(Order), keys, 3, indices);
 * // Access sorted order: orders[indices[0]], orders[indices[1]], ...
 * @endcode
 *
 * @param data Pointer to array of records (not modified)
 * @param count Number of records
 * @param record_size Size of each record in bytes
 * @param keys Array of key specifications
 * @param num_keys Number of keys (must be 1-8)
 * @param indices Output array of sorted indices (must be pre-allocated)
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_sort_multikey_indexed(
    const void* data,
    size_t count,
    size_t record_size,
    const fc_sort_key_spec_t* keys,
    size_t num_keys,
    size_t* indices
);

/**
 * @brief Check if an array of records is sorted according to key specifications
 *
 * Verifies that the array is sorted in the order specified by the keys.
 * Useful for testing and validation.
 *
 * Time complexity: O(n * k) where k is the number of keys
 *
 * @param data Pointer to array of records
 * @param count Number of records
 * @param record_size Size of each record in bytes
 * @param keys Array of key specifications
 * @param num_keys Number of keys
 * @return 1 if sorted, 0 otherwise
 */
FC_API int fc_sort_is_sorted_multikey(
    const void* data,
    size_t count,
    size_t record_size,
    const fc_sort_key_spec_t* keys,
    size_t num_keys
);

FC_END_DECLS

#endif /* FC_MULTIKEY_H */
