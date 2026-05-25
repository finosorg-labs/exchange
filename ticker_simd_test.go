package exchange

import (
	"fmt"
	"math"
	"testing"
	"time"
)

// TestTickerSIMDBatchProcessing tests SIMD batch processing with various batch sizes
func TestTickerSIMDBatchProcessing(t *testing.T) {
	batchSizes := []int{1, 4, 8, 16, 32, 64, 128}

	for _, batchSize := range batchSizes {
		t.Run(fmt.Sprintf("BatchSize_%d", batchSize), func(t *testing.T) {
			periods := []time.Duration{time.Minute}
			ticker, err := NewTicker(1, periods, PrecisionStandard)
			if err != nil {
				t.Fatalf("Failed to create ticker: %v", err)
			}
			defer ticker.Close()

			// Create batch
			baseTime := time.Unix(0, 1000000000)
			ticks := make([]Tick, batchSize)
			for i := 0; i < batchSize; i++ {
				ticks[i] = Tick{
					SymbolID:  0,
					Price:     100.0 + float64(i%100),
					Volume:    1000.0 + float64(i),
					Amount:    (100.0 + float64(i%100)) * (1000.0 + float64(i)),
					Timestamp: baseTime.Add(time.Duration(i) * time.Millisecond),
				}
			}

			// Process batch
			err = ticker.UpdateBatch(ticks)
			if err != nil {
				t.Fatalf("Failed to update batch: %v", err)
			}

			// Verify results
			ohlcvs, err := ticker.GetSymbolOHLCV(0)
			if err != nil {
				t.Fatalf("Failed to get OHLCV: %v", err)
			}

			if len(ohlcvs) == 0 {
				t.Fatal("No OHLCV data")
			}

			ohlcv := ohlcvs[0]
			if ohlcv.Open <= 0 {
				t.Errorf("Invalid open price: %f", ohlcv.Open)
			}
			if ohlcv.High < ohlcv.Low {
				t.Errorf("High (%f) should be >= Low (%f)", ohlcv.High, ohlcv.Low)
			}
			if ohlcv.Volume <= 0 {
				t.Errorf("Invalid volume: %f", ohlcv.Volume)
			}
		})
	}
}

// TestTickerSIMDCorrectness compares SIMD results with expected values
func TestTickerSIMDCorrectness(t *testing.T) {
	periods := []time.Duration{time.Minute}
	ticker, err := NewTicker(1, periods, PrecisionStandard)
	if err != nil {
		t.Fatalf("Failed to create ticker: %v", err)
	}
	defer ticker.Close()

	// Create a batch with known values
	baseTime := time.Unix(0, 1000000000)
	ticks := []Tick{
		{SymbolID: 0, Price: 100.0, Volume: 1000.0, Amount: 100000.0, Timestamp: baseTime},
		{SymbolID: 0, Price: 105.5, Volume: 1500.0, Amount: 158250.0, Timestamp: baseTime.Add(time.Second)},
		{SymbolID: 0, Price: 98.3, Volume: 2000.0, Amount: 196600.0, Timestamp: baseTime.Add(2 * time.Second)},
		{SymbolID: 0, Price: 110.2, Volume: 1200.0, Amount: 132240.0, Timestamp: baseTime.Add(3 * time.Second)},
		{SymbolID: 0, Price: 95.7, Volume: 2500.0, Amount: 239250.0, Timestamp: baseTime.Add(4 * time.Second)},
	}

	err = ticker.UpdateBatch(ticks)
	if err != nil {
		t.Fatalf("Failed to update batch: %v", err)
	}

	ohlcvs, err := ticker.GetSymbolOHLCV(0)
	if err != nil {
		t.Fatalf("Failed to get OHLCV: %v", err)
	}

	if len(ohlcvs) == 0 {
		t.Fatal("No OHLCV data")
	}

	ohlcv := ohlcvs[0]

	// Expected values
	expectedHigh := 110.2
	expectedLow := 95.7
	expectedVolume := 8200.0
	expectedAmount := 826340.0

	epsilon := 1e-6

	if math.Abs(ohlcv.High-expectedHigh) > epsilon {
		t.Errorf("Expected high=%f, got %f", expectedHigh, ohlcv.High)
	}
	if math.Abs(ohlcv.Low-expectedLow) > epsilon {
		t.Errorf("Expected low=%f, got %f", expectedLow, ohlcv.Low)
	}
	if math.Abs(ohlcv.Volume-expectedVolume) > epsilon {
		t.Errorf("Expected volume=%f, got %f", expectedVolume, ohlcv.Volume)
	}
	if math.Abs(ohlcv.Amount-expectedAmount) > epsilon {
		t.Errorf("Expected amount=%f, got %f", expectedAmount, ohlcv.Amount)
	}
}

// BenchmarkTickerSIMDBatch benchmarks SIMD batch processing
func BenchmarkTickerSIMDBatch(b *testing.B) {
	batchSizes := []int{8, 16, 32, 64, 128, 256}

	for _, batchSize := range batchSizes {
		b.Run(fmt.Sprintf("BatchSize_%d", batchSize), func(b *testing.B) {
			periods := []time.Duration{time.Minute}
			ticker, err := NewTicker(1, periods, PrecisionStandard)
			if err != nil {
				b.Fatalf("Failed to create ticker: %v", err)
			}
			defer ticker.Close()

			// Prepare batch
			baseTime := time.Unix(0, 1000000000)
			ticks := make([]Tick, batchSize)
			for i := 0; i < batchSize; i++ {
				ticks[i] = Tick{
					SymbolID:  0,
					Price:     100.0 + float64(i%100),
					Volume:    1000.0 + float64(i),
					Amount:    (100.0 + float64(i%100)) * (1000.0 + float64(i)),
					Timestamp: baseTime.Add(time.Duration(i) * time.Millisecond),
				}
			}

			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				err := ticker.UpdateBatch(ticks)
				if err != nil {
					b.Fatalf("UpdateBatch failed: %v", err)
				}
			}
		})
	}
}
