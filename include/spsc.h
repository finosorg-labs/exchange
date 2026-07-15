/**
 * @file spsc.h
 * @brief Lock-free SPSC (Single-Producer Single-Consumer) ring buffer
 *
 * High-performance bounded lock-free queue for single producer and single consumer.
 * Designed for HFT systems with nanosecond-level latency requirements.
 *
 * Key features:
 * - Lock-free wait-free operations
 * - Cache line isolation (head/tail in separate 64B lines)
 * - Local caching of remote sequence numbers
 * - Release/acquire memory ordering
 * - Cross-language support (C/Go share same binary layout)
 * - Zero allocation on hot path
 * - Capacity must be power of 2
 *
 * Performance:
 * - Single push/pop: ~5-15ns (C), ~15-35ns (Go)
 * - Peak throughput: ~100M+ ops/s
 *
 * Thread safety:
 * - Strictly one producer and one consumer
 * - Multiple producer/consumer pairs can use separate rings (zero contention)
 */

#ifndef FC_SPSC_H
#define FC_SPSC_H

#include <error.h>
#include <platform.h>
#include <stddef.h>
#include <stdint.h>

FC_BEGIN_DECLS

/**
 * @brief SPSC-specific status codes (extends fc_status_t range)
 *
 * Note: These values are outside the standard fc_status_t enum range.
 * Cast to int when strict type checking is required.
 */
#define FC_ERR_WOULD_BLOCK (-20)

/**
 * @brief SPSC ring buffer handle
 *
 * The handle is local to each side (producer/consumer) and contains:
 * - Pointers to shared control block fields (head, tail, data)
 * - Local cached sequence numbers (head_cached or tail_cached)
 * - Mask for index calculation (N-1)
 *
 * This is a complete type that can be stack-allocated.
 */
typedef struct fc_spsc {
    void* ctrl;             /* Pointer to shared control block */
    void* data;             /* Pointer to data region */
    uint64_t mask;          /* Capacity - 1 */
    uint64_t cached_remote; /* Producer caches head, consumer caches tail */
    uint32_t elem_size;
    uint32_t backpressure;
} fc_spsc_t;

/**
 * @brief Backpressure strategy when queue is full
 */
typedef enum {
    FC_SPSC_BACKPRESSURE_SPIN = 0, /**< Spin until space available */
    FC_SPSC_BACKPRESSURE_DROP = 1, /**< Drop new element and increment dropped counter */
} fc_spsc_backpressure_t;

/**
 * @brief Calculate required arena size for SPSC ring buffer
 *
 * The arena layout:
 * - Header: 64B (magic, version, capacity, elem_size, flags, ready)
 * - Producer line: 64B (tail + padding)
 * - Consumer line: 64B (head + padding)
 * - Observer line: 64B (dropped, high_watermark + padding)
 * - Data region: N * elem_size (page-aligned start)
 *
 * @param capacity_pow2 Capacity (must be power of 2)
 * @param elem_size Element size in bytes
 * @return Required memory size in bytes, or 0 on error
 *
 * Time complexity: O(1)
 * Thread safety: Thread-safe (pure function)
 */
FC_API size_t fc_spsc_arena_size(uint32_t capacity_pow2, uint32_t elem_size);

/**
 * @brief Initialize SPSC ring buffer control block
 *
 * Creates a new SPSC ring in caller-provided memory. The creator should call
 * this once, then other sides should call fc_spsc_attach() to map the same memory.
 *
 * @param q Output handle (caller-allocated)
 * @param mem Memory region (must be 64-byte aligned)
 * @param mem_size Size of memory region (>= fc_spsc_arena_size(...))
 * @param capacity_pow2 Capacity (must be power of 2)
 * @param elem_size Element size in bytes
 * @param backpressure Backpressure strategy for full queue
 * @return FC_OK on success, error code otherwise
 *
 * Errors:
 * - FC_ERR_INVALID_ARG: Invalid capacity (not power of 2), invalid size
 * - FC_ERR_INVALID_ARG: Memory not aligned or too small
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe (caller must ensure single init)
 */
FC_API fc_status_t fc_spsc_init(
    fc_spsc_t* q,
    void* mem,
    size_t mem_size,
    uint32_t capacity_pow2,
    uint32_t elem_size,
    fc_spsc_backpressure_t backpressure
);

/**
 * @brief Attach to existing SPSC ring buffer
 *
 * Maps to an already initialized SPSC ring. Validates magic, version, and layout.
 * Waits (with backoff) until the ready flag is set by the creator.
 *
 * @param q Output handle (caller-allocated)
 * @param mem Memory region containing initialized control block
 * @param mem_size Size of memory region
 * @return FC_OK on success, error code otherwise
 *
 * Errors:
 * - FC_ERR_INVALID_ARG: Invalid magic or version
 * - FC_ERR_INVALID_ARG: Memory size mismatch
 *
 * Time complexity: O(1) after ready flag is set
 * Thread safety: Not thread-safe (caller must ensure single attach per side)
 */
FC_API fc_status_t fc_spsc_attach(fc_spsc_t* q, void* mem, size_t mem_size);

/**
 * @brief Push single element into queue (producer side)
 *
 * Algorithm:
 * 1. Load tail with relaxed (producer owns tail)
 * 2. Check if full using local head_cached
 * 3. If locally full, refresh head_cached with acquire
 * 4. If truly full, apply backpressure strategy
 * 5. Write element to slots[tail & MASK]
 * 6. Store tail+1 with release (publish to consumer)
 *
 * @param q Producer handle
 * @param elem Element to push (elem_size bytes)
 * @return FC_OK on success, FC_ERR_WOULD_BLOCK if full (SPIN mode only)
 *
 * Time complexity: O(1), ~5-15ns typical
 * Thread safety: Single producer only
 */
FC_API fc_status_t fc_spsc_push(fc_spsc_t* q, const void* elem);

/**
 * @brief Pop single element from queue (consumer side)
 *
 * Algorithm:
 * 1. Load head with relaxed (consumer owns head)
 * 2. Check if empty using local tail_cached
 * 3. If locally empty, refresh tail_cached with acquire
 * 4. If truly empty, return FC_ERR_WOULD_BLOCK
 * 5. Read element from slots[head & MASK]
 * 6. Store head+1 with release (free slot for producer)
 *
 * @param q Consumer handle
 * @param elem_out Output buffer (elem_size bytes)
 * @return FC_OK on success, FC_ERR_WOULD_BLOCK if empty
 *
 * Time complexity: O(1), ~5-15ns typical
 * Thread safety: Single consumer only
 */
FC_API fc_status_t fc_spsc_pop(fc_spsc_t* q, void* elem_out);

/**
 * @brief Push multiple elements in batch (producer side)
 *
 * More efficient than repeated fc_spsc_push() calls as it amortizes
 * the release barrier over multiple elements.
 *
 * @param q Producer handle
 * @param elems Array of elements (n * elem_size bytes)
 * @param n Number of elements to push
 * @return Number of elements actually pushed (0 to n)
 *
 * Time complexity: O(n)
 * Thread safety: Single producer only
 */
FC_API size_t fc_spsc_push_bulk(fc_spsc_t* q, const void* elems, size_t n);

/**
 * @brief Pop multiple elements in batch (consumer side)
 *
 * More efficient than repeated fc_spsc_pop() calls as it amortizes
 * the acquire barrier over multiple elements.
 *
 * @param q Consumer handle
 * @param elems_out Output buffer (max_n * elem_size bytes)
 * @param max_n Maximum number of elements to pop
 * @return Number of elements actually popped (0 to max_n)
 *
 * Time complexity: O(n)
 * Thread safety: Single consumer only
 */
FC_API size_t fc_spsc_pop_bulk(fc_spsc_t* q, void* elems_out, size_t max_n);

/**
 * @brief Get current queue length
 *
 * Returns the number of elements currently in the queue.
 * Uses relaxed loads, suitable for monitoring but not synchronization.
 *
 * @param q Handle (producer or consumer)
 * @return Current length (0 to capacity)
 *
 * Time complexity: O(1)
 * Thread safety: Safe from any thread (monitoring only)
 */
FC_API uint64_t fc_spsc_len(const fc_spsc_t* q);

/**
 * @brief Get dropped element count
 *
 * Number of elements dropped due to full queue (DROP backpressure mode only).
 * Uses relaxed load, suitable for monitoring.
 *
 * @param q Handle (producer or consumer)
 * @return Total dropped count since initialization
 *
 * Time complexity: O(1)
 * Thread safety: Safe from any thread (monitoring only)
 */
FC_API uint64_t fc_spsc_dropped(const fc_spsc_t* q);

/**
 * @brief Get high watermark
 *
 * Maximum queue length observed since initialization.
 * Uses relaxed load, suitable for monitoring.
 *
 * @param q Handle (producer or consumer)
 * @return High watermark (0 to capacity)
 *
 * Time complexity: O(1)
 * Thread safety: Safe from any thread (monitoring only)
 */
FC_API uint64_t fc_spsc_high_watermark(const fc_spsc_t* q);

/**
 * @brief Get queue capacity
 *
 * @param q Handle (producer or consumer)
 * @return Capacity (power of 2)
 *
 * Time complexity: O(1)
 * Thread safety: Thread-safe
 */
FC_API uint32_t fc_spsc_capacity(const fc_spsc_t* q);

/**
 * @brief Get element size
 *
 * @param q Handle (producer or consumer)
 * @return Element size in bytes
 *
 * Time complexity: O(1)
 * Thread safety: Thread-safe
 */
FC_API uint32_t fc_spsc_elem_size(const fc_spsc_t* q);

FC_END_DECLS

#endif /* FC_SPSC_H */
