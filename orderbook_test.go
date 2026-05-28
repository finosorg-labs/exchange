package exchange

import (
	"math"
	"testing"
	"time"
)

func TestAggregateLevelsBasic(t *testing.T) {
	orders := []Order{
		{SymbolID: 0, Price: 100.0, Volume: 10.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 0, Price: 100.0, Volume: 20.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 0, Price: 99.5, Volume: 15.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 0, Price: 99.5, Volume: 25.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 0, Price: 99.0, Volume: 30.0, Side: OrderBookSideBid, Timestamp: time.Now()},
	}

	levels, err := AggregateLevels(orders, OrderBookPrecisionStandard)
	if err != nil {
		t.Fatalf("AggregateLevels failed: %v", err)
	}

	if len(levels) != 3 {
		t.Errorf("Expected 3 levels, got %d", len(levels))
	}

	if math.Abs(levels[0].Price-100.0) > 1e-10 {
		t.Errorf("Expected first level price 100.0, got %.2f", levels[0].Price)
	}
	if math.Abs(levels[0].Volume-30.0) > 1e-10 {
		t.Errorf("Expected first level volume 30.0, got %.2f", levels[0].Volume)
	}

	if math.Abs(levels[1].Price-99.5) > 1e-10 {
		t.Errorf("Expected second level price 99.5, got %.2f", levels[1].Price)
	}
	if math.Abs(levels[1].Volume-40.0) > 1e-10 {
		t.Errorf("Expected second level volume 40.0, got %.2f", levels[1].Volume)
	}
}

func TestAggregateLevelsKahan(t *testing.T) {
	orders := make([]Order, 1000)
	for i := 0; i < 1000; i++ {
		orders[i] = Order{
			SymbolID:  0,
			Price:     100.0,
			Volume:    0.1,
			Side:      OrderBookSideBid,
			Timestamp: time.Now(),
		}
	}

	levels, err := AggregateLevels(orders, OrderBookPrecisionKahan)
	if err != nil {
		t.Fatalf("AggregateLevels with Kahan failed: %v", err)
	}

	if len(levels) != 1 {
		t.Errorf("Expected 1 level, got %d", len(levels))
	}

	if math.Abs(levels[0].Price-100.0) > 1e-10 {
		t.Errorf("Expected price 100.0, got %.2f", levels[0].Price)
	}

	// Kahan summation should give more accurate result
	if math.Abs(levels[0].Volume-100.0) > 1e-6 {
		t.Errorf("Expected volume close to 100.0, got %.10f", levels[0].Volume)
	}
}

func TestGenerateSnapshotSingleSymbol(t *testing.T) {
	orders := []Order{
		{SymbolID: 0, Price: 100.0, Volume: 10.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 0, Price: 100.0, Volume: 20.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 0, Price: 99.5, Volume: 15.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 0, Price: 100.5, Volume: 12.0, Side: OrderBookSideAsk, Timestamp: time.Now()},
		{SymbolID: 0, Price: 100.5, Volume: 18.0, Side: OrderBookSideAsk, Timestamp: time.Now()},
		{SymbolID: 0, Price: 101.0, Volume: 25.0, Side: OrderBookSideAsk, Timestamp: time.Now()},
	}

	timestamp := time.Now()
	snapshot, err := GenerateSnapshot(orders, 10, OrderBookPrecisionKahan, timestamp)
	if err != nil {
		t.Fatalf("GenerateSnapshot failed: %v", err)
	}

	if snapshot.SymbolID != 0 {
		t.Errorf("Expected symbol ID 0, got %d", snapshot.SymbolID)
	}

	if len(snapshot.Bids) != 2 {
		t.Errorf("Expected 2 bid levels, got %d", len(snapshot.Bids))
	}

	if len(snapshot.Asks) != 2 {
		t.Errorf("Expected 2 ask levels, got %d", len(snapshot.Asks))
	}

	if math.Abs(snapshot.Bids[0].Price-100.0) > 1e-10 {
		t.Errorf("Expected best bid price 100.0, got %.2f", snapshot.Bids[0].Price)
	}

	if math.Abs(snapshot.Bids[0].Volume-30.0) > 1e-10 {
		t.Errorf("Expected best bid volume 30.0, got %.2f", snapshot.Bids[0].Volume)
	}

	if math.Abs(snapshot.Asks[0].Price-100.5) > 1e-10 {
		t.Errorf("Expected best ask price 100.5, got %.2f", snapshot.Asks[0].Price)
	}

	if math.Abs(snapshot.Asks[0].Volume-30.0) > 1e-10 {
		t.Errorf("Expected best ask volume 30.0, got %.2f", snapshot.Asks[0].Volume)
	}

	if math.Abs(snapshot.Spread-0.5) > 1e-10 {
		t.Errorf("Expected spread 0.5, got %.2f", snapshot.Spread)
	}

	if math.Abs(snapshot.MidPrice-100.25) > 1e-10 {
		t.Errorf("Expected mid price 100.25, got %.2f", snapshot.MidPrice)
	}
}

func TestGenerateSnapshotMaxLevels(t *testing.T) {
	orders := make([]Order, 20)
	for i := 0; i < 10; i++ {
		orders[i] = Order{
			SymbolID:  0,
			Price:     100.0 - float64(i)*0.5,
			Volume:    10.0,
			Side:      OrderBookSideBid,
			Timestamp: time.Now(),
		}
	}
	for i := 0; i < 10; i++ {
		orders[10+i] = Order{
			SymbolID:  0,
			Price:     100.5 + float64(i)*0.5,
			Volume:    10.0,
			Side:      OrderBookSideAsk,
			Timestamp: time.Now(),
		}
	}

	snapshot, err := GenerateSnapshot(orders, 5, OrderBookPrecisionStandard, time.Now())
	if err != nil {
		t.Fatalf("GenerateSnapshot failed: %v", err)
	}

	if len(snapshot.Bids) != 5 {
		t.Errorf("Expected 5 bid levels (max), got %d", len(snapshot.Bids))
	}

	if len(snapshot.Asks) != 5 {
		t.Errorf("Expected 5 ask levels (max), got %d", len(snapshot.Asks))
	}
}

func TestGenerateSnapshotEmpty(t *testing.T) {
	snapshot, err := GenerateSnapshot([]Order{}, 10, OrderBookPrecisionStandard, time.Now())
	if err != nil {
		t.Fatalf("GenerateSnapshot with empty orders failed: %v", err)
	}

	if len(snapshot.Bids) != 0 {
		t.Errorf("Expected 0 bid levels, got %d", len(snapshot.Bids))
	}

	if len(snapshot.Asks) != 0 {
		t.Errorf("Expected 0 ask levels, got %d", len(snapshot.Asks))
	}

	if math.Abs(snapshot.Spread) > 1e-10 {
		t.Errorf("Expected spread 0, got %.2f", snapshot.Spread)
	}
}

func TestGenerateSnapshotBatch(t *testing.T) {
	orders := []Order{
		{SymbolID: 0, Price: 100.0, Volume: 10.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 0, Price: 100.5, Volume: 12.0, Side: OrderBookSideAsk, Timestamp: time.Now()},
		{SymbolID: 1, Price: 200.0, Volume: 20.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 1, Price: 200.5, Volume: 22.0, Side: OrderBookSideAsk, Timestamp: time.Now()},
		{SymbolID: 2, Price: 300.0, Volume: 30.0, Side: OrderBookSideBid, Timestamp: time.Now()},
		{SymbolID: 2, Price: 300.5, Volume: 32.0, Side: OrderBookSideAsk, Timestamp: time.Now()},
	}

	snapshots, err := GenerateSnapshotBatch(orders, 3, 10, OrderBookPrecisionStandard, time.Now())
	if err != nil {
		t.Fatalf("GenerateSnapshotBatch failed: %v", err)
	}

	if len(snapshots) != 3 {
		t.Errorf("Expected 3 snapshots, got %d", len(snapshots))
	}

	if snapshots[0].SymbolID != 0 {
		t.Errorf("Expected symbol 0 ID to be 0, got %d", snapshots[0].SymbolID)
	}

	if len(snapshots[0].Bids) != 1 {
		t.Errorf("Expected symbol 0 to have 1 bid level, got %d", len(snapshots[0].Bids))
	}

	if math.Abs(snapshots[0].Bids[0].Price-100.0) > 1e-10 {
		t.Errorf("Expected symbol 0 bid price 100.0, got %.2f", snapshots[0].Bids[0].Price)
	}

	if snapshots[1].SymbolID != 1 {
		t.Errorf("Expected symbol 1 ID to be 1, got %d", snapshots[1].SymbolID)
	}

	if math.Abs(snapshots[1].Bids[0].Price-200.0) > 1e-10 {
		t.Errorf("Expected symbol 1 bid price 200.0, got %.2f", snapshots[1].Bids[0].Price)
	}

	if snapshots[2].SymbolID != 2 {
		t.Errorf("Expected symbol 2 ID to be 2, got %d", snapshots[2].SymbolID)
	}

	if math.Abs(snapshots[2].Bids[0].Price-300.0) > 1e-10 {
		t.Errorf("Expected symbol 2 bid price 300.0, got %.2f", snapshots[2].Bids[0].Price)
	}
}

func TestSnapshotHelperMethods(t *testing.T) {
	snapshot := &OrderBookSnapshot{
		Bids: []PriceLevel{
			{Price: 100.0, Volume: 50.0},
			{Price: 99.5, Volume: 30.0},
		},
		Asks: []PriceLevel{
			{Price: 100.5, Volume: 40.0},
			{Price: 101.0, Volume: 25.0},
		},
		Spread:           0.5,
		MidPrice:         100.25,
		WeightedMidPrice: 100.22,
		SymbolID:         0,
	}

	bestBid := snapshot.BestBid()
	if bestBid == nil {
		t.Fatal("BestBid should not be nil")
	}
	if math.Abs(bestBid.Price-100.0) > 1e-10 {
		t.Errorf("Expected best bid price 100.0, got %.2f", bestBid.Price)
	}

	bestAsk := snapshot.BestAsk()
	if bestAsk == nil {
		t.Fatal("BestAsk should not be nil")
	}
	if math.Abs(bestAsk.Price-100.5) > 1e-10 {
		t.Errorf("Expected best ask price 100.5, got %.2f", bestAsk.Price)
	}

	totalBidVol := snapshot.TotalBidVolume()
	if math.Abs(totalBidVol-80.0) > 1e-10 {
		t.Errorf("Expected total bid volume 80.0, got %.2f", totalBidVol)
	}

	totalAskVol := snapshot.TotalAskVolume()
	if math.Abs(totalAskVol-65.0) > 1e-10 {
		t.Errorf("Expected total ask volume 65.0, got %.2f", totalAskVol)
	}

	imbalance := snapshot.Imbalance()
	expectedImbalance := 80.0 / (80.0 + 65.0)
	if math.Abs(imbalance-expectedImbalance) > 1e-10 {
		t.Errorf("Expected imbalance %.4f, got %.4f", expectedImbalance, imbalance)
	}

	str := snapshot.String()
	if str == "" {
		t.Error("String() should not return empty string")
	}
}

func TestSnapshotEmptyHelperMethods(t *testing.T) {
	snapshot := &OrderBookSnapshot{
		Bids: []PriceLevel{},
		Asks: []PriceLevel{},
	}

	if snapshot.BestBid() != nil {
		t.Error("BestBid should be nil for empty snapshot")
	}

	if snapshot.BestAsk() != nil {
		t.Error("BestAsk should be nil for empty snapshot")
	}

	if snapshot.TotalBidVolume() != 0.0 {
		t.Error("TotalBidVolume should be 0 for empty snapshot")
	}

	if snapshot.TotalAskVolume() != 0.0 {
		t.Error("TotalAskVolume should be 0 for empty snapshot")
	}

	if math.Abs(snapshot.Imbalance()-0.5) > 1e-10 {
		t.Error("Imbalance should be 0.5 for empty snapshot")
	}
}

func BenchmarkAggregateLevels100(b *testing.B) {
	orders := make([]Order, 100)
	for i := 0; i < 100; i++ {
		orders[i] = Order{
			SymbolID:  0,
			Price:     100.0 - float64(i/10)*0.5,
			Volume:    10.0,
			Side:      OrderBookSideBid,
			Timestamp: time.Now(),
		}
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = AggregateLevels(orders, OrderBookPrecisionKahan)
	}
}

func BenchmarkGenerateSnapshotSingle(b *testing.B) {
	orders := make([]Order, 100)
	for i := 0; i < 50; i++ {
		orders[i] = Order{
			SymbolID:  0,
			Price:     100.0 - float64(i)*0.1,
			Volume:    10.0 + float64(i),
			Side:      OrderBookSideBid,
			Timestamp: time.Now(),
		}
	}
	for i := 0; i < 50; i++ {
		orders[50+i] = Order{
			SymbolID:  0,
			Price:     100.5 + float64(i)*0.1,
			Volume:    10.0 + float64(i),
			Side:      OrderBookSideAsk,
			Timestamp: time.Now(),
		}
	}

	timestamp := time.Now()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = GenerateSnapshot(orders, 10, OrderBookPrecisionKahan, timestamp)
	}
}

func BenchmarkGenerateSnapshotBatch100(b *testing.B) {
	const numSymbols = 100
	const ordersPerSymbol = 20
	orders := make([]Order, numSymbols*ordersPerSymbol)

	for i := 0; i < len(orders); i++ {
		symbolID := uint32(i / ordersPerSymbol)
		orders[i] = Order{
			SymbolID:  symbolID,
			Price:     100.0 + float64(symbolID),
			Volume:    10.0,
			Side:      OrderBookSide(i % 2),
			Timestamp: time.Now(),
		}
	}

	timestamp := time.Now()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = GenerateSnapshotBatch(orders, numSymbols, 10, OrderBookPrecisionKahan, timestamp)
	}
}

func BenchmarkGenerateSnapshotBatch1000(b *testing.B) {
	const numSymbols = 1000
	const ordersPerSymbol = 50
	orders := make([]Order, numSymbols*ordersPerSymbol)

	for i := 0; i < len(orders); i++ {
		symbolID := uint32(i / ordersPerSymbol)
		orders[i] = Order{
			SymbolID:  symbolID,
			Price:     100.0 + float64(symbolID),
			Volume:    10.0,
			Side:      OrderBookSide(i % 2),
			Timestamp: time.Now(),
		}
	}

	timestamp := time.Now()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = GenerateSnapshotBatch(orders, numSymbols, 10, OrderBookPrecisionKahan, timestamp)
	}
}

func TestGenerateSnapshotUnsorted(t *testing.T) {
	orders := []Order{
		{SymbolID: 0, Price: 100.5, Volume: 12.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 3)},
		{SymbolID: 0, Price: 99.5, Volume: 15.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 2)},
		{SymbolID: 0, Price: 100.0, Volume: 20.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 1)},
		{SymbolID: 0, Price: 101.0, Volume: 25.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 5)},
		{SymbolID: 0, Price: 100.0, Volume: 10.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 0)},
		{SymbolID: 0, Price: 100.5, Volume: 18.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 4)},
	}

	timestamp := time.Now()
	snapshot, err := GenerateSnapshotUnsorted(orders, 10, OrderBookPrecisionKahan, timestamp)
	if err != nil {
		t.Fatalf("GenerateSnapshotUnsorted failed: %v", err)
	}

	if snapshot.SymbolID != 0 {
		t.Errorf("Expected symbol ID 0, got %d", snapshot.SymbolID)
	}

	if len(snapshot.Bids) != 2 {
		t.Errorf("Expected 2 bid levels, got %d", len(snapshot.Bids))
	}

	if len(snapshot.Asks) != 2 {
		t.Errorf("Expected 2 ask levels, got %d", len(snapshot.Asks))
	}

	if math.Abs(snapshot.Bids[0].Price-100.0) > 1e-10 {
		t.Errorf("Expected best bid price 100.0, got %.2f", snapshot.Bids[0].Price)
	}

	if math.Abs(snapshot.Bids[0].Volume-30.0) > 1e-10 {
		t.Errorf("Expected best bid volume 30.0, got %.2f", snapshot.Bids[0].Volume)
	}

	if math.Abs(snapshot.Asks[0].Price-100.5) > 1e-10 {
		t.Errorf("Expected best ask price 100.5, got %.2f", snapshot.Asks[0].Price)
	}

	if math.Abs(snapshot.Asks[0].Volume-30.0) > 1e-10 {
		t.Errorf("Expected best ask volume 30.0, got %.2f", snapshot.Asks[0].Volume)
	}

	if math.Abs(snapshot.Spread-0.5) > 1e-10 {
		t.Errorf("Expected spread 0.5, got %.2f", snapshot.Spread)
	}

	if math.Abs(snapshot.MidPrice-100.25) > 1e-10 {
		t.Errorf("Expected mid price 100.25, got %.2f", snapshot.MidPrice)
	}
}

func TestGenerateSnapshotComparison(t *testing.T) {
	sortedOrders := []Order{
		{SymbolID: 0, Price: 100.0, Volume: 10.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 0)},
		{SymbolID: 0, Price: 100.0, Volume: 20.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 1)},
		{SymbolID: 0, Price: 99.5, Volume: 15.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 2)},
		{SymbolID: 0, Price: 100.5, Volume: 12.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 3)},
		{SymbolID: 0, Price: 100.5, Volume: 18.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 4)},
		{SymbolID: 0, Price: 101.0, Volume: 25.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 5)},
	}

	unsortedOrders := []Order{
		{SymbolID: 0, Price: 100.5, Volume: 12.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 3)},
		{SymbolID: 0, Price: 99.5, Volume: 15.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 2)},
		{SymbolID: 0, Price: 100.0, Volume: 20.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 1)},
		{SymbolID: 0, Price: 101.0, Volume: 25.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 5)},
		{SymbolID: 0, Price: 100.0, Volume: 10.0, Side: OrderBookSideBid, Timestamp: time.Unix(0, 0)},
		{SymbolID: 0, Price: 100.5, Volume: 18.0, Side: OrderBookSideAsk, Timestamp: time.Unix(0, 4)},
	}

	timestamp := time.Now()

	sortedSnapshot, err := GenerateSnapshot(sortedOrders, 10, OrderBookPrecisionKahan, timestamp)
	if err != nil {
		t.Fatalf("GenerateSnapshot failed: %v", err)
	}

	unsortedSnapshot, err := GenerateSnapshotUnsorted(unsortedOrders, 10, OrderBookPrecisionKahan, timestamp)
	if err != nil {
		t.Fatalf("GenerateSnapshotUnsorted failed: %v", err)
	}

	if len(sortedSnapshot.Bids) != len(unsortedSnapshot.Bids) {
		t.Errorf("Bid levels mismatch: sorted=%d, unsorted=%d", len(sortedSnapshot.Bids), len(unsortedSnapshot.Bids))
	}

	if len(sortedSnapshot.Asks) != len(unsortedSnapshot.Asks) {
		t.Errorf("Ask levels mismatch: sorted=%d, unsorted=%d", len(sortedSnapshot.Asks), len(unsortedSnapshot.Asks))
	}

	for i := 0; i < len(sortedSnapshot.Bids); i++ {
		if math.Abs(sortedSnapshot.Bids[i].Price-unsortedSnapshot.Bids[i].Price) > 1e-10 {
			t.Errorf("Bid[%d] price mismatch: sorted=%.2f, unsorted=%.2f", i, sortedSnapshot.Bids[i].Price, unsortedSnapshot.Bids[i].Price)
		}
		if math.Abs(sortedSnapshot.Bids[i].Volume-unsortedSnapshot.Bids[i].Volume) > 1e-10 {
			t.Errorf("Bid[%d] volume mismatch: sorted=%.2f, unsorted=%.2f", i, sortedSnapshot.Bids[i].Volume, unsortedSnapshot.Bids[i].Volume)
		}
	}

	for i := 0; i < len(sortedSnapshot.Asks); i++ {
		if math.Abs(sortedSnapshot.Asks[i].Price-unsortedSnapshot.Asks[i].Price) > 1e-10 {
			t.Errorf("Ask[%d] price mismatch: sorted=%.2f, unsorted=%.2f", i, sortedSnapshot.Asks[i].Price, unsortedSnapshot.Asks[i].Price)
		}
		if math.Abs(sortedSnapshot.Asks[i].Volume-unsortedSnapshot.Asks[i].Volume) > 1e-10 {
			t.Errorf("Ask[%d] volume mismatch: sorted=%.2f, unsorted=%.2f", i, sortedSnapshot.Asks[i].Volume, unsortedSnapshot.Asks[i].Volume)
		}
	}

	if math.Abs(sortedSnapshot.Spread-unsortedSnapshot.Spread) > 1e-10 {
		t.Errorf("Spread mismatch: sorted=%.2f, unsorted=%.2f", sortedSnapshot.Spread, unsortedSnapshot.Spread)
	}

	if math.Abs(sortedSnapshot.MidPrice-unsortedSnapshot.MidPrice) > 1e-10 {
		t.Errorf("MidPrice mismatch: sorted=%.2f, unsorted=%.2f", sortedSnapshot.MidPrice, unsortedSnapshot.MidPrice)
	}
}

func BenchmarkGenerateSnapshotUnsorted100(b *testing.B) {
	orders := make([]Order, 100)
	for i := 0; i < 50; i++ {
		orders[i*2] = Order{
			SymbolID:  0,
			Price:     100.0 - float64(i)*0.1,
			Volume:    10.0 + float64(i),
			Side:      OrderBookSideBid,
			Timestamp: time.Unix(0, int64(i)),
		}
		orders[i*2+1] = Order{
			SymbolID:  0,
			Price:     100.5 + float64(i)*0.1,
			Volume:    10.0 + float64(i),
			Side:      OrderBookSideAsk,
			Timestamp: time.Unix(0, int64(50+i)),
		}
	}

	timestamp := time.Now()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		ordersCopy := make([]Order, len(orders))
		copy(ordersCopy, orders)
		_, _ = GenerateSnapshotUnsorted(ordersCopy, 10, OrderBookPrecisionKahan, timestamp)
	}
}

func BenchmarkGenerateSnapshotUnsorted1000(b *testing.B) {
	orders := make([]Order, 1000)
	for i := 0; i < 500; i++ {
		orders[i*2] = Order{
			SymbolID:  0,
			Price:     100.0 - float64(i)*0.01,
			Volume:    10.0 + float64(i),
			Side:      OrderBookSideBid,
			Timestamp: time.Unix(0, int64(i)),
		}
		orders[i*2+1] = Order{
			SymbolID:  0,
			Price:     100.5 + float64(i)*0.01,
			Volume:    10.0 + float64(i),
			Side:      OrderBookSideAsk,
			Timestamp: time.Unix(0, int64(500+i)),
		}
	}

	timestamp := time.Now()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		ordersCopy := make([]Order, len(orders))
		copy(ordersCopy, orders)
		_, _ = GenerateSnapshotUnsorted(ordersCopy, 10, OrderBookPrecisionKahan, timestamp)
	}
}

func BenchmarkGenerateSnapshotSortedVsUnsorted(b *testing.B) {
	sortedOrders := make([]Order, 500)
	for i := 0; i < 250; i++ {
		sortedOrders[i] = Order{
			SymbolID:  0,
			Price:     100.0 - float64(i)*0.01,
			Volume:    10.0 + float64(i),
			Side:      OrderBookSideBid,
			Timestamp: time.Unix(0, int64(i)),
		}
	}
	for i := 0; i < 250; i++ {
		sortedOrders[250+i] = Order{
			SymbolID:  0,
			Price:     100.5 + float64(i)*0.01,
			Volume:    10.0 + float64(i),
			Side:      OrderBookSideAsk,
			Timestamp: time.Unix(0, int64(250+i)),
		}
	}

	timestamp := time.Now()

	b.Run("Sorted", func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_, _ = GenerateSnapshot(sortedOrders, 10, OrderBookPrecisionKahan, timestamp)
		}
	})

	b.Run("Unsorted", func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			ordersCopy := make([]Order, len(sortedOrders))
			copy(ordersCopy, sortedOrders)
			_, _ = GenerateSnapshotUnsorted(ordersCopy, 10, OrderBookPrecisionKahan, timestamp)
		}
	})
}

