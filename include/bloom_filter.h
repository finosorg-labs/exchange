/**
 * @file bloom_filter.h
 * @brief Bloom Filter - Space-efficient probabilistic data structure
 *
 * A Bloom filter is a space-efficient probabilistic data structure used to test
 * whether an element is a member of a set. False positive matches are possible,
 * but false negatives are not.
 *
 * Features:
 * - Configurable false positive rate
 * - Multiple hash functions using xxHash
 * - Batch operations for high performance
 * - Memory-efficient bit array storage
 * - Thread-safe for concurrent reads (writes require external synchronization)
 *
 * Typical use cases:
 * - Cache filtering
 * - Duplicate detection
 * - Set membership testing with acceptable false positive rate
 */

#ifndef FC_BLOOM_FILTER_H
#define FC_BLOOM_FILTER_H

#include <error.h>
#include <platform.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque Bloom filter handle
 */
typedef struct fc_bloom_filter fc_bloom_filter_t;

/**
 * @brief Bloom filter configuration
 */
typedef struct {
    size_t expected_elements;   /**< Expected number of elements */
    double false_positive_rate; /**< Desired false positive rate (0.0 to 1.0) */
} fc_bloom_config_t;

/**
 * @brief Bloom filter statistics
 */
typedef struct {
    size_t bit_array_size;     /**< Size of bit array in bits */
    size_t num_hash_functions; /**< Number of hash functions */
    size_t elements_added;     /**< Number of elements added */
    double estimated_fpp;      /**< Estimated false positive probability */
    size_t memory_bytes;       /**< Total memory usage in bytes */
} fc_bloom_stats_t;

/**
 * @brief Create a new Bloom filter with optimal parameters
 *
 * Automatically calculates optimal bit array size and number of hash functions
 * based on expected elements and desired false positive rate.
 *
 * @param config Configuration parameters
 * @return Bloom filter handle, or NULL on error
 *
 * @note Caller must call fc_bloom_destroy() to free resources
 * @note Thread-safe: No
 *
 * Time complexity: O(1)
 * Space complexity: O(m) where m = optimal bit array size
 */
fc_bloom_filter_t* fc_bloom_create(const fc_bloom_config_t* config);

/**
 * @brief Create a Bloom filter with explicit parameters
 *
 * @param bit_array_size Size of bit array in bits
 * @param num_hash_functions Number of hash functions to use
 * @return Bloom filter handle, or NULL on error
 *
 * @note Caller must call fc_bloom_destroy() to free resources
 * @note Thread-safe: No
 *
 * Time complexity: O(1)
 * Space complexity: O(m) where m = bit_array_size
 */
fc_bloom_filter_t* fc_bloom_create_explicit(size_t bit_array_size, size_t num_hash_functions);

/**
 * @brief Destroy a Bloom filter and free resources
 *
 * @param filter Bloom filter handle
 *
 * @note Thread-safe: No
 *
 * Time complexity: O(1)
 */
void fc_bloom_destroy(fc_bloom_filter_t* filter);

/**
 * @brief Add an element to the Bloom filter
 *
 * @param filter Bloom filter handle
 * @param data Element data
 * @param len Element length in bytes
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization for concurrent writes)
 *
 * Time complexity: O(k) where k = number of hash functions
 */
fc_status_t fc_bloom_add(fc_bloom_filter_t* filter, const void* data, size_t len);

/**
 * @brief Add multiple elements to the Bloom filter (batch operation)
 *
 * @param filter Bloom filter handle
 * @param data Array of element pointers
 * @param lengths Array of element lengths
 * @param count Number of elements
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization for concurrent writes)
 * @note Batch operations amortize overhead for better performance
 *
 * Time complexity: O(n * k) where n = count, k = number of hash functions
 */
fc_status_t fc_bloom_add_batch(
    fc_bloom_filter_t* filter,
    const void* const* data,
    const size_t* lengths,
    size_t count
);

/**
 * @brief Check if an element might be in the set
 *
 * @param filter Bloom filter handle
 * @param data Element data
 * @param len Element length in bytes
 * @param result Output: true if element might be present, false if definitely not present
 * @return FC_OK on success, error code otherwise
 *
 * @note False positives are possible, false negatives are not
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(k) where k = number of hash functions
 */
fc_status_t fc_bloom_contains(
    const fc_bloom_filter_t* filter,
    const void* data,
    size_t len,
    bool* result
);

/**
 * @brief Check multiple elements (batch operation)
 *
 * @param filter Bloom filter handle
 * @param data Array of element pointers
 * @param lengths Array of element lengths
 * @param count Number of elements
 * @param results Output array of boolean results
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 * @note Batch operations amortize overhead for better performance
 *
 * Time complexity: O(n * k) where n = count, k = number of hash functions
 */
fc_status_t fc_bloom_contains_batch(
    const fc_bloom_filter_t* filter,
    const void* const* data,
    const size_t* lengths,
    size_t count,
    bool* results
);

/**
 * @brief Clear all elements from the Bloom filter
 *
 * Resets the bit array to all zeros, effectively removing all elements.
 *
 * @param filter Bloom filter handle
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: No (requires external synchronization)
 *
 * Time complexity: O(m) where m = bit array size
 */
fc_status_t fc_bloom_clear(fc_bloom_filter_t* filter);

/**
 * @brief Get Bloom filter statistics
 *
 * @param filter Bloom filter handle
 * @param stats Output statistics structure
 * @return FC_OK on success, error code otherwise
 *
 * @note Thread-safe: Yes (safe for concurrent reads)
 *
 * Time complexity: O(1)
 */
fc_status_t fc_bloom_get_stats(const fc_bloom_filter_t* filter, fc_bloom_stats_t* stats);

/**
 * @brief Calculate optimal bit array size for given parameters
 *
 * Uses formula: m = -n * ln(p) / (ln(2)^2)
 * where n = expected elements, p = false positive rate
 *
 * @param expected_elements Expected number of elements
 * @param false_positive_rate Desired false positive rate
 * @return Optimal bit array size in bits
 *
 * Time complexity: O(1)
 */
size_t fc_bloom_optimal_size(size_t expected_elements, double false_positive_rate);

/**
 * @brief Calculate optimal number of hash functions
 *
 * Uses formula: k = (m/n) * ln(2)
 * where m = bit array size, n = expected elements
 *
 * @param bit_array_size Size of bit array in bits
 * @param expected_elements Expected number of elements
 * @return Optimal number of hash functions
 *
 * Time complexity: O(1)
 */
size_t fc_bloom_optimal_hash_count(size_t bit_array_size, size_t expected_elements);

#ifdef __cplusplus
}
#endif

#endif /* FC_BLOOM_FILTER_H */
