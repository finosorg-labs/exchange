/**
 * @file orderbook.h
 * @brief Order book snapshot generation
 *
 * Provides high-performance order book snapshot generation for market data distribution.
 * Aggregates orders at each price level and extracts top N levels for bid/ask sides.
 *
 * Key features:
 * - Price level aggregation (merge orders at same price)
 * - Top N levels extraction (typically 5-10 levels)
 * - Spread, mid-price, and weighted mid-price calculation
 * - Batch snapshot generation for full market (5000+ symbols)
 * - SIMD-optimized aggregation and sorting
 *
 * Performance target: Single symbol snapshot < 10μs
 */

#ifndef FC_ORDERBOOK_H
#define FC_ORDERBOOK_H

#include "error.h"
#include "platform.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Order book side (bid or ask)
 */
typedef enum {
    FC_ORDERBOOK_SIDE_BID = 0, /**< Bid (buy) side */
    FC_ORDERBOOK_SIDE_ASK = 1  /**< Ask (sell) side */
} fc_orderbook_side_t;

/**
 * @brief Price level entry
 *
 * Represents aggregated orders at a single price level.
 */
typedef struct {
    double price;  /**< Price level */
    double volume; /**< Total volume at this price level */
} fc_price_level_t;

/**
 * @brief Order book snapshot
 *
 * Contains top N levels for both bid and ask sides, plus derived metrics.
 */
typedef struct {
    fc_price_level_t* bids;    /**< Bid levels (sorted descending by price) */
    fc_price_level_t* asks;    /**< Ask levels (sorted ascending by price) */
    uint32_t num_bid_levels;   /**< Number of bid levels */
    uint32_t num_ask_levels;   /**< Number of ask levels */
    double spread;             /**< Bid-ask spread (best_ask - best_bid) */
    double mid_price;          /**< Mid price ((best_bid + best_ask) / 2) */
    double weighted_mid_price; /**< Weighted mid price */
    int64_t timestamp_ns;      /**< Snapshot timestamp (nanoseconds) */
    uint32_t symbol_id;        /**< Symbol identifier */
} fc_orderbook_snapshot_t;

/**
 * @brief Raw order entry
 *
 * Input format for order book aggregation.
 */
typedef struct {
    double price;             /**< Order price */
    double volume;            /**< Order volume */
    int64_t timestamp_ns;     /**< Order timestamp */
    uint32_t symbol_id;       /**< Symbol identifier */
    fc_orderbook_side_t side; /**< Order side (bid/ask) */
} fc_order_t;

/**
 * @brief Precision mode for price level aggregation
 */
typedef enum {
    FC_ORDERBOOK_PRECISION_STANDARD = 0, /**< Standard floating-point addition */
    FC_ORDERBOOK_PRECISION_KAHAN    = 1, /**< Kahan summation (recommended) */
    FC_ORDERBOOK_PRECISION_BIGFLOAT = 2  /**< Arbitrary precision using platform bigfloat */
} fc_orderbook_precision_mode_t;

/**
 * @brief Generate order book snapshot from raw orders (pre-sorted)
 *
 * Aggregates orders at each price level and extracts top N levels for both sides.
 * Orders must be pre-sorted by price (descending for bids, ascending for asks).
 *
 * This is the high-performance path for scenarios where orders are already sorted
 * (e.g., extracted from an ordered data structure like a red-black tree).
 *
 * @param[out] snapshot Output snapshot structure (caller must allocate)
 * @param[in] orders Array of orders for a single symbol (must be pre-sorted)
 * @param[in] num_orders Number of orders
 * @param[in] max_levels Maximum number of levels to extract per side
 * @param[in] precision_mode Precision mode for volume aggregation
 * @param[in] timestamp_ns Snapshot timestamp
 * @return FC_STATUS_OK on success, error code otherwise
 *
 * Time complexity: O(num_orders)
 * Space complexity: O(max_levels)
 * Thread safety: Not thread-safe (caller must synchronize)
 *
 * @note Caller must allocate snapshot->bids and snapshot->asks arrays
 *       with at least max_levels elements each before calling.
 * @note Orders must be sorted: bids descending by price, asks ascending by price.
 * @note If there are fewer than max_levels unique price levels, the actual
 *       number of levels will be returned in num_bid_levels/num_ask_levels.
 * @note For unsorted orders, use fc_orderbook_snapshot_generate_unsorted instead.
 */
FC_API fc_status_t fc_orderbook_snapshot_generate(
    fc_orderbook_snapshot_t* snapshot,
    const fc_order_t* orders,
    size_t num_orders,
    uint32_t max_levels,
    fc_orderbook_precision_mode_t precision_mode,
    int64_t timestamp_ns
);

/**
 * @brief Generate order book snapshot from raw orders (auto-sort)
 *
 * Aggregates orders at each price level and extracts top N levels for both sides.
 * Orders can be in any order - they will be automatically sorted by price.
 *
 * This is the convenience path for scenarios where orders are unsorted
 * (e.g., batch processing of historical data, call auction order collection).
 *
 * @param[out] snapshot Output snapshot structure (caller must allocate)
 * @param[in,out] orders Array of orders for a single symbol (will be sorted in-place)
 * @param[in] num_orders Number of orders
 * @param[in] max_levels Maximum number of levels to extract per side
 * @param[in] precision_mode Precision mode for volume aggregation
 * @param[in] timestamp_ns Snapshot timestamp
 * @return FC_STATUS_OK on success, error code otherwise
 *
 * Time complexity: O(num_orders * log(num_orders))
 * Space complexity: O(num_orders + max_levels)
 * Thread safety: Not thread-safe (caller must synchronize)
 *
 * @note Caller must allocate snapshot->bids and snapshot->asks arrays
 *       with at least max_levels elements each before calling.
 * @note Input orders array will be modified (sorted in-place by side and price).
 * @note If orders are already sorted, use fc_orderbook_snapshot_generate for better performance.
 * @note Sorting uses Timsort algorithm: O(n) for nearly sorted data, O(n log n) worst case.
 */
FC_API fc_status_t fc_orderbook_snapshot_generate_unsorted(
    fc_orderbook_snapshot_t* snapshot,
    fc_order_t* orders,
    size_t num_orders,
    uint32_t max_levels,
    fc_orderbook_precision_mode_t precision_mode,
    int64_t timestamp_ns
);

/**
 * @brief Generate snapshots for multiple symbols in batch
 *
 * Processes orders for multiple symbols and generates snapshots in parallel.
 * This is more efficient than calling fc_orderbook_snapshot_generate repeatedly.
 *
 * @param[out] snapshots Output snapshot array (one per symbol)
 * @param[in] orders Array of orders (can contain multiple symbols)
 * @param[in] num_orders Total number of orders
 * @param[in] num_symbols Number of symbols
 * @param[in] max_levels Maximum number of levels per side per symbol
 * @param[in] precision_mode Precision mode for volume aggregation
 * @param[in] timestamp_ns Snapshot timestamp
 * @return FC_STATUS_OK on success, error code otherwise
 *
 * Time complexity: O(num_orders + num_symbols * max_levels * log(max_levels))
 * Space complexity: O(num_symbols * max_levels)
 * Thread safety: Not thread-safe (caller must synchronize)
 *
 * @note Caller must allocate snapshots array with num_symbols elements.
 * @note Caller must allocate bids/asks arrays for each snapshot.
 * @note Orders can be in any order (will be grouped by symbol internally).
 */
FC_API fc_status_t fc_orderbook_snapshot_generate_batch(
    fc_orderbook_snapshot_t* snapshots,
    const fc_order_t* orders,
    size_t num_orders,
    uint32_t num_symbols,
    uint32_t max_levels,
    fc_orderbook_precision_mode_t precision_mode,
    int64_t timestamp_ns
);

/**
 * @brief Aggregate orders at the same price level
 *
 * Merges orders with identical prices by summing their volumes.
 * Input orders must be sorted by price.
 *
 * @param[out] levels Output price levels (caller must allocate)
 * @param[out] num_levels Number of unique price levels
 * @param[in] orders Input orders (must be sorted by price)
 * @param[in] num_orders Number of input orders
 * @param[in] precision_mode Precision mode for volume aggregation
 * @return FC_STATUS_OK on success, error code otherwise
 *
 * Time complexity: O(num_orders)
 * Space complexity: O(1) additional space
 * Thread safety: Not thread-safe
 *
 * @note Input orders must be sorted by price (same direction as output).
 * @note Output levels array must have at least num_orders elements.
 * @note Actual number of levels will be <= num_orders.
 */
FC_API fc_status_t fc_orderbook_aggregate_levels(
    fc_price_level_t* levels,
    uint32_t* num_levels,
    const fc_order_t* orders,
    size_t num_orders,
    fc_orderbook_precision_mode_t precision_mode
);

/**
 * @brief Calculate derived metrics from price levels
 *
 * Computes spread, mid-price, and weighted mid-price from top bid/ask levels.
 *
 * @param[in,out] snapshot Snapshot with bid/ask levels filled
 * @return FC_STATUS_OK on success, error code otherwise
 *
 * Time complexity: O(1)
 * Space complexity: O(1)
 * Thread safety: Not thread-safe
 *
 * @note Requires at least one bid and one ask level.
 * @note Weighted mid-price formula:
 *       (best_bid * best_ask_volume + best_ask * best_bid_volume) /
 *       (best_bid_volume + best_ask_volume)
 */
FC_API fc_status_t fc_orderbook_calculate_metrics(fc_orderbook_snapshot_t* snapshot);

/**
 * @brief Free resources allocated for a snapshot
 *
 * @param[in] snapshot Snapshot to free
 *
 * @note Only frees internal allocations, not the snapshot structure itself.
 * @note Safe to call on zero-initialized or already-freed snapshots.
 */
FC_API void fc_orderbook_snapshot_free(fc_orderbook_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif

#endif // FC_ORDERBOOK_H
