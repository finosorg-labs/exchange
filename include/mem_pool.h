/**
 * @file mem_pool.h
 * @brief Fixed-size memory pool for high-frequency allocation
 *
 * Provides O(1) allocation/deallocation of fixed-size memory blocks using
 * a pre-allocated pool with lock-free free list management. Designed for
 * high-frequency trading scenarios where dynamic allocation overhead is
 * unacceptable.
 *
 * Features:
 * - O(1) allocation and deallocation
 * - Lock-free thread-safe operations using CAS
 * - Cache-line aligned blocks to avoid false sharing
 * - Batch allocation/deallocation support
 * - Zero allocation overhead after pool creation
 *
 * Typical use cases:
 * - Order object allocation in high-frequency trading
 * - Market data snapshot buffering
 * - Order book node management
 * - Temporary buffer management in crypto trading
 */

#ifndef FC_MEM_POOL_H
#define FC_MEM_POOL_H

#include "error.h"
#include "platform.h"
#include <stddef.h>

FC_BEGIN_DECLS

/**
 * @brief Opaque memory pool handle
 */
typedef struct fc_mem_pool fc_mem_pool_t;

/**
 * @brief Memory pool statistics
 */
typedef struct {
    size_t block_size;       /**< Size of each block in bytes */
    size_t total_blocks;     /**< Total number of blocks in pool */
    size_t used_blocks;      /**< Number of currently allocated blocks */
    size_t available_blocks; /**< Number of available blocks */
    size_t peak_usage;       /**< Peak number of blocks used */
    size_t alloc_count;      /**< Total allocation count */
    size_t free_count;       /**< Total free count */
} fc_mem_pool_stats_t;

/**
 * @brief Create a fixed-size memory pool
 *
 * Allocates a pool with the specified block size and number of blocks.
 * All blocks are cache-line aligned (64 bytes) to avoid false sharing.
 *
 * Time complexity: O(n) where n is num_blocks
 * Space complexity: O(n * block_size)
 *
 * @param block_size Size of each memory block in bytes (must be > 0)
 * @param num_blocks Number of blocks to pre-allocate (must be > 0)
 *
 * @return Pointer to created pool, NULL on failure
 *
 * @note Caller must call fc_ds_mem_pool_destroy() to free resources
 * @note Block size is rounded up to cache line alignment
 * @note Thread-safe: can be called concurrently
 */
FC_API fc_mem_pool_t* fc_ds_mem_pool_create(size_t block_size, size_t num_blocks);

/**
 * @brief Destroy a memory pool
 *
 * Frees all resources associated with the pool. Any outstanding allocated
 * blocks become invalid after this call.
 *
 * Time complexity: O(1)
 *
 * @param pool Pool to destroy (can be NULL)
 *
 * @warning Do not free blocks after destroying the pool
 * @warning Ensure all allocated blocks are freed before destroying
 * @note Thread-safe: but caller must ensure no concurrent allocations
 */
FC_API void fc_ds_mem_pool_destroy(fc_mem_pool_t* pool);

/**
 * @brief Allocate a single block from the pool
 *
 * Returns a pointer to a memory block of size block_size (specified at
 * pool creation). The memory is uninitialized.
 *
 * Time complexity: O(1)
 *
 * @param pool Memory pool (must not be NULL)
 *
 * @return Pointer to allocated block, NULL if pool is exhausted
 *
 * @note Memory is uninitialized, caller must initialize before use
 * @note Thread-safe: uses lock-free CAS operations
 */
FC_API void* fc_ds_mem_pool_alloc(fc_mem_pool_t* pool);

/**
 * @brief Free a single block back to the pool
 *
 * Returns a previously allocated block to the pool for reuse.
 *
 * Time complexity: O(1)
 *
 * @param pool Memory pool (must not be NULL)
 * @param ptr Pointer to block allocated from this pool (must not be NULL)
 *
 * @warning ptr must have been allocated from this pool
 * @warning Do not free the same block twice
 * @note Thread-safe: uses lock-free CAS operations
 */
FC_API void fc_ds_mem_pool_free(fc_mem_pool_t* pool, void* ptr);

/**
 * @brief Allocate multiple blocks in batch
 *
 * Allocates count blocks and stores pointers in the ptrs array.
 * If fewer than count blocks are available, allocates as many as possible.
 *
 * Time complexity: O(count)
 *
 * @param pool Memory pool (must not be NULL)
 * @param count Number of blocks to allocate (must be > 0)
 * @param ptrs Output array for block pointers (must not be NULL, size >= count)
 *
 * @return Number of blocks actually allocated (0 to count)
 *
 * @note Partial allocation is possible if pool is nearly exhausted
 * @note Thread-safe: uses lock-free CAS operations
 */
FC_API size_t fc_ds_mem_pool_alloc_batch(fc_mem_pool_t* pool, size_t count, void** ptrs);

/**
 * @brief Free multiple blocks in batch
 *
 * Returns count blocks to the pool for reuse.
 *
 * Time complexity: O(count)
 *
 * @param pool Memory pool (must not be NULL)
 * @param ptrs Array of block pointers (must not be NULL)
 * @param count Number of blocks to free (must be > 0)
 *
 * @warning All pointers must have been allocated from this pool
 * @warning Do not free the same block twice
 * @note Thread-safe: uses lock-free CAS operations
 */
FC_API void fc_ds_mem_pool_free_batch(fc_mem_pool_t* pool, void* const* ptrs, size_t count);

/**
 * @brief Get number of available blocks
 *
 * Returns the current number of free blocks in the pool.
 *
 * Time complexity: O(1)
 *
 * @param pool Memory pool (must not be NULL)
 *
 * @return Number of available blocks
 *
 * @note This is a snapshot value, may change immediately in multi-threaded use
 * @note Thread-safe: uses atomic load
 */
FC_API size_t fc_ds_mem_pool_available(const fc_mem_pool_t* pool);

/**
 * @brief Get number of used blocks
 *
 * Returns the current number of allocated blocks.
 *
 * Time complexity: O(1)
 *
 * @param pool Memory pool (must not be NULL)
 *
 * @return Number of used blocks
 *
 * @note This is a snapshot value, may change immediately in multi-threaded use
 * @note Thread-safe: uses atomic load
 */
FC_API size_t fc_ds_mem_pool_used(const fc_mem_pool_t* pool);

/**
 * @brief Get total number of blocks
 *
 * Returns the total capacity of the pool (used + available).
 *
 * Time complexity: O(1)
 *
 * @param pool Memory pool (must not be NULL)
 *
 * @return Total number of blocks
 *
 * @note This value is constant after pool creation
 * @note Thread-safe: read-only access
 */
FC_API size_t fc_ds_mem_pool_capacity(const fc_mem_pool_t* pool);

/**
 * @brief Get block size
 *
 * Returns the size of each block in bytes.
 *
 * Time complexity: O(1)
 *
 * @param pool Memory pool (must not be NULL)
 *
 * @return Block size in bytes
 *
 * @note This value is constant after pool creation
 * @note Thread-safe: read-only access
 */
FC_API size_t fc_ds_mem_pool_block_size(const fc_mem_pool_t* pool);

/**
 * @brief Get detailed pool statistics
 *
 * Returns comprehensive statistics about pool usage.
 *
 * Time complexity: O(1)
 *
 * @param pool Memory pool (must not be NULL)
 * @param stats Output statistics structure (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Statistics are snapshots and may be inconsistent in multi-threaded use
 * @note Thread-safe: uses atomic loads
 */
FC_API fc_status_t fc_ds_mem_pool_get_stats(const fc_mem_pool_t* pool, fc_mem_pool_stats_t* stats);

/**
 * @brief Reset pool statistics
 *
 * Resets peak usage and allocation/free counters to zero.
 * Does not affect current allocation state.
 *
 * Time complexity: O(1)
 *
 * @param pool Memory pool (must not be NULL)
 *
 * @note Thread-safe: uses atomic stores
 */
FC_API void fc_ds_mem_pool_reset_stats(fc_mem_pool_t* pool);

FC_END_DECLS

#endif /* FC_MEM_POOL_H */
