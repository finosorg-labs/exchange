package exchange

import (
	"math"
	"testing"
	"time"
)

func marketTrade(symbol uint32, price, volume, buy, sell float64, ts time.Time) MarketTrade {
	return MarketTrade{
		SymbolID:   symbol,
		Price:      price,
		Volume:     volume,
		BuyVolume:  buy,
		SellVolume: sell,
		Timestamp:  ts,
	}
}

func assertClose(t *testing.T, got, want float64) {
	t.Helper()
	if math.Abs(got-want) > 1e-9 {
		t.Fatalf("got %v, want %v", got, want)
	}
}

func TestNewMarketIndicators(t *testing.T) {
	m, err := NewMarketIndicators(10, time.Minute, PrecisionKahan)
	if err != nil {
		t.Fatalf("NewMarketIndicators failed: %v", err)
	}
	m.Close()
	m.Close()

	if _, err := NewMarketIndicators(0, time.Minute, PrecisionKahan); err == nil {
		t.Fatal("expected error for zero symbols")
	}
	if _, err := NewMarketIndicators(10, 0, PrecisionKahan); err == nil {
		t.Fatal("expected error for zero window")
	}
}

func TestMarketIndicatorsUpdate(t *testing.T) {
	m, err := NewMarketIndicators(2, time.Minute, PrecisionKahan)
	if err != nil {
		t.Fatal(err)
	}
	defer m.Close()

	ts := time.Unix(0, int64(time.Second))
	if err := m.Update(&MarketTrade{SymbolID: 0, Price: 100, Volume: 10, BuyVolume: 6, SellVolume: 4, Timestamp: ts}); err != nil {
		t.Fatalf("Update failed: %v", err)
	}

	values, err := m.Get(0)
	if err != nil {
		t.Fatalf("Get failed: %v", err)
	}
	if !values.Initialized {
		t.Fatal("expected initialized values")
	}
	assertClose(t, values.VWAP, 100)
	assertClose(t, values.TWAP, 100)
	assertClose(t, values.Volatility, 0)
	assertClose(t, values.BuySellPressureRatio, 1.5)
	if values.TradeCount != 1 {
		t.Fatalf("got trade count %d, want 1", values.TradeCount)
	}
}

func TestMarketIndicatorsUpdateBatch(t *testing.T) {
	m, err := NewMarketIndicators(1, time.Minute, PrecisionKahan)
	if err != nil {
		t.Fatal(err)
	}
	defer m.Close()

	base := time.Unix(0, 0)
	trades := []MarketTrade{
		marketTrade(0, 100, 10, 5, 5, base),
		marketTrade(0, 110, 20, 10, 10, base.Add(10*time.Second)),
		marketTrade(0, 121, 10, 10, 0, base.Add(30*time.Second)),
	}
	if err := m.UpdateBatch(trades); err != nil {
		t.Fatalf("UpdateBatch failed: %v", err)
	}

	values, err := m.Get(0)
	if err != nil {
		t.Fatal(err)
	}
	assertClose(t, values.VWAP, 110.25)
	assertClose(t, values.TWAP, (100*10+110*20)/30.0)
	assertClose(t, values.BuySellPressureRatio, 25.0/15.0)
}

func TestMarketIndicatorsMultipleSymbolsAndGetAll(t *testing.T) {
	m, err := NewMarketIndicators(3, time.Minute, PrecisionStandard)
	if err != nil {
		t.Fatal(err)
	}
	defer m.Close()

	ts := time.Unix(0, int64(time.Second))
	trades := []MarketTrade{
		marketTrade(0, 100, 10, 5, 5, ts),
		marketTrade(1, 200, 5, 2, 3, ts),
	}
	if err := m.UpdateBatch(trades); err != nil {
		t.Fatal(err)
	}

	all, err := m.GetAll()
	if err != nil {
		t.Fatal(err)
	}
	if len(all) != 3 {
		t.Fatalf("got %d values, want 3", len(all))
	}
	assertClose(t, all[0].VWAP, 100)
	assertClose(t, all[1].VWAP, 200)
	if all[2].Initialized {
		t.Fatal("expected third symbol to be uninitialized")
	}
}

func TestMarketIndicatorsRank(t *testing.T) {
	m, err := NewMarketIndicators(4, time.Minute, PrecisionKahan)
	if err != nil {
		t.Fatal(err)
	}
	defer m.Close()

	ts := time.Unix(0, int64(time.Second))
	trades := []MarketTrade{
		marketTrade(0, 100, 10, 5, 5, ts),
		marketTrade(1, 200, 10, 5, 5, ts),
		marketTrade(2, 150, 10, 5, 5, ts),
	}
	if err := m.UpdateBatch(trades); err != nil {
		t.Fatal(err)
	}

	ranks, err := m.Rank(MarketIndicatorVWAP, 3, true)
	if err != nil {
		t.Fatal(err)
	}
	if len(ranks) != 3 {
		t.Fatalf("got %d ranks, want 3", len(ranks))
	}
	if ranks[0].SymbolID != 1 || ranks[1].SymbolID != 2 || ranks[2].SymbolID != 0 {
		t.Fatalf("unexpected ranks: %+v", ranks)
	}
}

func TestMarketIndicatorsValidationAndReset(t *testing.T) {
	m, err := NewMarketIndicators(1, time.Minute, PrecisionKahan)
	if err != nil {
		t.Fatal(err)
	}
	defer m.Close()

	if err := m.Update(nil); err == nil {
		t.Fatal("expected nil trade error")
	}
	if err := m.Update(&MarketTrade{SymbolID: 0, Price: math.NaN(), Volume: 1, Timestamp: time.Unix(0, 0)}); err == nil {
		t.Fatal("expected NaN error")
	}

	valid := marketTrade(0, 100, 10, 5, 5, time.Unix(0, int64(time.Second)))
	if err := m.Update(&valid); err != nil {
		t.Fatal(err)
	}
	if err := m.ResetSymbol(0); err != nil {
		t.Fatal(err)
	}
	values, err := m.Get(0)
	if err != nil {
		t.Fatal(err)
	}
	if values.Initialized {
		t.Fatal("expected reset symbol to be uninitialized")
	}

	if err := m.ResetAll(); err != nil {
		t.Fatal(err)
	}
}

func TestMarketIndicatorsClosed(t *testing.T) {
	m, err := NewMarketIndicators(1, time.Minute, PrecisionKahan)
	if err != nil {
		t.Fatal(err)
	}
	m.Close()

	if err := m.Update(&MarketTrade{}); err == nil {
		t.Fatal("expected update after close to fail")
	}
	if _, err := m.Get(0); err == nil {
		t.Fatal("expected get after close to fail")
	}
}
