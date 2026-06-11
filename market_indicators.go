package exchange

/*
#include "market_indicators.h"
#include "error.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"time"
	"unsafe"
)

// MarketTrade represents a trade used for realtime indicator calculation.
type MarketTrade struct {
	SymbolID   uint32
	Price      float64
	Volume     float64
	BuyVolume  float64
	SellVolume float64
	Timestamp  time.Time
}

// MarketIndicatorKind identifies a market indicator for ranking.
type MarketIndicatorKind int

const (
	MarketIndicatorVWAP MarketIndicatorKind = C.FC_MARKET_INDICATOR_VWAP
	MarketIndicatorTWAP MarketIndicatorKind = C.FC_MARKET_INDICATOR_TWAP
	MarketIndicatorVolatility MarketIndicatorKind = C.FC_MARKET_INDICATOR_VOLATILITY
	MarketIndicatorPressureRatio MarketIndicatorKind = C.FC_MARKET_INDICATOR_PRESSURE_RATIO
)

// MarketIndicatorValues contains current indicator values for one symbol.
type MarketIndicatorValues struct {
	VWAP                 float64
	TWAP                 float64
	Volatility           float64
	BuySellPressureRatio float64
	TradeCount           uint64
	TotalVolume          float64
	TotalAmount          float64
	WindowStart          time.Time
	WindowEnd            time.Time
	Initialized          bool
}

// MarketIndicatorRank contains one ranked indicator value.
type MarketIndicatorRank struct {
	SymbolID uint32
	Value    float64
}

type marketTradeBatchConverter struct {
	buffer    []C.fc_market_trade_t
	bufferCap int
}

func newMarketTradeBatchConverter(initialCap int) *marketTradeBatchConverter {
	if initialCap < 1024 {
		initialCap = 1024
	}
	return &marketTradeBatchConverter{
		buffer:    make([]C.fc_market_trade_t, initialCap),
		bufferCap: initialCap,
	}
}

func (c *marketTradeBatchConverter) convertBatch(trades []MarketTrade) []C.fc_market_trade_t {
	n := len(trades)
	if n == 0 {
		return nil
	}
	if n > c.bufferCap {
		newCap := n
		if newCap < c.bufferCap*2 {
			newCap = c.bufferCap * 2
		}
		c.buffer = make([]C.fc_market_trade_t, newCap)
		c.bufferCap = newCap
	}

	cTrades := c.buffer[:n]
	for i := range trades {
		trade := &trades[i]
		cTrades[i].symbol_id = C.uint32_t(trade.SymbolID)
		cTrades[i].price = C.double(trade.Price)
		cTrades[i].volume = C.double(trade.Volume)
		cTrades[i].buy_volume = C.double(trade.BuyVolume)
		cTrades[i].sell_volume = C.double(trade.SellVolume)
		cTrades[i].timestamp_ns = C.int64_t(trade.Timestamp.UnixNano())
	}
	return cTrades
}

// MarketIndicators calculates realtime market indicators for multiple symbols.
type MarketIndicators struct {
	ctx       *C.fc_market_indicators_ctx_t
	converter *marketTradeBatchConverter
}

// NewMarketIndicators creates a realtime market indicators calculator.
func NewMarketIndicators(numSymbols uint32, window time.Duration, precisionMode PrecisionMode) (*MarketIndicators, error) {
	if numSymbols == 0 {
		return nil, errors.New("numSymbols must be greater than 0")
	}
	if window <= 0 {
		return nil, errors.New("window must be greater than 0")
	}

	ctx := C.fc_market_indicators_create(
		C.uint32_t(numSymbols),
		C.int64_t(window.Nanoseconds()),
		C.fc_market_indicators_precision_mode_t(precisionMode),
	)
	if ctx == nil {
		return nil, errors.New("failed to create market indicators context")
	}

	initialBufferSize := 1024
	if numSymbols > 1024 {
		initialBufferSize = int(numSymbols)
	}
	return &MarketIndicators{
		ctx:       ctx,
		converter: newMarketTradeBatchConverter(initialBufferSize),
	}, nil
}

// Close destroys the calculator and frees resources.
func (m *MarketIndicators) Close() {
	if m.ctx != nil {
		C.fc_market_indicators_destroy(m.ctx)
		m.ctx = nil
	}
}

// Update processes one trade.
func (m *MarketIndicators) Update(trade *MarketTrade) error {
	if m.ctx == nil {
		return errors.New("market indicators context is nil")
	}
	if trade == nil {
		return errors.New("trade is nil")
	}

	cTrade := C.fc_market_trade_t{
		symbol_id:    C.uint32_t(trade.SymbolID),
		price:        C.double(trade.Price),
		volume:       C.double(trade.Volume),
		buy_volume:   C.double(trade.BuyVolume),
		sell_volume:  C.double(trade.SellVolume),
		timestamp_ns: C.int64_t(trade.Timestamp.UnixNano()),
	}
	result := C.fc_market_indicators_update(m.ctx, &cTrade)
	if result != 0 {
		return codeToError(C.int(result))
	}
	return nil
}

// UpdateBatch processes multiple trades in order.
func (m *MarketIndicators) UpdateBatch(trades []MarketTrade) error {
	if m.ctx == nil {
		return errors.New("market indicators context is nil")
	}
	if len(trades) == 0 {
		return nil
	}

	cTrades := m.converter.convertBatch(trades)
	result := C.fc_market_indicators_update_batch(
		m.ctx,
		(*C.fc_market_trade_t)(unsafe.Pointer(&cTrades[0])),
		C.size_t(len(trades)),
	)
	if result != 0 {
		return codeToError(C.int(result))
	}
	return nil
}

// Get retrieves indicator values for one symbol.
func (m *MarketIndicators) Get(symbolID uint32) (*MarketIndicatorValues, error) {
	if m.ctx == nil {
		return nil, errors.New("market indicators context is nil")
	}

	var cValues C.fc_market_indicators_t
	result := C.fc_market_indicators_get(m.ctx, C.uint32_t(symbolID), &cValues)
	if result != 0 {
		return nil, codeToError(C.int(result))
	}
	return cMarketIndicatorsToGo(&cValues), nil
}

// GetAll retrieves indicator values for all symbols.
func (m *MarketIndicators) GetAll() ([]MarketIndicatorValues, error) {
	if m.ctx == nil {
		return nil, errors.New("market indicators context is nil")
	}

	var cNumSymbols C.uint32_t
	result := C.fc_market_indicators_get_stats(m.ctx, &cNumSymbols)
	if result != 0 {
		return nil, codeToError(C.int(result))
	}

	values := make([]C.fc_market_indicators_t, int(cNumSymbols))
	if len(values) == 0 {
		return nil, nil
	}
	result = C.fc_market_indicators_get_all(
		m.ctx,
		(*C.fc_market_indicators_t)(unsafe.Pointer(&values[0])),
	)
	if result != 0 {
		return nil, codeToError(C.int(result))
	}

	out := make([]MarketIndicatorValues, len(values))
	for i := range values {
		out[i] = *cMarketIndicatorsToGo(&values[i])
	}
	return out, nil
}

// Rank ranks symbols by a selected indicator.
func (m *MarketIndicators) Rank(kind MarketIndicatorKind, maxResults int, descending bool) ([]MarketIndicatorRank, error) {
	if m.ctx == nil {
		return nil, errors.New("market indicators context is nil")
	}
	if maxResults <= 0 {
		return nil, errors.New("maxResults must be greater than 0")
	}

	symbolIDs := make([]C.uint32_t, maxResults)
	values := make([]C.double, maxResults)
	var count C.size_t
	desc := C.int(0)
	if descending {
		desc = 1
	}
	result := C.fc_market_indicators_rank(
		m.ctx,
		C.fc_market_indicator_kind_t(kind),
		(*C.uint32_t)(unsafe.Pointer(&symbolIDs[0])),
		(*C.double)(unsafe.Pointer(&values[0])),
		C.size_t(maxResults),
		&count,
		desc,
	)
	if result != 0 {
		return nil, codeToError(C.int(result))
	}

	out := make([]MarketIndicatorRank, int(count))
	for i := range out {
		out[i].SymbolID = uint32(symbolIDs[i])
		out[i].Value = float64(values[i])
	}
	return out, nil
}

// ResetSymbol clears indicator state for one symbol.
func (m *MarketIndicators) ResetSymbol(symbolID uint32) error {
	if m.ctx == nil {
		return errors.New("market indicators context is nil")
	}
	result := C.fc_market_indicators_reset_symbol(m.ctx, C.uint32_t(symbolID))
	if result != 0 {
		return codeToError(C.int(result))
	}
	return nil
}

// ResetAll clears all indicator state.
func (m *MarketIndicators) ResetAll() error {
	if m.ctx == nil {
		return errors.New("market indicators context is nil")
	}
	result := C.fc_market_indicators_reset_all(m.ctx)
	if result != 0 {
		return codeToError(C.int(result))
	}
	return nil
}

func cMarketIndicatorsToGo(values *C.fc_market_indicators_t) *MarketIndicatorValues {
	return &MarketIndicatorValues{
		VWAP:                 float64(values.vwap),
		TWAP:                 float64(values.twap),
		Volatility:           float64(values.volatility),
		BuySellPressureRatio: float64(values.buy_sell_pressure_ratio),
		TradeCount:           uint64(values.trade_count),
		TotalVolume:          float64(values.total_volume),
		TotalAmount:          float64(values.total_amount),
		WindowStart:          time.Unix(0, int64(values.window_start_ns)),
		WindowEnd:            time.Unix(0, int64(values.window_end_ns)),
		Initialized:          values.initialized != 0,
	}
}
