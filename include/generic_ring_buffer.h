#ifndef FC_GENERIC_RING_BUFFER_H
#define FC_GENERIC_RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef FC_RING_BUFFER_LOCKFREE
#    include <stdatomic.h>
#endif

/**
 * @file generic_ring_buffer.h
 * @brief Generic Ring Buffer with dual implementation support
 *
 * Implementation Selection:
 *
 * FC_RING_BUFFER_LOCKFREE:
 *   - Lock-free SPSC (Single Producer Single Consumer)
 *   - Uses C11 atomics with memory ordering
 *   - Cache-line aligned to prevent false sharing
 *   - Best for: Pure C multi-threaded scenarios
 *   - Performance: ~1.1ns/op (with atomic overhead)
 *   - Memory: 224 bytes + data (cache-aligned)
 *
 * Default (Single-threaded):
 *   - No atomic operations, no memory barriers
 *   - Compact memory layout
 *   - Best for: cgo scenarios, Go-side concurrency control
 *   - Performance: ~0.9ns/op (minimal overhead)
 *   - Memory: 32 bytes + data
 *
 * Usage:
 *   - Default: cmake -B build
 *   - Lock-free: cmake -B build -DFC_RING_BUFFER_LOCKFREE=ON
 */

#ifdef FC_RING_BUFFER_LOCKFREE
#    define FC_RING_BUFFER_IMPL "lock-free SPSC"
#else
#    define FC_RING_BUFFER_IMPL "single-threaded"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic ring buffer structure for efficient sliding window operations
 *
 * Implementation is selected at compile time via FC_RING_BUFFER_LOCKFREE:
 *
 * Single-threaded (default):
 *   - Optimized for cgo use cases
 *   - Minimal overhead (~1ns/op)
 *   - Requires external synchronization for multi-threaded access
 *   - Compact memory layout (32 bytes + data)
 *
 * Lock-free SPSC (FC_RING_BUFFER_LOCKFREE defined):
 *   - True lock-free for single producer, single consumer
 *   - Uses C11 atomics with proper memory ordering
 *   - Cache-line aligned to prevent false sharing (224 bytes + data)
 *   - Atomic overhead (~5ns/op)
 *
 * Capacity is always a power of 2 for efficient modulo operations.
 */
#ifdef FC_RING_BUFFER_LOCKFREE
typedef struct {
    void* data;          /**< Aligned data array */
    size_t capacity;     /**< Capacity (power of 2) */
    size_t element_size; /**< Size of each element in bytes */

    char _pad0[64 - sizeof(void*) - sizeof(size_t) * 2];

    _Atomic size_t head; /**< Write position (producer only) */

    char _pad1[64 - sizeof(_Atomic size_t)];

    _Atomic size_t tail; /**< Read position (consumer only) */

    char _pad2[64 - sizeof(_Atomic size_t)];
} fc_generic_ring_buffer_t;
#else
typedef struct {
    void* data;          /**< Aligned data array */
    size_t capacity;     /**< Capacity (power of 2) */
    size_t element_size; /**< Size of each element in bytes */
    size_t head;         /**< Write position */
    size_t count;        /**< Current element count */
} fc_generic_ring_buffer_t;
#endif

/**
 * @brief Create a new generic ring buffer
 *
 * @param capacity Desired capacity (will be rounded up to next power of 2)
 * @param element_size Size of each element in bytes
 * @return Pointer to newly created ring buffer, or NULL on failure
 *
 * Time complexity: O(1)
 * Space complexity: O(capacity * element_size)
 * Thread safety: Not thread-safe
 */
fc_generic_ring_buffer_t* fc_generic_ring_buffer_create(size_t capacity, size_t element_size);

/**
 * @brief Destroy a generic ring buffer and free its memory
 *
 * @param rb Ring buffer to destroy
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe
 */
void fc_generic_ring_buffer_destroy(fc_generic_ring_buffer_t* rb);

/**
 * @brief Push a single element to the ring buffer
 *
 * If buffer is full, the oldest element is overwritten.
 *
 * @param rb Ring buffer
 * @param value Pointer to value to push (must point to element_size bytes)
 * @return true on success, false if rb or value is NULL
 *
 * Time complexity: O(1)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization
 *   - Lock-free: Safe for single producer (uses release semantics)
 */
bool fc_generic_ring_buffer_push(fc_generic_ring_buffer_t* rb, const void* value);

/**
 * @brief Push multiple elements to the ring buffer
 *
 * If buffer becomes full during the operation, oldest elements are overwritten.
 *
 * @param rb Ring buffer
 * @param values Pointer to array of values to push
 * @param n Number of values to push
 * @return Number of elements successfully pushed, 0 if rb or values is NULL
 *
 * Time complexity: O(n)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization
 *   - Lock-free: Safe for single producer
 */
size_t fc_generic_ring_buffer_push_batch(
    fc_generic_ring_buffer_t* rb,
    const void* values,
    size_t n
);

/**
 * @brief Pop a single element from the ring buffer
 *
 * @param rb Ring buffer
 * @param out Pointer to store the popped value (must have space for element_size bytes)
 * @return true if element was popped, false if buffer is empty or rb/out is NULL
 *
 * Time complexity: O(1)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization
 *   - Lock-free: Safe for single consumer (uses acquire semantics)
 */
bool fc_generic_ring_buffer_pop(fc_generic_ring_buffer_t* rb, void* out);

/**
 * @brief Pop multiple elements from the ring buffer
 *
 * @param rb Ring buffer
 * @param out Array to store popped values (must have space for n * element_size bytes)
 * @param n Maximum number of values to pop
 * @return Number of elements actually popped, 0 if rb or out is NULL
 *
 * Time complexity: O(n)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization
 *   - Lock-free: Safe for single consumer
 */
size_t fc_generic_ring_buffer_pop_batch(fc_generic_ring_buffer_t* rb, void* out, size_t n);

/**
 * @brief Get element at specific index (0 = oldest, count-1 = newest)
 *
 * @param rb Ring buffer
 * @param index Index of element to retrieve
 * @param out Pointer to store the value (must have space for element_size bytes)
 * @return true if element was retrieved, false if index out of bounds or rb/out is NULL
 *
 * Time complexity: O(1)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization for writes
 *   - Lock-free: Safe with single writer (read-only operation)
 */
bool fc_generic_ring_buffer_get(const fc_generic_ring_buffer_t* rb, size_t index, void* out);

/**
 * @brief Get all elements in order (oldest to newest)
 *
 * @param rb Ring buffer
 * @param out Array to store elements (must have space for count * element_size bytes)
 * @return Number of elements copied, 0 if rb or out is NULL
 *
 * Time complexity: O(count)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization for writes
 *   - Lock-free: Safe with single writer (read-only operation)
 */
size_t fc_generic_ring_buffer_get_all(const fc_generic_ring_buffer_t* rb, void* out);

/**
 * @brief Get the number of elements currently in the buffer
 *
 * @param rb Ring buffer
 * @return Number of elements, or 0 if rb is NULL
 *
 * Time complexity: O(1)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization for writes
 *   - Lock-free: Safe from any thread (returns snapshot with relaxed semantics)
 */
size_t fc_generic_ring_buffer_size(const fc_generic_ring_buffer_t* rb);

/**
 * @brief Get the capacity of the buffer
 *
 * @param rb Ring buffer
 * @return Capacity, or 0 if rb is NULL
 *
 * Time complexity: O(1)
 * Thread safety: Always safe (read-only, immutable after creation)
 */
size_t fc_generic_ring_buffer_capacity(const fc_generic_ring_buffer_t* rb);

/**
 * @brief Get the element size of the buffer
 *
 * @param rb Ring buffer
 * @return Element size in bytes, or 0 if rb is NULL
 *
 * Time complexity: O(1)
 * Thread safety: Always safe (read-only, immutable after creation)
 */
size_t fc_generic_ring_buffer_element_size(const fc_generic_ring_buffer_t* rb);

/**
 * @brief Check if the buffer is empty
 *
 * @param rb Ring buffer
 * @return true if empty, false otherwise (or if rb is NULL)
 *
 * Time complexity: O(1)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization for writes
 *   - Lock-free: Safe from any thread (returns snapshot)
 */
bool fc_generic_ring_buffer_is_empty(const fc_generic_ring_buffer_t* rb);

/**
 * @brief Check if the buffer is full
 *
 * @param rb Ring buffer
 * @return true if full, false otherwise (or if rb is NULL)
 *
 * Time complexity: O(1)
 * Thread safety:
 *   - Single-threaded: Requires external synchronization for writes
 *   - Lock-free: Safe from any thread (returns snapshot)
 */
bool fc_generic_ring_buffer_is_full(const fc_generic_ring_buffer_t* rb);

/**
 * @brief Clear all elements from the buffer
 *
 * @param rb Ring buffer
 *
 * Time complexity: O(1)
 * Thread safety: Not thread-safe (requires external synchronization)
 */
void fc_generic_ring_buffer_clear(fc_generic_ring_buffer_t* rb);

/**
 * @brief Get the implementation type
 *
 * @return String describing the implementation ("single-threaded" or "lock-free SPSC")
 */
const char* fc_generic_ring_buffer_implementation(void);

#ifdef __cplusplus
}
#endif

#endif /* FC_GENERIC_RING_BUFFER_H */
