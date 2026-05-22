package exchange

/*
#include "ticker_merge.h"
#include "error.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"time"
	"unsafe"
)

// TickerMergeCallback is called when a derived period K-line is completed
type TickerMergeCallback func(symbolID uint32, derivedPeriodIdx uint32, ohlcv *OHLCV)

// TickerMerge aggregates tick data with multi-period K-line merging
type TickerMerge struct {
	ctx      *C.fc_ticker_merge_ctx_t
	callback TickerMergeCallback
}

// NewTickerMerge creates a new ticker merge aggregator
//
// Parameters:
//   - numSymbols: Number of symbols to track
//   - basePeriod: Base period duration (e.g., 1 minute)
//   - derivedPeriods: Array of derived period durations (must be multiples of basePeriod)
//   - precisionMode: Precision mode for accumulation
//   - callback: Optional callback for derived period completion (can be nil)
//
// Returns the ticker merge instance or an error
func NewTickerMerge(
	numSymbols uint32,
	basePeriod time.Duration,
	derivedPeriods []time.Duration,
	precisionMode PrecisionMode,
	callback TickerMergeCallback,
) (*TickerMerge, error) {
	if numSymbols == 0 {
		return nil, errors.New("numSymbols must be greater than 0")
	}
	if basePeriod <= 0 {
		return nil, errors.New("basePeriod must be positive")
	}

	basePeriodNs := C.int64_t(basePeriod.Nanoseconds())

	var derivedPeriodsNs []C.int64_t
	var derivedPeriodsPtr *C.int64_t
	numDerivedPeriods := C.uint32_t(0)

	if len(derivedPeriods) > 0 {
		derivedPeriodsNs = make([]C.int64_t, len(derivedPeriods))
		for i, period := range derivedPeriods {
			if period <= 0 {
				return nil, errors.New("derived periods must be positive")
			}
			if period%basePeriod != 0 {
				return nil, errors.New("derived periods must be multiples of base period")
			}
			derivedPeriodsNs[i] = C.int64_t(period.Nanoseconds())
		}
		derivedPeriodsPtr = (*C.int64_t)(unsafe.Pointer(&derivedPeriodsNs[0]))
		numDerivedPeriods = C.uint32_t(len(derivedPeriods))
	}

	tm := &TickerMerge{
		callback: callback,
	}

	ctx := C.fc_ticker_merge_create(
		C.uint32_t(numSymbols),
		basePeriodNs,
		derivedPeriodsPtr,
		numDerivedPeriods,
		C.fc_ticker_precision_mode_t(precisionMode),
		nil,
		nil,
	)

	if ctx == nil {
		return nil, errors.New("failed to create ticker merge context")
	}

	tm.ctx = ctx
	return tm, nil
}

// Close destroys the ticker merge and frees resources
func (tm *TickerMerge) Close() {
	if tm.ctx != nil {
		C.fc_ticker_merge_destroy(tm.ctx)
		tm.ctx = nil
	}
}

// Update processes a single tick
func (tm *TickerMerge) Update(tick *Tick) error {
	if tm.ctx == nil {
		return errors.New("ticker merge context is nil")
	}

	cTick := C.fc_tick_t{
		symbol_id:    C.uint32_t(tick.SymbolID),
		price:        C.double(tick.Price),
		volume:       C.double(tick.Volume),
		amount:       C.double(tick.Amount),
		timestamp_ns: C.int64_t(tick.Timestamp.UnixNano()),
	}

	result := C.fc_ticker_merge_update(tm.ctx, &cTick)
	if result != 0 {
		return codeToError(result)
	}

	return nil
}

// UpdateBatch processes multiple ticks in a batch
func (tm *TickerMerge) UpdateBatch(ticks []Tick) error {
	if tm.ctx == nil {
		return errors.New("ticker merge context is nil")
	}
	if len(ticks) == 0 {
		return nil
	}

	cTicks := make([]C.fc_tick_t, len(ticks))
	for i, tick := range ticks {
		cTicks[i] = C.fc_tick_t{
			symbol_id:    C.uint32_t(tick.SymbolID),
			price:        C.double(tick.Price),
			volume:       C.double(tick.Volume),
			amount:       C.double(tick.Amount),
			timestamp_ns: C.int64_t(tick.Timestamp.UnixNano()),
		}
	}

	result := C.fc_ticker_merge_update_batch(
		tm.ctx,
		(*C.fc_tick_t)(unsafe.Pointer(&cTicks[0])),
		C.size_t(len(ticks)),
	)

	if result != 0 {
		return codeToError(result)
	}

	return nil
}

// GetBaseOHLCV retrieves base period OHLCV data for a specific symbol
func (tm *TickerMerge) GetBaseOHLCV(symbolID uint32) (*OHLCV, error) {
	if tm.ctx == nil {
		return nil, errors.New("ticker merge context is nil")
	}

	var cOHLCV C.fc_ohlcv_t
	result := C.fc_ticker_merge_get_base_ohlcv(
		tm.ctx,
		C.uint32_t(symbolID),
		&cOHLCV,
	)

	if result != 0 {
		return nil, codeToError(result)
	}

	return cOHLCVToGo(&cOHLCV), nil
}

// GetDerivedOHLCV retrieves derived period OHLCV data for a specific symbol
func (tm *TickerMerge) GetDerivedOHLCV(symbolID uint32, derivedPeriodIdx uint32) (*OHLCV, error) {
	if tm.ctx == nil {
		return nil, errors.New("ticker merge context is nil")
	}

	var cOHLCV C.fc_ohlcv_t
	result := C.fc_ticker_merge_get_derived_ohlcv(
		tm.ctx,
		C.uint32_t(symbolID),
		C.uint32_t(derivedPeriodIdx),
		&cOHLCV,
	)

	if result != 0 {
		return nil, codeToError(result)
	}

	return cOHLCVToGo(&cOHLCV), nil
}

// GetDerivedHistory retrieves the last N completed K-lines for a derived period
func (tm *TickerMerge) GetDerivedHistory(symbolID uint32, derivedPeriodIdx uint32, count int) ([]OHLCV, error) {
	if tm.ctx == nil {
		return nil, errors.New("ticker merge context is nil")
	}
	if count <= 0 {
		return nil, nil
	}

	cOHLCVArray := make([]C.fc_ohlcv_t, count)
	retrieved := C.fc_ticker_merge_get_derived_history(
		tm.ctx,
		C.uint32_t(symbolID),
		C.uint32_t(derivedPeriodIdx),
		(*C.fc_ohlcv_t)(unsafe.Pointer(&cOHLCVArray[0])),
		C.size_t(count),
	)

	if retrieved == 0 {
		return nil, nil
	}

	ohlcvArray := make([]OHLCV, int(retrieved))
	for i := 0; i < int(retrieved); i++ {
		ohlcvArray[i] = *cOHLCVToGo(&cOHLCVArray[i])
	}

	return ohlcvArray, nil
}

// Reset resets all K-lines for a specific symbol
func (tm *TickerMerge) Reset(symbolID uint32) error {
	if tm.ctx == nil {
		return errors.New("ticker merge context is nil")
	}

	result := C.fc_ticker_merge_reset(tm.ctx, C.uint32_t(symbolID))
	if result != 0 {
		return codeToError(result)
	}

	return nil
}

// ResetAll resets all K-lines for all symbols
func (tm *TickerMerge) ResetAll() error {
	if tm.ctx == nil {
		return errors.New("ticker merge context is nil")
	}

	result := C.fc_ticker_merge_reset_all(tm.ctx)
	if result != 0 {
		return codeToError(result)
	}

	return nil
}

// GetStats returns the number of symbols, base period, and number of derived periods
func (tm *TickerMerge) GetStats() (numSymbols uint32, basePeriod time.Duration, numDerivedPeriods uint32, err error) {
	if tm.ctx == nil {
		return 0, 0, 0, errors.New("ticker merge context is nil")
	}

	var cNumSymbols, cNumDerivedPeriods C.uint32_t
	var cBasePeriodNs C.int64_t
	result := C.fc_ticker_merge_get_stats(tm.ctx, &cNumSymbols, &cBasePeriodNs, &cNumDerivedPeriods)
	if result != 0 {
		return 0, 0, 0, codeToError(result)
	}

	return uint32(cNumSymbols), time.Duration(cBasePeriodNs), uint32(cNumDerivedPeriods), nil
}
