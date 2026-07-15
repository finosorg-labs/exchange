/**
 * @file spsc_layout.h
 * @brief SPSC ring buffer memory layout constants (C/Go ABI contract)
 *
 * This header defines the binary memory layout shared between C and Go implementations.
 * Any changes to these constants must be synchronized with ds-c/spsc/layout.go.
 *
 * Memory layout:
 *   - Cache Line 0 (0-63):    Header (magic, version, capacity, elem_size, flags, ready)
 *   - Cache Line 1 (64-127):  Producer line (tail + padding)
 *   - Cache Line 2 (128-191): Consumer line (head + padding)
 *   - Cache Line 3 (192-255): Observability (dropped, high_watermark + padding)
 *   - Page-aligned:           Data slots begin at page boundary (typically 4096)
 *
 * Each hot atomic field (tail, head) occupies its own cache line to prevent
 * false sharing between producer and consumer.
 */

#ifndef FC_SPSC_LAYOUT_H
#define FC_SPSC_LAYOUT_H

#include <stdint.h>

/* Cache line and page size for alignment */
#define FC_SPSC_CACHE_LINE_SIZE 64
#define FC_SPSC_PAGE_SIZE       4096

/* Header field offsets (Cache Line 0: bytes 0-63) */
#define FC_SPSC_OFFSET_MAGIC    0  /* uint32: Magic number 'SPSC' (0x43535053) */
#define FC_SPSC_OFFSET_VERSION  4  /* uint32: Layout version */
#define FC_SPSC_OFFSET_CAPACITY 8  /* uint32: Ring capacity (must be power of 2) */
#define FC_SPSC_OFFSET_ELEMSIZE 12 /* uint32: Element size in bytes */
#define FC_SPSC_OFFSET_FLAGS    16 /* uint32: Configuration flags */
#define FC_SPSC_OFFSET_READY    20 /* uint32: Ready flag */
/* Bytes 24-63: reserved/padding */

/* Producer line offsets (Cache Line 1: bytes 64-127) */
#define FC_SPSC_OFFSET_TAIL 64 /* uint64: Producer sequence number (atomic) */
/* Bytes 72-127: padding */

/* Consumer line offsets (Cache Line 2: bytes 128-191) */
#define FC_SPSC_OFFSET_HEAD 128 /* uint64: Consumer sequence number (atomic) */
/* Bytes 136-191: padding */

/* Observability line offsets (Cache Line 3: bytes 192-255) */
#define FC_SPSC_OFFSET_DROPPED   192 /* uint64: Drop counter */
#define FC_SPSC_OFFSET_HIGHWATER 200 /* uint64: High watermark */
/* Bytes 208-255: padding */

/* Control block size (4 cache lines) */
#define FC_SPSC_CONTROL_BLOCK_SIZE 256

/* Magic number 'SPSC' in little-endian byte order */
#define FC_SPSC_MAGIC_NUMBER 0x43535053u

/* Current layout version */
#define FC_SPSC_LAYOUT_VERSION 1u

#endif /* FC_SPSC_LAYOUT_H */
