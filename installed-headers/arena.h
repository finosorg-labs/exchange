#ifndef FC_DS_ARENA_H
#define FC_DS_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Arena allocator (Bump Allocator) for fast temporary memory allocation
 *
 * Arena allocator provides extremely fast memory allocation by simply bumping
 * a pointer forward. Individual allocations cannot be freed; instead, the
 * entire arena is reset or destroyed at once. This makes it ideal for:
 * - Temporary calculations
 * - Batch processing
 * - Per-request memory pools
 * - Parser/compiler temporary data
 *
 * Time Complexity:
 * - Allocation: O(1)
 * - Reset: O(1)
 * - Destroy: O(1)
 *
 * Space Complexity: O(capacity)
 *
 * Thread Safety: Each arena instance is NOT thread-safe. Use separate arenas
 * per thread or external synchronization.
 */

typedef struct fc_arena fc_arena_t;

/**
 * @brief Create a new arena allocator
 *
 * @param capacity Initial capacity in bytes (must be > 0)
 * @return Pointer to arena, or NULL on allocation failure
 *
 * Time Complexity: O(1)
 */
fc_arena_t* fc_arena_create(size_t capacity);

/**
 * @brief Allocate memory from arena with default alignment
 *
 * @param arena Arena allocator (must not be NULL)
 * @param size Number of bytes to allocate (must be > 0)
 * @return Pointer to allocated memory, or NULL if insufficient space
 *
 * Time Complexity: O(1)
 *
 * Note: Default alignment is sizeof(void*) (typically 8 bytes on 64-bit)
 */
void* fc_arena_alloc(fc_arena_t* arena, size_t size);

/**
 * @brief Allocate memory from arena with specified alignment
 *
 * @param arena Arena allocator (must not be NULL)
 * @param size Number of bytes to allocate (must be > 0)
 * @param alignment Alignment requirement in bytes (must be power of 2)
 * @return Pointer to allocated memory, or NULL if insufficient space
 *
 * Time Complexity: O(1)
 *
 * Note: Alignment must be a power of 2 (1, 2, 4, 8, 16, 32, 64, ...)
 */
void* fc_arena_alloc_aligned(fc_arena_t* arena, size_t size, size_t alignment);

/**
 * @brief Reset arena to initial state, invalidating all allocations
 *
 * @param arena Arena allocator (must not be NULL)
 *
 * Time Complexity: O(1)
 *
 * Note: All pointers returned by previous allocations become invalid
 */
void fc_arena_reset(fc_arena_t* arena);

/**
 * @brief Get current memory usage
 *
 * @param arena Arena allocator (must not be NULL)
 * @return Number of bytes currently allocated
 *
 * Time Complexity: O(1)
 */
size_t fc_arena_used(const fc_arena_t* arena);

/**
 * @brief Get total capacity
 *
 * @param arena Arena allocator (must not be NULL)
 * @return Total capacity in bytes
 *
 * Time Complexity: O(1)
 */
size_t fc_arena_capacity(const fc_arena_t* arena);

/**
 * @brief Get remaining available space
 *
 * @param arena Arena allocator (must not be NULL)
 * @return Number of bytes available for allocation
 *
 * Time Complexity: O(1)
 */
size_t fc_arena_available(const fc_arena_t* arena);

/**
 * @brief Destroy arena and free all memory
 *
 * @param arena Arena allocator (can be NULL)
 *
 * Time Complexity: O(1)
 *
 * Note: After destruction, all pointers returned by allocations are invalid
 */
void fc_arena_destroy(fc_arena_t* arena);

#ifdef __cplusplus
}
#endif

#endif
