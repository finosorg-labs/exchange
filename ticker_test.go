package exchange

import (
	"math"
	"testing"
	"time"
)

const epsilon = 1e-10

func TestTickerCreateDestroy(t *testing.T) {
	periods := []time.Duration{time.Minute, 5 * time.Minute}
	ticker, err := NewTicker(100, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	numSymbols, numPeriods, err := ticker.GetStats()
	if err != nil {
		t.Fatalf("Failed to get stats: %v", err)
	}

	if numSymbols != 100 {
		t.Errorf("Expected numSymbols=100, got %d", numSymbols)
	}
	if numPeriods != 2 {
		t.Errorf("Expected numPeriods=2, got %d", numPeriods)
	}
}

func TestTickerCreateInvalidArgs(t *testing.T) {
	periods := []time.Duration{time.Minute}

	_, err := NewTicker(0, periods, PrecisionKahan)
	if err == nil {
		t.Error("Expected error with numSymbols=0")
	}

	_, err = NewTicker(100, []time.Duration{}, PrecisionKahan)
	if err == nil {
		t.Error("Expected error with empty periods")
	}
}

func TestTickerSingleTickUpdate(t *testing.T) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(10, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	tick := &Tick{
		SymbolID:  0,
		Price:     100.5,
		Volume:    1000.0,
		Amount:    100500.0,
		Timestamp: time.Unix(0, 1000000000),
	}

	err = ticker.Update(tick)
	if err != nil {
		t.Fatalf("Failed to update tick: %v", err)
	}

	ohlcv, err := ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV: %v", err)
	}

	if !ohlcv.Initialized {
		t.Error("OHLCV not initialized")
	}
	if math.Abs(ohlcv.Open-100.5) > epsilon {
		t.Errorf("Expected open=100.5, got %f", ohlcv.Open)
	}
	if math.Abs(ohlcv.High-100.5) > epsilon {
		t.Errorf("Expected high=100.5, got %f", ohlcv.High)
	}
	if math.Abs(ohlcv.Low-100.5) > epsilon {
		t.Errorf("Expected low=100.5, got %f", ohlcv.Low)
	}
	if math.Abs(ohlcv.Close-100.5) > epsilon {
		t.Errorf("Expected close=100.5, got %f", ohlcv.Close)
	}
	if math.Abs(ohlcv.Volume-1000.0) > epsilon {
		t.Errorf("Expected volume=1000.0, got %f", ohlcv.Volume)
	}
	if math.Abs(ohlcv.Amount-100500.0) > epsilon {
		t.Errorf("Expected amount=100500.0, got %f", ohlcv.Amount)
	}
	if ohlcv.TickCount != 1 {
		t.Errorf("Expected tick_count=1, got %d", ohlcv.TickCount)
	}
}

func TestTickerMultipleTicksSamePeriod(t *testing.T) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(10, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	ticks := []Tick{
		{0, 100.0, 100.0, 10000.0, time.Unix(0, 1000000000)},
		{0, 105.0, 200.0, 21000.0, time.Unix(0, 2000000000)},
		{0, 98.0, 150.0, 14700.0, time.Unix(0, 3000000000)},
		{0, 102.0, 300.0, 30600.0, time.Unix(0, 4000000000)},
	}

	for i := range ticks {
		err = ticker.Update(&ticks[i])
		if err != nil {
			t.Fatalf("Failed to update tick %d: %v", i, err)
		}
	}

	ohlcv, err := ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV: %v", err)
	}

	if math.Abs(ohlcv.Open-100.0) > epsilon {
		t.Errorf("Expected open=100.0, got %f", ohlcv.Open)
	}
	if math.Abs(ohlcv.High-105.0) > epsilon {
		t.Errorf("Expected high=105.0, got %f", ohlcv.High)
	}
	if math.Abs(ohlcv.Low-98.0) > epsilon {
		t.Errorf("Expected low=98.0, got %f", ohlcv.Low)
	}
	if math.Abs(ohlcv.Close-102.0) > epsilon {
		t.Errorf("Expected close=102.0, got %f", ohlcv.Close)
	}
	if math.Abs(ohlcv.Volume-750.0) > epsilon {
		t.Errorf("Expected volume=750.0, got %f", ohlcv.Volume)
	}
	if math.Abs(ohlcv.Amount-76300.0) > epsilon {
		t.Errorf("Expected amount=76300.0, got %f", ohlcv.Amount)
	}
	if ohlcv.TickCount != 4 {
		t.Errorf("Expected tick_count=4, got %d", ohlcv.TickCount)
	}
}

func TestTickerPeriodRollover(t *testing.T) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(10, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	tick1 := &Tick{0, 100.0, 100.0, 10000.0, time.Unix(0, 1000000000)}
	tick2 := &Tick{0, 105.0, 200.0, 21000.0, time.Unix(0, 61000000000)}

	err = ticker.Update(tick1)
	if err != nil {
		t.Fatalf("Failed to update tick1: %v", err)
	}

	ohlcv, err := ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV: %v", err)
	}
	if math.Abs(ohlcv.Open-100.0) > epsilon {
		t.Errorf("Expected open=100.0 for period 1, got %f", ohlcv.Open)
	}
	if ohlcv.TickCount != 1 {
		t.Errorf("Expected tick_count=1 for period 1, got %d", ohlcv.TickCount)
	}

	err = ticker.Update(tick2)
	if err != nil {
		t.Fatalf("Failed to update tick2: %v", err)
	}

	ohlcv, err = ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV: %v", err)
	}
	if math.Abs(ohlcv.Open-105.0) > epsilon {
		t.Errorf("Expected open=105.0 for period 2, got %f", ohlcv.Open)
	}
	if math.Abs(ohlcv.Close-105.0) > epsilon {
		t.Errorf("Expected close=105.0 for period 2, got %f", ohlcv.Close)
	}
	if ohlcv.TickCount != 1 {
		t.Errorf("Expected tick_count=1 for period 2, got %d", ohlcv.TickCount)
	}
}

func TestTickerMultipleSymbols(t *testing.T) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(3, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	ticks := []Tick{
		{0, 100.0, 100.0, 10000.0, time.Unix(0, 1000000000)},
		{1, 200.0, 200.0, 40000.0, time.Unix(0, 1000000000)},
		{2, 300.0, 300.0, 90000.0, time.Unix(0, 1000000000)},
		{0, 105.0, 150.0, 15750.0, time.Unix(0, 2000000000)},
	}

	for i := range ticks {
		err = ticker.Update(&ticks[i])
		if err != nil {
			t.Fatalf("Failed to update tick %d: %v", i, err)
		}
	}

	ohlcv0, err := ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV for symbol 0: %v", err)
	}
	if math.Abs(ohlcv0.Open-100.0) > epsilon {
		t.Errorf("Expected open=100.0 for symbol 0, got %f", ohlcv0.Open)
	}
	if math.Abs(ohlcv0.Close-105.0) > epsilon {
		t.Errorf("Expected close=105.0 for symbol 0, got %f", ohlcv0.Close)
	}
	if ohlcv0.TickCount != 2 {
		t.Errorf("Expected tick_count=2 for symbol 0, got %d", ohlcv0.TickCount)
	}

	ohlcv1, err := ticker.GetOHLCV(1, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV for symbol 1: %v", err)
	}
	if math.Abs(ohlcv1.Open-200.0) > epsilon {
		t.Errorf("Expected open=200.0 for symbol 1, got %f", ohlcv1.Open)
	}
	if ohlcv1.TickCount != 1 {
		t.Errorf("Expected tick_count=1 for symbol 1, got %d", ohlcv1.TickCount)
	}

	ohlcv2, err := ticker.GetOHLCV(2, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV for symbol 2: %v", err)
	}
	if math.Abs(ohlcv2.Open-300.0) > epsilon {
		t.Errorf("Expected open=300.0 for symbol 2, got %f", ohlcv2.Open)
	}
	if ohlcv2.TickCount != 1 {
		t.Errorf("Expected tick_count=1 for symbol 2, got %d", ohlcv2.TickCount)
	}
}

func TestTickerMultiplePeriods(t *testing.T) {
	periods := []time.Duration{time.Minute, 5 * time.Minute}
	ticker, err := NewTicker(1, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	ticks := []Tick{
		{0, 100.0, 100.0, 10000.0, time.Unix(0, 1000000000)},
		{0, 105.0, 200.0, 21000.0, time.Unix(0, 61000000000)},
		{0, 110.0, 150.0, 16500.0, time.Unix(0, 121000000000)},
	}

	for i := range ticks {
		err = ticker.Update(&ticks[i])
		if err != nil {
			t.Fatalf("Failed to update tick %d: %v", i, err)
		}
	}

	ohlcv1m, err := ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV for 1m period: %v", err)
	}
	if math.Abs(ohlcv1m.Open-110.0) > epsilon {
		t.Errorf("Expected open=110.0 for 1m period, got %f", ohlcv1m.Open)
	}
	if ohlcv1m.TickCount != 1 {
		t.Errorf("Expected tick_count=1 for 1m period, got %d", ohlcv1m.TickCount)
	}

	ohlcv5m, err := ticker.GetOHLCV(0, 1)
	if err != nil {
		t.Fatalf("Failed to get OHLCV for 5m period: %v", err)
	}
	if math.Abs(ohlcv5m.Open-100.0) > epsilon {
		t.Errorf("Expected open=100.0 for 5m period, got %f", ohlcv5m.Open)
	}
	if math.Abs(ohlcv5m.High-110.0) > epsilon {
		t.Errorf("Expected high=110.0 for 5m period, got %f", ohlcv5m.High)
	}
	if math.Abs(ohlcv5m.Low-100.0) > epsilon {
		t.Errorf("Expected low=100.0 for 5m period, got %f", ohlcv5m.Low)
	}
	if math.Abs(ohlcv5m.Close-110.0) > epsilon {
		t.Errorf("Expected close=110.0 for 5m period, got %f", ohlcv5m.Close)
	}
	if ohlcv5m.TickCount != 3 {
		t.Errorf("Expected tick_count=3 for 5m period, got %d", ohlcv5m.TickCount)
	}
}

func TestTickerBatchUpdate(t *testing.T) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(2, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	ticks := []Tick{
		{0, 100.0, 100.0, 10000.0, time.Unix(0, 1000000000)},
		{1, 200.0, 200.0, 40000.0, time.Unix(0, 1000000000)},
		{0, 105.0, 150.0, 15750.0, time.Unix(0, 2000000000)},
		{1, 205.0, 250.0, 51250.0, time.Unix(0, 2000000000)},
	}

	err = ticker.UpdateBatch(ticks)
	if err != nil {
		t.Fatalf("Failed to update batch: %v", err)
	}

	ohlcv0, err := ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV for symbol 0: %v", err)
	}
	if ohlcv0.TickCount != 2 {
		t.Errorf("Expected tick_count=2 for symbol 0, got %d", ohlcv0.TickCount)
	}

	ohlcv1, err := ticker.GetOHLCV(1, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV for symbol 1: %v", err)
	}
	if ohlcv1.TickCount != 2 {
		t.Errorf("Expected tick_count=2 for symbol 1, got %d", ohlcv1.TickCount)
	}
}

func TestTickerGetSymbolOHLCV(t *testing.T) {
	periods := []time.Duration{time.Minute, 5 * time.Minute}
	ticker, err := NewTicker(1, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	tick := &Tick{0, 100.0, 100.0, 10000.0, time.Unix(0, 1000000000)}
	err = ticker.Update(tick)
	if err != nil {
		t.Fatalf("Failed to update tick: %v", err)
	}

	ohlcvArray, err := ticker.GetSymbolOHLCV(0)
	if err != nil {
		t.Fatalf("Failed to get symbol OHLCV: %v", err)
	}

	if len(ohlcvArray) != 2 {
		t.Errorf("Expected 2 periods, got %d", len(ohlcvArray))
	}
	if !ohlcvArray[0].Initialized {
		t.Error("Period 0 should be initialized")
	}
	if !ohlcvArray[1].Initialized {
		t.Error("Period 1 should be initialized")
	}
}

func TestTickerReset(t *testing.T) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(2, periods, PrecisionKahan)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	tick := &Tick{0, 100.0, 100.0, 10000.0, time.Unix(0, 1000000000)}
	err = ticker.Update(tick)
	if err != nil {
		t.Fatalf("Failed to update tick: %v", err)
	}

	ohlcv, err := ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV: %v", err)
	}
	if !ohlcv.Initialized {
		t.Error("OHLCV should be initialized")
	}

	err = ticker.ResetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to reset OHLCV: %v", err)
	}

	ohlcv, err = ticker.GetOHLCV(0, 0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV: %v", err)
	}
	if ohlcv.Initialized {
		t.Error("OHLCV should be reset")
	}
}

func BenchmarkTickerSingleUpdate(b *testing.B) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(1, periods, PrecisionKahan)
	if err != nil {
		b.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	tick := &Tick{0, 100.0, 100.0, 10000.0, time.Unix(0, 1000000000)}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tick.Timestamp = time.Unix(0, int64(i)*1000000)
		_ = ticker.Update(tick)
	}
}

func BenchmarkTickerBatchUpdate(b *testing.B) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(5000, periods, PrecisionKahan)
	if err != nil {
		b.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	ticks := make([]Tick, 10000)
	for i := range ticks {
		ticks[i] = Tick{
			SymbolID:  uint32(i % 5000),
			Price:     100.0 + float64(i%100),
			Volume:    100.0,
			Amount:    10000.0,
			Timestamp: time.Unix(0, int64(i)*1000000),
		}
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = ticker.UpdateBatch(ticks)
	}
}
