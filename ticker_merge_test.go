package exchange

import (
	"testing"
	"time"
)

func TestTickerMergeCreate(t *testing.T) {
	tests := []struct {
		name           string
		numSymbols     uint32
		basePeriod     time.Duration
		derivedPeriods []time.Duration
		precisionMode  PrecisionMode
		wantErr        bool
	}{
		{
			name:           "valid single derived period",
			numSymbols:     1,
			basePeriod:     time.Minute,
			derivedPeriods: []time.Duration{5 * time.Minute},
			precisionMode:  PrecisionStandard,
			wantErr:        false,
		},
		{
			name:           "valid multiple derived periods",
			numSymbols:     10,
			basePeriod:     time.Minute,
			derivedPeriods: []time.Duration{5 * time.Minute, 15 * time.Minute, time.Hour},
			precisionMode:  PrecisionKahan,
			wantErr:        false,
		},
		{
			name:           "no derived periods",
			numSymbols:     1,
			basePeriod:     time.Minute,
			derivedPeriods: nil,
			precisionMode:  PrecisionStandard,
			wantErr:        false,
		},
		{
			name:           "zero symbols",
			numSymbols:     0,
			basePeriod:     time.Minute,
			derivedPeriods: []time.Duration{5 * time.Minute},
			precisionMode:  PrecisionStandard,
			wantErr:        true,
		},
		{
			name:           "zero base period",
			numSymbols:     1,
			basePeriod:     0,
			derivedPeriods: []time.Duration{5 * time.Minute},
			precisionMode:  PrecisionStandard,
			wantErr:        true,
		},
		{
			name:           "derived period not multiple of base",
			numSymbols:     1,
			basePeriod:     time.Minute,
			derivedPeriods: []time.Duration{90 * time.Second},
			precisionMode:  PrecisionStandard,
			wantErr:        true,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			tm, err := NewTickerMerge(tt.numSymbols, tt.basePeriod, tt.derivedPeriods, tt.precisionMode, nil)
			if (err != nil) != tt.wantErr {
				t.Errorf("NewTickerMerge() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if tm != nil {
				defer tm.Close()
			}
		})
	}
}

func TestTickerMergeBasicUpdate(t *testing.T) {
	tm, err := NewTickerMerge(1, time.Minute, nil, PrecisionStandard, nil)
	if err != nil {
		t.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}

	err = tm.Update(tick)
	if err != nil {
		t.Fatalf("Update() error = %v", err)
	}

	ohlcv, err := tm.GetBaseOHLCV(0)
	if err != nil {
		t.Fatalf("GetBaseOHLCV() error = %v", err)
	}

	if !ohlcv.Initialized {
		t.Error("OHLCV should be initialized")
	}
	if ohlcv.Open != 100.0 {
		t.Errorf("Open = %v, want 100.0", ohlcv.Open)
	}
	if ohlcv.High != 100.0 {
		t.Errorf("High = %v, want 100.0", ohlcv.High)
	}
	if ohlcv.Low != 100.0 {
		t.Errorf("Low = %v, want 100.0", ohlcv.Low)
	}
	if ohlcv.Close != 100.0 {
		t.Errorf("Close = %v, want 100.0", ohlcv.Close)
	}
	if ohlcv.Volume != 10.0 {
		t.Errorf("Volume = %v, want 10.0", ohlcv.Volume)
	}
	if ohlcv.Amount != 1000.0 {
		t.Errorf("Amount = %v, want 1000.0", ohlcv.Amount)
	}
	if ohlcv.TickCount != 1 {
		t.Errorf("TickCount = %v, want 1", ohlcv.TickCount)
	}
}

func TestTickerMergeDerivedPeriod(t *testing.T) {
	derivedPeriods := []time.Duration{5 * time.Minute}
	tm, err := NewTickerMerge(1, time.Minute, derivedPeriods, PrecisionStandard, nil)
	if err != nil {
		t.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	baseTime = baseTime.Truncate(time.Minute)

	// Send ticks for 6 minutes to complete one 5-minute period
	for i := 0; i < 6; i++ {
		for j := 0; j < 10; j++ {
			tick := &Tick{
				SymbolID:  0,
				Price:     100.0 + float64(i),
				Volume:    10.0,
				Amount:    (100.0 + float64(i)) * 10.0,
				Timestamp: baseTime.Add(time.Duration(i)*time.Minute + time.Duration(j)*time.Second),
			}
			err = tm.Update(tick)
			if err != nil {
				t.Fatalf("Update() error = %v", err)
			}
		}
	}

	ohlcv, err := tm.GetDerivedOHLCV(0, 0)
	if err != nil {
		t.Fatalf("GetDerivedOHLCV() error = %v", err)
	}

	if !ohlcv.Initialized {
		t.Error("Derived OHLCV should be initialized")
	}
	if ohlcv.Open != 100.0 {
		t.Errorf("Open = %v, want 100.0", ohlcv.Open)
	}
	if ohlcv.Close != 104.0 {
		t.Errorf("Close = %v, want 104.0", ohlcv.Close)
	}
	if ohlcv.High != 104.0 {
		t.Errorf("High = %v, want 104.0", ohlcv.High)
	}
	if ohlcv.Low != 100.0 {
		t.Errorf("Low = %v, want 100.0", ohlcv.Low)
	}
	if ohlcv.Volume != 500.0 {
		t.Errorf("Volume = %v, want 500.0", ohlcv.Volume)
	}
	if ohlcv.TickCount != 50 {
		t.Errorf("TickCount = %v, want 50", ohlcv.TickCount)
	}
}

func TestTickerMergeCallback(t *testing.T) {
	t.Skip("Callback feature not yet implemented in Go bindings")

	callbackCalled := false
	var callbackSymbolID uint32
	var callbackPeriodIdx uint32
	var callbackOHLCV *OHLCV

	callback := func(symbolID uint32, derivedPeriodIdx uint32, ohlcv *OHLCV) {
		callbackCalled = true
		callbackSymbolID = symbolID
		callbackPeriodIdx = derivedPeriodIdx
		callbackOHLCV = ohlcv
	}

	derivedPeriods := []time.Duration{5 * time.Minute}
	tm, err := NewTickerMerge(1, time.Minute, derivedPeriods, PrecisionStandard, callback)
	if err != nil {
		t.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	baseTime = baseTime.Truncate(time.Minute)

	for i := 0; i < 5; i++ {
		tick := &Tick{
			SymbolID:  0,
			Price:     100.0,
			Volume:    10.0,
			Amount:    1000.0,
			Timestamp: baseTime.Add(time.Duration(i) * time.Minute),
		}
		err = tm.Update(tick)
		if err != nil {
			t.Fatalf("Update() error = %v", err)
		}
	}

	if !callbackCalled {
		t.Error("Callback should have been called")
	}
	if callbackSymbolID != 0 {
		t.Errorf("Callback symbolID = %v, want 0", callbackSymbolID)
	}
	if callbackPeriodIdx != 0 {
		t.Errorf("Callback periodIdx = %v, want 0", callbackPeriodIdx)
	}
	if callbackOHLCV == nil {
		t.Fatal("Callback OHLCV should not be nil")
	}
	if callbackOHLCV.Volume != 50.0 {
		t.Errorf("Callback OHLCV Volume = %v, want 50.0", callbackOHLCV.Volume)
	}
}

func TestTickerMergeBatchUpdate(t *testing.T) {
	tm, err := NewTickerMerge(1, time.Minute, nil, PrecisionStandard, nil)
	if err != nil {
		t.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	ticks := make([]Tick, 10)
	for i := 0; i < 10; i++ {
		ticks[i] = Tick{
			SymbolID:  0,
			Price:     100.0 + float64(i),
			Volume:    10.0,
			Amount:    (100.0 + float64(i)) * 10.0,
			Timestamp: baseTime.Add(time.Duration(i) * time.Second),
		}
	}

	err = tm.UpdateBatch(ticks)
	if err != nil {
		t.Fatalf("UpdateBatch() error = %v", err)
	}

	ohlcv, err := tm.GetBaseOHLCV(0)
	if err != nil {
		t.Fatalf("GetBaseOHLCV() error = %v", err)
	}

	if ohlcv.TickCount != 10 {
		t.Errorf("TickCount = %v, want 10", ohlcv.TickCount)
	}
	if ohlcv.Open != 100.0 {
		t.Errorf("Open = %v, want 100.0", ohlcv.Open)
	}
	if ohlcv.Close != 109.0 {
		t.Errorf("Close = %v, want 109.0", ohlcv.Close)
	}
}

func TestTickerMergeMultipleSymbols(t *testing.T) {
	tm, err := NewTickerMerge(3, time.Minute, nil, PrecisionStandard, nil)
	if err != nil {
		t.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	for symbolID := uint32(0); symbolID < 3; symbolID++ {
		tick := &Tick{
			SymbolID:  symbolID,
			Price:     100.0 + float64(symbolID)*10.0,
			Volume:    10.0,
			Amount:    (100.0 + float64(symbolID)*10.0) * 10.0,
			Timestamp: baseTime,
		}
		err = tm.Update(tick)
		if err != nil {
			t.Fatalf("Update() error = %v", err)
		}
	}

	for symbolID := uint32(0); symbolID < 3; symbolID++ {
		ohlcv, err := tm.GetBaseOHLCV(symbolID)
		if err != nil {
			t.Fatalf("GetBaseOHLCV(%d) error = %v", symbolID, err)
		}
		expectedPrice := 100.0 + float64(symbolID)*10.0
		if ohlcv.Open != expectedPrice {
			t.Errorf("Symbol %d: Open = %v, want %v", symbolID, ohlcv.Open, expectedPrice)
		}
	}
}

func TestTickerMergeReset(t *testing.T) {
	tm, err := NewTickerMerge(2, time.Minute, nil, PrecisionStandard, nil)
	if err != nil {
		t.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}
	err = tm.Update(tick)
	if err != nil {
		t.Fatalf("Update() error = %v", err)
	}

	err = tm.Reset(0)
	if err != nil {
		t.Fatalf("Reset() error = %v", err)
	}

	ohlcv, err := tm.GetBaseOHLCV(0)
	if err != nil {
		t.Fatalf("GetBaseOHLCV() error = %v", err)
	}

	if ohlcv.Initialized {
		t.Error("OHLCV should not be initialized after reset")
	}
}

func TestTickerMergeGetStats(t *testing.T) {
	numSymbols := uint32(5)
	basePeriod := time.Minute
	derivedPeriods := []time.Duration{5 * time.Minute, 15 * time.Minute}

	tm, err := NewTickerMerge(numSymbols, basePeriod, derivedPeriods, PrecisionStandard, nil)
	if err != nil {
		t.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	gotNumSymbols, gotBasePeriod, gotNumDerivedPeriods, err := tm.GetStats()
	if err != nil {
		t.Fatalf("GetStats() error = %v", err)
	}

	if gotNumSymbols != numSymbols {
		t.Errorf("NumSymbols = %v, want %v", gotNumSymbols, numSymbols)
	}
	if gotBasePeriod != basePeriod {
		t.Errorf("BasePeriod = %v, want %v", gotBasePeriod, basePeriod)
	}
	if gotNumDerivedPeriods != uint32(len(derivedPeriods)) {
		t.Errorf("NumDerivedPeriods = %v, want %v", gotNumDerivedPeriods, len(derivedPeriods))
	}
}

func TestTickerMergePrecisionModes(t *testing.T) {
	modes := []PrecisionMode{
		PrecisionStandard,
		PrecisionKahan,
	}

	for _, mode := range modes {
		t.Run(mode.String(), func(t *testing.T) {
			tm, err := NewTickerMerge(1, time.Minute, nil, mode, nil)
			if err != nil {
				t.Fatalf("NewTickerMerge() error = %v", err)
			}
			defer tm.Close()

			baseTime := time.Unix(1000000000, 0)
			baseTime = baseTime.Truncate(time.Minute)
			for i := 0; i < 100; i++ {
				tick := &Tick{
					SymbolID:  0,
					Price:     100.0,
					Volume:    0.1,
					Amount:    10.0,
					Timestamp: baseTime.Add(time.Duration(i) * 100 * time.Millisecond),
				}
				err = tm.Update(tick)
				if err != nil {
					t.Fatalf("Update() error = %v", err)
				}
			}

			ohlcv, err := tm.GetBaseOHLCV(0)
			if err != nil {
				t.Fatalf("GetBaseOHLCV() error = %v", err)
			}

			if ohlcv.Volume < 9.9 || ohlcv.Volume > 10.1 {
				t.Errorf("Volume = %v, want ~10.0", ohlcv.Volume)
			}
		})
	}
}

func (pm PrecisionMode) String() string {
	switch pm {
	case PrecisionStandard:
		return "Standard"
	case PrecisionKahan:
		return "Kahan"
	case PrecisionBigfloat:
		return "Bigfloat"
	default:
		return "Unknown"
	}
}

func BenchmarkTickerMergeBaseOnly(b *testing.B) {
	tm, err := NewTickerMerge(1, time.Minute, nil, PrecisionStandard, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tick.Timestamp = baseTime.Add(time.Duration(i) * time.Millisecond)
		_ = tm.Update(tick)
	}
}

func BenchmarkTickerMerge5min(b *testing.B) {
	derivedPeriods := []time.Duration{5 * time.Minute}
	tm, err := NewTickerMerge(1, time.Minute, derivedPeriods, PrecisionStandard, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tick.Timestamp = baseTime.Add(time.Duration(i) * time.Millisecond)
		_ = tm.Update(tick)
	}
}

func BenchmarkTickerMergeMultiPeriod(b *testing.B) {
	derivedPeriods := []time.Duration{5 * time.Minute, 15 * time.Minute, time.Hour}
	tm, err := NewTickerMerge(1, time.Minute, derivedPeriods, PrecisionStandard, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tick.Timestamp = baseTime.Add(time.Duration(i) * time.Millisecond)
		_ = tm.Update(tick)
	}
}

func BenchmarkTickerMergeMultiSymbol(b *testing.B) {
	const numSymbols = 100
	derivedPeriods := []time.Duration{5 * time.Minute}
	tm, err := NewTickerMerge(numSymbols, time.Minute, derivedPeriods, PrecisionStandard, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tick.SymbolID = uint32(i % numSymbols)
		tick.Timestamp = baseTime.Add(time.Duration(i) * time.Millisecond)
		_ = tm.Update(tick)
	}
}

func BenchmarkTickerMergeBatch(b *testing.B) {
	derivedPeriods := []time.Duration{5 * time.Minute}
	tm, err := NewTickerMerge(1, time.Minute, derivedPeriods, PrecisionStandard, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	batchSize := 1000
	ticks := make([]Tick, batchSize)
	for i := 0; i < batchSize; i++ {
		ticks[i] = Tick{
			SymbolID:  0,
			Price:     100.0,
			Volume:    10.0,
			Amount:    1000.0,
			Timestamp: baseTime.Add(time.Duration(i) * time.Millisecond),
		}
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = tm.UpdateBatch(ticks)
	}
}

func BenchmarkTickerMergePrecisionStandard(b *testing.B) {
	tm, err := NewTickerMerge(1, time.Minute, nil, PrecisionStandard, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tick.Timestamp = baseTime.Add(time.Duration(i) * time.Millisecond)
		_ = tm.Update(tick)
	}
}

func BenchmarkTickerMergePrecisionKahan(b *testing.B) {
	tm, err := NewTickerMerge(1, time.Minute, nil, PrecisionKahan, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tick.Timestamp = baseTime.Add(time.Duration(i) * time.Millisecond)
		_ = tm.Update(tick)
	}
}

func BenchmarkTickerMergePrecisionBigfloat(b *testing.B) {
	tm, err := NewTickerMerge(1, time.Minute, nil, PrecisionBigfloat, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tick.Timestamp = baseTime.Add(time.Duration(i) * time.Millisecond)
		_ = tm.Update(tick)
	}
}

func BenchmarkTickerMergeGetBaseOHLCV(b *testing.B) {
	tm, err := NewTickerMerge(1, time.Minute, nil, PrecisionStandard, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	tick := &Tick{
		SymbolID:  0,
		Price:     100.0,
		Volume:    10.0,
		Amount:    1000.0,
		Timestamp: baseTime,
	}
	_ = tm.Update(tick)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = tm.GetBaseOHLCV(0)
	}
}

func BenchmarkTickerMergeGetDerivedOHLCV(b *testing.B) {
	derivedPeriods := []time.Duration{5 * time.Minute}
	tm, err := NewTickerMerge(1, time.Minute, derivedPeriods, PrecisionStandard, nil)
	if err != nil {
		b.Fatalf("NewTickerMerge() error = %v", err)
	}
	defer tm.Close()

	baseTime := time.Unix(1000000000, 0)
	baseTime = baseTime.Truncate(time.Minute)

	// Generate enough ticks to complete one derived period
	for i := 0; i < 6; i++ {
		tick := &Tick{
			SymbolID:  0,
			Price:     100.0,
			Volume:    10.0,
			Amount:    1000.0,
			Timestamp: baseTime.Add(time.Duration(i) * time.Minute),
		}
		_ = tm.Update(tick)
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = tm.GetDerivedOHLCV(0, 0)
	}
}
