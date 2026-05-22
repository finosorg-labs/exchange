package exchange

/*
#include "ticker.h"
#include "error.h"
#include "platform.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"time"
	"unsafe"
)

// Tick represents a single trade event
type Tick struct {
	SymbolID  uint32
	Price     float64
	Volume    float64
	Amount    float64
	Timestamp time.Time
}

// OHLCV represents aggregated OHLCV data for a time period
type OHLCV struct {
	Open        float64
	High        float64
	Low         float64
	Close       float64
	Volume      float64
	Amount      float64
	PeriodStart time.Time
	PeriodEnd   time.Time
	TickCount   uint32
	Initialized bool
}

// PrecisionMode defines the precision mode for volume/amount accumulation
type PrecisionMode int

const (
	// PrecisionKahan uses Kahan summation (recommended, ~10% overhead)
	PrecisionKahan PrecisionMode = C.FC_TICKER_PRECISION_KAHAN
	// PrecisionStandard uses standard floating-point addition
	PrecisionStandard PrecisionMode = C.FC_TICKER_PRECISION_STANDARD
	// PrecisionBigfloat uses arbitrary precision (extreme precision, ~2000% overhead)
	PrecisionBigfloat PrecisionMode = C.FC_TICKER_PRECISION_BIGFLOAT
)

// Ticker aggregates tick data into OHLCV format
type Ticker struct {
	ctx *C.fc_ticker_ctx_t
}

// NewTicker creates a new ticker aggregator
//
// Parameters:
//   - numSymbols: Number of symbols to track
//   - periods: Array of period durations
//   - precisionMode: Precision mode for accumulation
//
// Returns the ticker instance or an error
func NewTicker(numSymbols uint32, periods []time.Duration, precisionMode PrecisionMode) (*Ticker, error) {
	if numSymbols == 0 {
		return nil, errors.New("numSymbols must be greater than 0")
	}
	if len(periods) == 0 {
		return nil, errors.New("periods must not be empty")
	}

	numPeriods := C.uint32_t(len(periods))
	periodDurationsNs := make([]C.int64_t, len(periods))
	for i, period := range periods {
		periodDurationsNs[i] = C.int64_t(period.Nanoseconds())
	}

	ctx := C.fc_ticker_create(
		C.uint32_t(numSymbols),
		numPeriods,
		(*C.int64_t)(unsafe.Pointer(&periodDurationsNs[0])),
		C.fc_ticker_precision_mode_t(precisionMode),
	)

	if ctx == nil {
		return nil, errors.New("failed to create ticker context")
	}

	return &Ticker{ctx: ctx}, nil
}

// Close destroys the ticker and frees resources
func (t *Ticker) Close() {
	if t.ctx != nil {
		C.fc_ticker_destroy(t.ctx)
		t.ctx = nil
	}
}

// Update processes a single tick
func (t *Ticker) Update(tick *Tick) error {
	if t.ctx == nil {
		return errors.New("ticker context is nil")
	}

	cTick := C.fc_tick_t{
		symbol_id:    C.uint32_t(tick.SymbolID),
		price:        C.double(tick.Price),
		volume:       C.double(tick.Volume),
		amount:       C.double(tick.Amount),
		timestamp_ns: C.int64_t(tick.Timestamp.UnixNano()),
	}

	result := C.fc_ticker_update(t.ctx, &cTick)
	if result != 0 {
		return codeToError(result)
	}

	return nil
}

// UpdateBatch processes multiple ticks in a batch
// Returns error on first validation failure; all ticks before the error are applied
func (t *Ticker) UpdateBatch(ticks []Tick) error {
	if t.ctx == nil {
		return errors.New("ticker context is nil")
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

	result := C.fc_ticker_update_batch(
		t.ctx,
		(*C.fc_tick_t)(unsafe.Pointer(&cTicks[0])),
		C.size_t(len(ticks)),
	)

	if result != 0 {
		return codeToError(C.int(result))
	}

	return nil
}

// GetOHLCV retrieves OHLCV data for a specific symbol and period
func (t *Ticker) GetOHLCV(symbolID uint32, periodIdx uint32) (*OHLCV, error) {
	if t.ctx == nil {
		return nil, errors.New("ticker context is nil")
	}

	var cOHLCV C.fc_ohlcv_t
	result := C.fc_ticker_get_ohlcv(
		t.ctx,
		C.uint32_t(symbolID),
		C.uint32_t(periodIdx),
		&cOHLCV,
	)

	if result != 0 {
		return nil, codeToError(result)
	}

	return cOHLCVToGo(&cOHLCV), nil
}

// GetSymbolOHLCV retrieves all OHLCV data for a specific symbol (all periods)
func (t *Ticker) GetSymbolOHLCV(symbolID uint32) ([]OHLCV, error) {
	if t.ctx == nil {
		return nil, errors.New("ticker context is nil")
	}

	var numSymbols, numPeriods C.uint32_t
	result := C.fc_ticker_get_stats(t.ctx, &numSymbols, &numPeriods)
	if result != 0 {
		return nil, codeToError(result)
	}

	cOHLCVArray := make([]C.fc_ohlcv_t, int(numPeriods))
	result = C.fc_ticker_get_symbol_ohlcv(
		t.ctx,
		C.uint32_t(symbolID),
		(*C.fc_ohlcv_t)(unsafe.Pointer(&cOHLCVArray[0])),
	)

	if result != 0 {
		return nil, codeToError(result)
	}

	ohlcvArray := make([]OHLCV, int(numPeriods))
	for i := range ohlcvArray {
		ohlcvArray[i] = *cOHLCVToGo(&cOHLCVArray[i])
	}

	return ohlcvArray, nil
}

// ResetOHLCV resets OHLCV data for a specific symbol and period
func (t *Ticker) ResetOHLCV(symbolID uint32, periodIdx uint32) error {
	if t.ctx == nil {
		return errors.New("ticker context is nil")
	}

	result := C.fc_ticker_reset_ohlcv(
		t.ctx,
		C.uint32_t(symbolID),
		C.uint32_t(periodIdx),
	)

	if result != 0 {
		return codeToError(result)
	}

	return nil
}

// ResetAll resets all OHLCV data
func (t *Ticker) ResetAll() error {
	if t.ctx == nil {
		return errors.New("ticker context is nil")
	}

	result := C.fc_ticker_reset_all(t.ctx)
	if result != 0 {
		return codeToError(result)
	}

	return nil
}

// GetStats returns the number of symbols and periods
func (t *Ticker) GetStats() (numSymbols uint32, numPeriods uint32, err error) {
	if t.ctx == nil {
		return 0, 0, errors.New("ticker context is nil")
	}

	var cNumSymbols, cNumPeriods C.uint32_t
	result := C.fc_ticker_get_stats(t.ctx, &cNumSymbols, &cNumPeriods)
	if result != 0 {
		return 0, 0, codeToError(result)
	}

	return uint32(cNumSymbols), uint32(cNumPeriods), nil
}

// Helper function to convert C OHLCV to Go OHLCV
func cOHLCVToGo(cOHLCV *C.fc_ohlcv_t) *OHLCV {
	return &OHLCV{
		Open:        float64(cOHLCV.open),
		High:        float64(cOHLCV.high),
		Low:         float64(cOHLCV.low),
		Close:       float64(cOHLCV.close),
		Volume:      float64(cOHLCV.volume),
		Amount:      float64(cOHLCV.amount),
		PeriodStart: time.Unix(0, int64(cOHLCV.period_start_ns)),
		PeriodEnd:   time.Unix(0, int64(cOHLCV.period_end_ns)),
		TickCount:   uint32(cOHLCV.tick_count),
		Initialized: cOHLCV.initialized != 0,
	}
}

// Helper function to convert C error code to Go error
func codeToError(code C.int) error {
	switch code {
	case C.FC_ERR_INVALID_ARG:
		return errors.New("invalid argument")
	default:
		return errors.New("unknown error")
	}
}
