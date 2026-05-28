package exchange

/*
#include "orderbook.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"time"
	"unsafe"
)

// OrderBookSide represents the side of an order (bid or ask)
type OrderBookSide int

const (
	// OrderBookSideBid represents the bid (buy) side
	OrderBookSideBid OrderBookSide = 0
	// OrderBookSideAsk represents the ask (sell) side
	OrderBookSideAsk OrderBookSide = 1
)

// OrderBookPrecisionMode represents the precision mode for volume aggregation
type OrderBookPrecisionMode int

const (
	// OrderBookPrecisionStandard uses standard floating-point addition
	OrderBookPrecisionStandard OrderBookPrecisionMode = 0
	// OrderBookPrecisionKahan uses Kahan summation (recommended)
	OrderBookPrecisionKahan OrderBookPrecisionMode = 1
	// OrderBookPrecisionBigFloat uses arbitrary precision
	OrderBookPrecisionBigFloat OrderBookPrecisionMode = 2
)

// PriceLevel represents aggregated orders at a single price level
type PriceLevel struct {
	Price  float64
	Volume float64
}

// Order represents a raw order entry
type Order struct {
	SymbolID  uint32
	Price     float64
	Volume    float64
	Side      OrderBookSide
	Timestamp time.Time
}

// OrderBookSnapshot represents an order book snapshot with top N levels
type OrderBookSnapshot struct {
	Bids             []PriceLevel
	Asks             []PriceLevel
	Spread           float64
	MidPrice         float64
	WeightedMidPrice float64
	Timestamp        time.Time
	SymbolID         uint32
}

// GenerateSnapshot generates an order book snapshot from pre-sorted orders
//
// This is the high-performance path for scenarios where orders are already sorted
// (e.g., extracted from an ordered data structure like a red-black tree).
//
// Parameters:
//   - orders: Array of orders for a single symbol (must be pre-sorted by price)
//   - maxLevels: Maximum number of levels to extract per side
//   - precisionMode: Precision mode for volume aggregation
//   - timestamp: Snapshot timestamp
//
// Returns:
//   - OrderBookSnapshot with top N levels for both bid and ask sides
//   - error if generation fails
//
// Note: Orders must be sorted: bids descending by price, asks ascending by price.
// For unsorted orders, use GenerateSnapshotUnsorted instead.
func GenerateSnapshot(orders []Order, maxLevels uint32, precisionMode OrderBookPrecisionMode, timestamp time.Time) (*OrderBookSnapshot, error) {
	if len(orders) == 0 {
		return &OrderBookSnapshot{
			Bids:      []PriceLevel{},
			Asks:      []PriceLevel{},
			Timestamp: timestamp,
		}, nil
	}

	// Allocate C memory for orders to avoid Go pointer issues
	cOrders := (*C.fc_order_t)(C.malloc(C.size_t(len(orders)) * C.size_t(unsafe.Sizeof(C.fc_order_t{}))))
	if cOrders == nil {
		return nil, fmt.Errorf("failed to allocate memory for orders")
	}
	defer C.free(unsafe.Pointer(cOrders))

	// Convert Go orders to C orders
	cOrdersSlice := unsafe.Slice(cOrders, len(orders))
	for i, order := range orders {
		cOrdersSlice[i].symbol_id = C.uint32_t(order.SymbolID)
		cOrdersSlice[i].price = C.double(order.Price)
		cOrdersSlice[i].volume = C.double(order.Volume)
		cOrdersSlice[i].side = C.fc_orderbook_side_t(order.Side)
		cOrdersSlice[i].timestamp_ns = C.int64_t(order.Timestamp.UnixNano())
	}

	// Allocate C memory for bids and asks
	cBids := (*C.fc_price_level_t)(C.malloc(C.size_t(maxLevels) * C.size_t(unsafe.Sizeof(C.fc_price_level_t{}))))
	if cBids == nil {
		return nil, fmt.Errorf("failed to allocate memory for bids")
	}
	defer C.free(unsafe.Pointer(cBids))

	cAsks := (*C.fc_price_level_t)(C.malloc(C.size_t(maxLevels) * C.size_t(unsafe.Sizeof(C.fc_price_level_t{}))))
	if cAsks == nil {
		return nil, fmt.Errorf("failed to allocate memory for asks")
	}
	defer C.free(unsafe.Pointer(cAsks))

	// Create C snapshot structure
	var cSnapshot C.fc_orderbook_snapshot_t
	cSnapshot.bids = cBids
	cSnapshot.asks = cAsks

	// Call C function
	status := C.fc_orderbook_snapshot_generate(
		&cSnapshot,
		cOrders,
		C.size_t(len(orders)),
		C.uint32_t(maxLevels),
		C.fc_orderbook_precision_mode_t(precisionMode),
		C.int64_t(timestamp.UnixNano()),
	)

	if status != C.FC_OK {
		return nil, fmt.Errorf("failed to generate snapshot: status %d", status)
	}

	// Convert C snapshot to Go snapshot
	snapshot := &OrderBookSnapshot{
		Bids:             make([]PriceLevel, int(cSnapshot.num_bid_levels)),
		Asks:             make([]PriceLevel, int(cSnapshot.num_ask_levels)),
		Spread:           float64(cSnapshot.spread),
		MidPrice:         float64(cSnapshot.mid_price),
		WeightedMidPrice: float64(cSnapshot.weighted_mid_price),
		Timestamp:        time.Unix(0, int64(cSnapshot.timestamp_ns)),
		SymbolID:         uint32(cSnapshot.symbol_id),
	}

	cBidsSlice := unsafe.Slice(cBids, int(cSnapshot.num_bid_levels))
	for i := 0; i < int(cSnapshot.num_bid_levels); i++ {
		snapshot.Bids[i] = PriceLevel{
			Price:  float64(cBidsSlice[i].price),
			Volume: float64(cBidsSlice[i].volume),
		}
	}

	cAsksSlice := unsafe.Slice(cAsks, int(cSnapshot.num_ask_levels))
	for i := 0; i < int(cSnapshot.num_ask_levels); i++ {
		snapshot.Asks[i] = PriceLevel{
			Price:  float64(cAsksSlice[i].price),
			Volume: float64(cAsksSlice[i].volume),
		}
	}

	return snapshot, nil
}

// GenerateSnapshotUnsorted generates an order book snapshot from unsorted orders
//
// This is the convenience path for scenarios where orders are unsorted
// (e.g., batch processing of historical data, call auction order collection).
// Orders will be automatically sorted by price.
//
// Parameters:
//   - orders: Array of orders for a single symbol (can be in any order)
//   - maxLevels: Maximum number of levels to extract per side
//   - precisionMode: Precision mode for volume aggregation
//   - timestamp: Snapshot timestamp
//
// Returns:
//   - OrderBookSnapshot with top N levels for both bid and ask sides
//   - error if generation fails
//
// Note: Input orders array will be modified (sorted in-place by side and price).
// If orders are already sorted, use GenerateSnapshot for better performance.
func GenerateSnapshotUnsorted(orders []Order, maxLevels uint32, precisionMode OrderBookPrecisionMode, timestamp time.Time) (*OrderBookSnapshot, error) {
	if len(orders) == 0 {
		return &OrderBookSnapshot{
			Bids:      []PriceLevel{},
			Asks:      []PriceLevel{},
			Timestamp: timestamp,
		}, nil
	}

	// Allocate C memory for orders to avoid Go pointer issues
	cOrders := (*C.fc_order_t)(C.malloc(C.size_t(len(orders)) * C.size_t(unsafe.Sizeof(C.fc_order_t{}))))
	if cOrders == nil {
		return nil, fmt.Errorf("failed to allocate memory for orders")
	}
	defer C.free(unsafe.Pointer(cOrders))

	// Convert Go orders to C orders
	cOrdersSlice := unsafe.Slice(cOrders, len(orders))
	for i, order := range orders {
		cOrdersSlice[i].symbol_id = C.uint32_t(order.SymbolID)
		cOrdersSlice[i].price = C.double(order.Price)
		cOrdersSlice[i].volume = C.double(order.Volume)
		cOrdersSlice[i].side = C.fc_orderbook_side_t(order.Side)
		cOrdersSlice[i].timestamp_ns = C.int64_t(order.Timestamp.UnixNano())
	}

	// Allocate C memory for bids and asks
	cBids := (*C.fc_price_level_t)(C.malloc(C.size_t(maxLevels) * C.size_t(unsafe.Sizeof(C.fc_price_level_t{}))))
	if cBids == nil {
		return nil, fmt.Errorf("failed to allocate memory for bids")
	}
	defer C.free(unsafe.Pointer(cBids))

	cAsks := (*C.fc_price_level_t)(C.malloc(C.size_t(maxLevels) * C.size_t(unsafe.Sizeof(C.fc_price_level_t{}))))
	if cAsks == nil {
		return nil, fmt.Errorf("failed to allocate memory for asks")
	}
	defer C.free(unsafe.Pointer(cAsks))

	// Create C snapshot structure
	var cSnapshot C.fc_orderbook_snapshot_t
	cSnapshot.bids = cBids
	cSnapshot.asks = cAsks

	// Call C function (unsorted version)
	status := C.fc_orderbook_snapshot_generate_unsorted(
		&cSnapshot,
		cOrders,
		C.size_t(len(orders)),
		C.uint32_t(maxLevels),
		C.fc_orderbook_precision_mode_t(precisionMode),
		C.int64_t(timestamp.UnixNano()),
	)

	if status != C.FC_OK {
		return nil, fmt.Errorf("failed to generate snapshot: status %d", status)
	}

	// Convert C snapshot to Go snapshot
	snapshot := &OrderBookSnapshot{
		Bids:             make([]PriceLevel, int(cSnapshot.num_bid_levels)),
		Asks:             make([]PriceLevel, int(cSnapshot.num_ask_levels)),
		Spread:           float64(cSnapshot.spread),
		MidPrice:         float64(cSnapshot.mid_price),
		WeightedMidPrice: float64(cSnapshot.weighted_mid_price),
		Timestamp:        time.Unix(0, int64(cSnapshot.timestamp_ns)),
		SymbolID:         uint32(cSnapshot.symbol_id),
	}

	cBidsSlice := unsafe.Slice(cBids, int(cSnapshot.num_bid_levels))
	for i := 0; i < int(cSnapshot.num_bid_levels); i++ {
		snapshot.Bids[i] = PriceLevel{
			Price:  float64(cBidsSlice[i].price),
			Volume: float64(cBidsSlice[i].volume),
		}
	}

	cAsksSlice := unsafe.Slice(cAsks, int(cSnapshot.num_ask_levels))
	for i := 0; i < int(cSnapshot.num_ask_levels); i++ {
		snapshot.Asks[i] = PriceLevel{
			Price:  float64(cAsksSlice[i].price),
			Volume: float64(cAsksSlice[i].volume),
		}
	}

	return snapshot, nil
}

// GenerateSnapshotBatch generates snapshots for multiple symbols in batch
//
// Parameters:
//   - orders: Array of orders (can contain multiple symbols)
//   - numSymbols: Number of symbols
//   - maxLevels: Maximum number of levels per side per symbol
//   - precisionMode: Precision mode for volume aggregation
//   - timestamp: Snapshot timestamp
//
// Returns:
//   - Array of OrderBookSnapshot (one per symbol)
//   - error if generation fails
//
// Note: This is more efficient than calling GenerateSnapshot repeatedly.
func GenerateSnapshotBatch(orders []Order, numSymbols uint32, maxLevels uint32, precisionMode OrderBookPrecisionMode, timestamp time.Time) ([]OrderBookSnapshot, error) {
	if numSymbols == 0 {
		return []OrderBookSnapshot{}, nil
	}

	// Allocate C memory for orders
	var cOrders *C.fc_order_t
	if len(orders) > 0 {
		cOrders = (*C.fc_order_t)(C.malloc(C.size_t(len(orders)) * C.size_t(unsafe.Sizeof(C.fc_order_t{}))))
		if cOrders == nil {
			return nil, fmt.Errorf("failed to allocate memory for orders")
		}
		defer C.free(unsafe.Pointer(cOrders))
		
		cOrdersSlice := unsafe.Slice(cOrders, len(orders))
		for i, order := range orders {
			cOrdersSlice[i].symbol_id = C.uint32_t(order.SymbolID)
			cOrdersSlice[i].price = C.double(order.Price)
			cOrdersSlice[i].volume = C.double(order.Volume)
			cOrdersSlice[i].side = C.fc_orderbook_side_t(order.Side)
			cOrdersSlice[i].timestamp_ns = C.int64_t(order.Timestamp.UnixNano())
		}
	}

	// Allocate C memory for snapshots and level arrays
	cSnapshots := (*C.fc_orderbook_snapshot_t)(C.malloc(C.size_t(numSymbols) * C.size_t(unsafe.Sizeof(C.fc_orderbook_snapshot_t{}))))
	if cSnapshots == nil {
		return nil, fmt.Errorf("failed to allocate memory for snapshots")
	}
	defer C.free(unsafe.Pointer(cSnapshots))

	cSnapshotsSlice := unsafe.Slice(cSnapshots, numSymbols)

	// Allocate C memory for bid/ask arrays
	bidArrays := make([]*C.fc_price_level_t, numSymbols)
	askArrays := make([]*C.fc_price_level_t, numSymbols)

	for i := uint32(0); i < numSymbols; i++ {
		bidArrays[i] = (*C.fc_price_level_t)(C.malloc(C.size_t(maxLevels) * C.size_t(unsafe.Sizeof(C.fc_price_level_t{}))))
		if bidArrays[i] == nil {
			// Clean up previously allocated memory
			for j := uint32(0); j < i; j++ {
				C.free(unsafe.Pointer(bidArrays[j]))
				C.free(unsafe.Pointer(askArrays[j]))
			}
			return nil, fmt.Errorf("failed to allocate memory for bid array")
		}

		askArrays[i] = (*C.fc_price_level_t)(C.malloc(C.size_t(maxLevels) * C.size_t(unsafe.Sizeof(C.fc_price_level_t{}))))
		if askArrays[i] == nil {
			// Clean up previously allocated memory
			C.free(unsafe.Pointer(bidArrays[i]))
			for j := uint32(0); j < i; j++ {
				C.free(unsafe.Pointer(bidArrays[j]))
				C.free(unsafe.Pointer(askArrays[j]))
			}
			return nil, fmt.Errorf("failed to allocate memory for ask array")
		}

		cSnapshotsSlice[i].bids = bidArrays[i]
		cSnapshotsSlice[i].asks = askArrays[i]
	}

	// Defer cleanup of all arrays
	defer func() {
		for i := uint32(0); i < numSymbols; i++ {
			C.free(unsafe.Pointer(bidArrays[i]))
			C.free(unsafe.Pointer(askArrays[i]))
		}
	}()

	// Call C function
	var ordersPtr *C.fc_order_t
	if len(orders) > 0 {
		ordersPtr = cOrders
	}

	status := C.fc_orderbook_snapshot_generate_batch(
		cSnapshots,
		ordersPtr,
		C.size_t(len(orders)),
		C.uint32_t(numSymbols),
		C.uint32_t(maxLevels),
		C.fc_orderbook_precision_mode_t(precisionMode),
		C.int64_t(timestamp.UnixNano()),
	)

	if status != C.FC_OK {
		return nil, fmt.Errorf("failed to generate batch snapshots: status %d", status)
	}

	// Convert C snapshots to Go snapshots
	snapshots := make([]OrderBookSnapshot, numSymbols)
	for i := uint32(0); i < numSymbols; i++ {
		snapshots[i] = OrderBookSnapshot{
			Bids:             make([]PriceLevel, int(cSnapshotsSlice[i].num_bid_levels)),
			Asks:             make([]PriceLevel, int(cSnapshotsSlice[i].num_ask_levels)),
			Spread:           float64(cSnapshotsSlice[i].spread),
			MidPrice:         float64(cSnapshotsSlice[i].mid_price),
			WeightedMidPrice: float64(cSnapshotsSlice[i].weighted_mid_price),
			Timestamp:        time.Unix(0, int64(cSnapshotsSlice[i].timestamp_ns)),
			SymbolID:         uint32(cSnapshotsSlice[i].symbol_id),
		}

		bidLevelsSlice := unsafe.Slice(bidArrays[i], int(cSnapshotsSlice[i].num_bid_levels))
		for j := 0; j < int(cSnapshotsSlice[i].num_bid_levels); j++ {
			snapshots[i].Bids[j] = PriceLevel{
				Price:  float64(bidLevelsSlice[j].price),
				Volume: float64(bidLevelsSlice[j].volume),
			}
		}

		askLevelsSlice := unsafe.Slice(askArrays[i], int(cSnapshotsSlice[i].num_ask_levels))
		for j := 0; j < int(cSnapshotsSlice[i].num_ask_levels); j++ {
			snapshots[i].Asks[j] = PriceLevel{
				Price:  float64(askLevelsSlice[j].price),
				Volume: float64(askLevelsSlice[j].volume),
			}
		}
	}

	return snapshots, nil
}

// AggregateLevels aggregates orders at the same price level
//
// Parameters:
//   - orders: Input orders (must be sorted by price)
//   - precisionMode: Precision mode for volume aggregation
//
// Returns:
//   - Array of PriceLevel with aggregated volumes
//   - error if aggregation fails
//
// Note: Input orders must be sorted by price (same direction as output).
func AggregateLevels(orders []Order, precisionMode OrderBookPrecisionMode) ([]PriceLevel, error) {
	if len(orders) == 0 {
		return []PriceLevel{}, nil
	}

	// Allocate C memory for orders
	cOrders := (*C.fc_order_t)(C.malloc(C.size_t(len(orders)) * C.size_t(unsafe.Sizeof(C.fc_order_t{}))))
	if cOrders == nil {
		return nil, fmt.Errorf("failed to allocate memory for orders")
	}
	defer C.free(unsafe.Pointer(cOrders))
	
	cOrdersSlice := unsafe.Slice(cOrders, len(orders))
	for i, order := range orders {
		cOrdersSlice[i].symbol_id = C.uint32_t(order.SymbolID)
		cOrdersSlice[i].price = C.double(order.Price)
		cOrdersSlice[i].volume = C.double(order.Volume)
		cOrdersSlice[i].side = C.fc_orderbook_side_t(order.Side)
		cOrdersSlice[i].timestamp_ns = C.int64_t(order.Timestamp.UnixNano())
	}

	// Allocate C memory for levels
	cLevels := (*C.fc_price_level_t)(C.malloc(C.size_t(len(orders)) * C.size_t(unsafe.Sizeof(C.fc_price_level_t{}))))
	if cLevels == nil {
		return nil, fmt.Errorf("failed to allocate memory for levels")
	}
	defer C.free(unsafe.Pointer(cLevels))
	var numLevels C.uint32_t

	// Call C function
	status := C.fc_orderbook_aggregate_levels(
		cLevels,
		&numLevels,
		cOrders,
		C.size_t(len(orders)),
		C.fc_orderbook_precision_mode_t(precisionMode),
	)

	if status != C.FC_OK {
		return nil, fmt.Errorf("failed to aggregate levels: status %d", status)
	}

	// Convert C levels to Go levels
	levels := make([]PriceLevel, int(numLevels))
	cLevelsSlice := unsafe.Slice(cLevels, int(numLevels))
	for i := 0; i < int(numLevels); i++ {
		levels[i] = PriceLevel{
			Price:  float64(cLevelsSlice[i].price),
			Volume: float64(cLevelsSlice[i].volume),
		}
	}

	return levels, nil
}

// String returns a string representation of the snapshot
func (s *OrderBookSnapshot) String() string {
	return fmt.Sprintf(
		"OrderBookSnapshot{SymbolID: %d, Bids: %d levels, Asks: %d levels, Spread: %.4f, MidPrice: %.4f, WeightedMidPrice: %.4f}",
		s.SymbolID,
		len(s.Bids),
		len(s.Asks),
		s.Spread,
		s.MidPrice,
		s.WeightedMidPrice,
	)
}

// BestBid returns the best bid price level, or nil if no bids
func (s *OrderBookSnapshot) BestBid() *PriceLevel {
	if len(s.Bids) == 0 {
		return nil
	}
	return &s.Bids[0]
}

// BestAsk returns the best ask price level, or nil if no asks
func (s *OrderBookSnapshot) BestAsk() *PriceLevel {
	if len(s.Asks) == 0 {
		return nil
	}
	return &s.Asks[0]
}

// TotalBidVolume returns the total volume across all bid levels
func (s *OrderBookSnapshot) TotalBidVolume() float64 {
	total := 0.0
	for _, level := range s.Bids {
		total += level.Volume
	}
	return total
}

// TotalAskVolume returns the total volume across all ask levels
func (s *OrderBookSnapshot) TotalAskVolume() float64 {
	total := 0.0
	for _, level := range s.Asks {
		total += level.Volume
	}
	return total
}

// Imbalance returns the order book imbalance ratio (bid_volume / (bid_volume + ask_volume))
func (s *OrderBookSnapshot) Imbalance() float64 {
	bidVol := s.TotalBidVolume()
	askVol := s.TotalAskVolume()
	total := bidVol + askVol
	if total < 1e-10 {
		return 0.5
	}
	return bidVol / total
}

// init ensures the C library is initialized
func init() {
	// C library initialization is handled by platform module
	_ = unsafe.Pointer(nil)
}
