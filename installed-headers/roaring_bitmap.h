/**
 * @file roaring_bitmap.h
 * @brief Roaring Bitmap - Compressed bitmap for efficient set operations
 *
 * A Roaring Bitmap is a compressed bitmap data structure that provides fast set
 * operations (union, intersection, difference) while using significantly less memory
 * than traditional bitmaps. It divides the 32-bit integer space into 2^16 containers,
 * each storing 16-bit values using one of three container types:
 * - Array container: for sparse data (< 4096 elements)
 * - Bitmap container: for dense data (>= 4096 elements)
 * - Run container: for consecutive sequences
 *
 * Features:
 * - Memory-efficient compression
 * - Fast set operations with SIMD acceleration
 * - Automatic container type selection
 * - Batch operations for high performance
 * - Cardinality tracking
 *
 * Typical use cases:
 * - Index compression in databases
 * - Set operations on large integer sets
 * - Bitmap indexes for analytics
 * - Efficient storage of sparse bit arrays
 */

#ifndef FC_ROARING_BITMAP_H
#define FC_ROARING_BITMAP_H

#include <error.h>
#include <platform.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque Roaring Bitmap handle
 */
typedef struct fc_roaring_bitmap fc_roaring_bitmap_t;

/**
 * @brief Roaring Bitmap statistics
 */
typedef struct {
    uint64_t cardinality;         /**< Number of elements in the bitmap */
    size_t num_containers;        /**< Number of containers */
    size_t num_array_containers;  /**< Number of array containers */
    size_t num_bitmap_containers; /**< Number of bitmap containers */
    size_t num_run_containers;    /**< Number of run containers */
    size_t memory_bytes;          /**< Total memory usage in bytes */
} fc_roaring_stats_t;

/**
 * @brief Create a new empty Roaring Bitmap
 *
 * @return Roaring Bitmap handle, or NULL on error
 *
 * @note Caller must call fc_roaring_destroy() to free resources
 * @note Thread-safe: No
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 */
fc_roaring_bitmap_t* fc_roaring_create(void);

/**
 * @brief Create a Roaring Bitmap from an array of values
 *
 * @param values Array of 32-bit unsigned integers
 * @param count Number of values
 * @return Roaring Bitmap handle, or NULL on error
 *
 * @note Caller must call fc_roaring_destroy() to free resources
 * @note Thread-safe: No
 *
 * Time complexity: O(n log n) where n = count
 * Space complexity: O(n)
 */
fc_roaring_bitmap_t* fc_roaring_create_from_array(const uint32_t* values, size_t count);

/**
 * @brief Create a Roaring Bitmap from a range [min, max)
 *
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 * @return Roaring Bitmap handle, or NULL on error
 *
 * @note Caller must call fc_roaring_destroy() to free resources
 * @note Thread-safe: No
 *
 * Time complexity: O((max - min) / 65536)
 * Space complexity: O((max - min) / 65536)
 */
fc_roaring_bitmap_t* fc_roaring_create_from_range(uint32_t min, uint32_t max);

/**
 * @brief Clone a Roaring Bitmap
 *
 * @param bitmap Source bitmap
 * @return Cloned bitmap handle, or NULL on error
 *
 * @note Caller must call fc_roaring_destroy() on the cloned bitmap
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(n) where n = number of containers
 * Space complexity: O(m) where m = memory used by source bitmap
 */
fc_roaring_bitmap_t* fc_roaring_clone(const fc_roaring_bitmap_t* bitmap);

/**
 * @brief Destroy a Roaring Bitmap and free resources
 *
 * @param bitmap Roaring Bitmap handle
 *
 * @note Thread-safe: No
 *
 * Time complexity: O(n) where n = number of containers
 */
void fc_roaring_destroy(fc_roaring_bitmap_t* bitmap);

/**
 * @brief Add a value to the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @param value Value to add
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization for concurrent writes)
 *
 * Time complexity: O(log n) where n = container size
 */
fc_status_t fc_roaring_add(fc_roaring_bitmap_t* bitmap, uint32_t value);

/**
 * @brief Add multiple values to the bitmap (batch operation)
 *
 * @param bitmap Roaring Bitmap handle
 * @param values Array of values to add
 * @param count Number of values
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization for concurrent writes)
 * @note Values do not need to be sorted
 *
 * Time complexity: O(n log n) where n = count
 */
fc_status_t fc_roaring_add_batch(fc_roaring_bitmap_t* bitmap, const uint32_t* values, size_t count);

/**
 * @brief Add a range of values [min, max) to the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization for concurrent writes)
 *
 * Time complexity: O((max - min) / 65536)
 */
fc_status_t fc_roaring_add_range(fc_roaring_bitmap_t* bitmap, uint32_t min, uint32_t max);

/**
 * @brief Remove a value from the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @param value Value to remove
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization for concurrent writes)
 *
 * Time complexity: O(log n) where n = container size
 */
fc_status_t fc_roaring_remove(fc_roaring_bitmap_t* bitmap, uint32_t value);

/**
 * @brief Remove a range of values [min, max) from the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization for concurrent writes)
 *
 * Time complexity: O((max - min) / 65536)
 */
fc_status_t fc_roaring_remove_range(fc_roaring_bitmap_t* bitmap, uint32_t min, uint32_t max);

/**
 * @brief Check if a value is in the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @param value Value to check
 * @param result Output: true if value is present, false otherwise
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(log n) where n = container size
 */
fc_status_t fc_roaring_contains(const fc_roaring_bitmap_t* bitmap, uint32_t value, bool* result);

/**
 * @brief Check multiple values (batch operation)
 *
 * @param bitmap Roaring Bitmap handle
 * @param values Array of values to check
 * @param count Number of values
 * @param results Output array of boolean results
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(n log m) where n = count, m = container size
 */
fc_status_t fc_roaring_contains_batch(
    const fc_roaring_bitmap_t* bitmap,
    const uint32_t* values,
    size_t count,
    bool* results
);

/**
 * @brief Get the cardinality (number of elements) in the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @return Number of elements, or 0 if bitmap is NULL
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(1)
 */
uint64_t fc_roaring_cardinality(const fc_roaring_bitmap_t* bitmap);

/**
 * @brief Check if the bitmap is empty
 *
 * @param bitmap Roaring Bitmap handle
 * @return true if empty, false otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(1)
 */
bool fc_roaring_is_empty(const fc_roaring_bitmap_t* bitmap);

/**
 * @brief Clear all elements from the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization)
 *
 * Time complexity: O(n) where n = number of containers
 */
fc_status_t fc_roaring_clear(fc_roaring_bitmap_t* bitmap);

/**
 * @brief Compute the union of two bitmaps (A ∪ B)
 *
 * @param a First bitmap
 * @param b Second bitmap
 * @return New bitmap containing the union, or NULL on error
 *
 * @note Caller must call fc_roaring_destroy() on the result
 * @note Thread-safe: Yes (safe for concurrent reads)
 * @note Uses SIMD acceleration for bitmap containers
 *
 * Time complexity: O(n + m) where n, m = number of containers
 */
fc_roaring_bitmap_t* fc_roaring_union(const fc_roaring_bitmap_t* a, const fc_roaring_bitmap_t* b);

/**
 * @brief Compute the intersection of two bitmaps (A ∩ B)
 *
 * @param a First bitmap
 * @param b Second bitmap
 * @return New bitmap containing the intersection, or NULL on error
 *
 * @note Caller must call fc_roaring_destroy() on the result
 * @note Thread-safe: Yes (safe for concurrent reads)
 * @note Uses SIMD acceleration for bitmap containers
 *
 * Time complexity: O(min(n, m)) where n, m = number of containers
 */
fc_roaring_bitmap_t* fc_roaring_intersection(
    const fc_roaring_bitmap_t* a,
    const fc_roaring_bitmap_t* b
);

/**
 * @brief Compute the difference of two bitmaps (A - B)
 *
 * @param a First bitmap
 * @param b Second bitmap
 * @return New bitmap containing the difference, or NULL on error
 *
 * @note Caller must call fc_roaring_destroy() on the result
 * @note Thread-safe: Yes (safe for concurrent reads)
 * @note Uses SIMD acceleration for bitmap containers
 *
 * Time complexity: O(n) where n = number of containers in A
 */
fc_roaring_bitmap_t* fc_roaring_difference(
    const fc_roaring_bitmap_t* a,
    const fc_roaring_bitmap_t* b
);

/**
 * @brief Compute the symmetric difference of two bitmaps (A ⊕ B)
 *
 * @param a First bitmap
 * @param b Second bitmap
 * @return New bitmap containing the symmetric difference, or NULL on error
 *
 * @note Caller must call fc_roaring_destroy() on the result
 * @note Thread-safe: Yes (safe for concurrent reads)
 * @note Uses SIMD acceleration for bitmap containers
 *
 * Time complexity: O(n + m) where n, m = number of containers
 */
fc_roaring_bitmap_t* fc_roaring_xor(const fc_roaring_bitmap_t* a, const fc_roaring_bitmap_t* b);

/**
 * @brief Compute union in-place (A = A ∪ B)
 *
 * @param a Bitmap to modify
 * @param b Bitmap to union with
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization)
 *
 * Time complexity: O(n + m) where n, m = number of containers
 */
fc_status_t fc_roaring_union_inplace(fc_roaring_bitmap_t* a, const fc_roaring_bitmap_t* b);

/**
 * @brief Compute intersection in-place (A = A ∩ B)
 *
 * @param a Bitmap to modify
 * @param b Bitmap to intersect with
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization)
 *
 * Time complexity: O(min(n, m)) where n, m = number of containers
 */
fc_status_t fc_roaring_intersection_inplace(fc_roaring_bitmap_t* a, const fc_roaring_bitmap_t* b);

/**
 * @brief Check if two bitmaps are equal
 *
 * @param a First bitmap
 * @param b Second bitmap
 * @return true if equal, false otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(n) where n = number of containers
 */
bool fc_roaring_equals(const fc_roaring_bitmap_t* a, const fc_roaring_bitmap_t* b);

/**
 * @brief Check if bitmap A is a subset of bitmap B (A ⊆ B)
 *
 * @param a First bitmap
 * @param b Second bitmap
 * @return true if A is a subset of B, false otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(n) where n = number of containers in A
 */
bool fc_roaring_is_subset(const fc_roaring_bitmap_t* a, const fc_roaring_bitmap_t* b);

/**
 * @brief Check if two bitmaps intersect (have common elements)
 *
 * @param a First bitmap
 * @param b Second bitmap
 * @return true if bitmaps intersect, false otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(min(n, m)) where n, m = number of containers
 */
bool fc_roaring_intersects(const fc_roaring_bitmap_t* a, const fc_roaring_bitmap_t* b);

/**
 * @brief Get the minimum value in the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @param result Output: minimum value
 * @return FC_OK on success, FC_ERROR_INVALID_ARGUMENT if bitmap is empty
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(log n) where n = container size
 */
fc_status_t fc_roaring_min(const fc_roaring_bitmap_t* bitmap, uint32_t* result);

/**
 * @brief Get the maximum value in the bitmap
 *
 * @param bitmap Roaring Bitmap handle
 * @param result Output: maximum value
 * @return FC_OK on success, FC_ERROR_INVALID_ARGUMENT if bitmap is empty
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(log n) where n = container size
 */
fc_status_t fc_roaring_max(const fc_roaring_bitmap_t* bitmap, uint32_t* result);

/**
 * @brief Convert bitmap to array of values
 *
 * @param bitmap Roaring Bitmap handle
 * @param values Output array (must be pre-allocated with size >= cardinality)
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 * @note Caller must allocate values array with size >= fc_roaring_cardinality()
 *
 * Time complexity: O(n) where n = cardinality
 */
fc_status_t fc_roaring_to_array(const fc_roaring_bitmap_t* bitmap, uint32_t* values);

/**
 * @brief Optimize the bitmap by converting containers to most efficient type
 *
 * @param bitmap Roaring Bitmap handle
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization)
 * @note Automatically converts between array/bitmap/run containers
 *
 * Time complexity: O(n) where n = number of containers
 */
fc_status_t fc_roaring_optimize(fc_roaring_bitmap_t* bitmap);

/**
 * @brief Get bitmap statistics
 *
 * @param bitmap Roaring Bitmap handle
 * @param stats Output statistics structure
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(n) where n = number of containers
 */
fc_status_t fc_roaring_get_stats(const fc_roaring_bitmap_t* bitmap, fc_roaring_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif /* FC_ROARING_BITMAP_H */
