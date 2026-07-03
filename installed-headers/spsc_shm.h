#ifndef FC_SPSC_SHM_H
#define FC_SPSC_SHM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status codes for SPSC shared memory operations
 */
typedef enum {
    FC_SPSC_SHM_OK                = 0,
    FC_SPSC_SHM_ERR_INVALID_ARG   = -1,
    FC_SPSC_SHM_ERR_CREATE        = -2,
    FC_SPSC_SHM_ERR_OPEN          = -3,
    FC_SPSC_SHM_ERR_MAP           = -4,
    FC_SPSC_SHM_ERR_UNLINK        = -5,
    FC_SPSC_SHM_ERR_CLOSE         = -6,
    FC_SPSC_SHM_ERR_SIZE_MISMATCH = -7
} fc_spsc_shm_status_t;

/**
 * @brief Opaque handle for shared memory segment
 *
 * Platform-specific implementation details are hidden.
 */
typedef struct fc_spsc_shm {
    void* base;     /**< Mapped base address */
    size_t size;    /**< Size of shared memory segment */
    char name[256]; /**< Shared memory name */
#ifdef _WIN32
    void* handle; /**< Windows file mapping handle */
#else
    int fd; /**< POSIX shared memory file descriptor */
#endif
} fc_spsc_shm_t;

/**
 * @brief Create and map a named shared memory segment
 *
 * Creates a new named shared memory segment and maps it into the process address space.
 * The segment is created with read/write permissions.
 *
 * @param[out] shm    Shared memory handle to initialize
 * @param[in]  name   Unique name for the shared memory segment (max 255 chars)
 * @param[in]  size   Size of the shared memory segment in bytes
 *
 * @return FC_SPSC_SHM_OK on success, error code otherwise
 *
 * @note The caller is responsible for calling fc_spsc_shm_close() when done.
 * @note On Linux/POSIX, the name should not contain '/' except as prefix.
 * @note Thread-safe: Yes (operates on independent handles)
 */
fc_spsc_shm_status_t fc_spsc_shm_create(fc_spsc_shm_t* shm, const char* name, size_t size);

/**
 * @brief Open and map an existing named shared memory segment
 *
 * Opens an existing named shared memory segment and maps it into the process address space.
 *
 * @param[out] shm    Shared memory handle to initialize
 * @param[in]  name   Name of the existing shared memory segment
 * @param[in]  size   Expected size of the shared memory segment
 *
 * @return FC_SPSC_SHM_OK on success, error code otherwise
 *
 * @note The size must match the size used when creating the segment.
 * @note The caller is responsible for calling fc_spsc_shm_close() when done.
 * @note Thread-safe: Yes (operates on independent handles)
 */
fc_spsc_shm_status_t fc_spsc_shm_open(fc_spsc_shm_t* shm, const char* name, size_t size);

/**
 * @brief Unmap and close a shared memory segment
 *
 * Unmaps the shared memory segment from the process address space and closes the handle.
 * Does not delete the named shared memory object.
 *
 * @param[in,out] shm  Shared memory handle to close
 *
 * @return FC_SPSC_SHM_OK on success, error code otherwise
 *
 * @note After closing, the handle should not be used.
 * @note Thread-safe: No (caller must ensure exclusive access to the handle)
 */
fc_spsc_shm_status_t fc_spsc_shm_close(fc_spsc_shm_t* shm);

/**
 * @brief Delete a named shared memory segment
 *
 * Removes the named shared memory object from the system. Should be called by the
 * creator after all processes have closed their mappings.
 *
 * @param[in] name  Name of the shared memory segment to delete
 *
 * @return FC_SPSC_SHM_OK on success, error code otherwise
 *
 * @note On POSIX, this is shm_unlink(). On Windows, the object is deleted when
 *       the last handle is closed.
 * @note Thread-safe: Yes
 */
fc_spsc_shm_status_t fc_spsc_shm_unlink(const char* name);

/**
 * @brief Get the base address of a mapped shared memory segment
 *
 * @param[in] shm  Shared memory handle
 *
 * @return Base address of the mapped memory, or NULL if not mapped
 *
 * @note Thread-safe: Yes (read-only access)
 */
static inline void* fc_spsc_shm_base(const fc_spsc_shm_t* shm) {
    return shm ? shm->base : NULL;
}

/**
 * @brief Get the size of a mapped shared memory segment
 *
 * @param[in] shm  Shared memory handle
 *
 * @return Size of the mapped memory in bytes
 *
 * @note Thread-safe: Yes (read-only access)
 */
static inline size_t fc_spsc_shm_size(const fc_spsc_shm_t* shm) {
    return shm ? shm->size : 0;
}

#ifdef __cplusplus
}
#endif

#endif // FC_SPSC_SHM_H
