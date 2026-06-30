package exchange

import (
	"math"
	"testing"
)

const mmEpsilon = 1e-9

func TestMarketMakerQuotesBasic(t *testing.T) {
	midPrices := []float64{100.0, 200.0}
	inventories := []float64{0.0, 0.0}
	volatilities := []float64{0.02, 0.03}
	arrivalRates := []float64{10.0, 20.0}
	riskAversion := 0.1
	timeHorizon := 1.0

	bidPrices, askPrices, err := MarketMakerQuotes(
		midPrices, inventories, volatilities, arrivalRates, riskAversion, timeHorizon,
	)

	if err != nil {
		t.Fatalf("MarketMakerQuotes failed: %v", err)
	}

	if len(bidPrices) != 2 || len(askPrices) != 2 {
		t.Fatalf("Expected 2 prices, got %d bids and %d asks", len(bidPrices), len(askPrices))
	}

	for i := 0; i < 2; i++ {
		if bidPrices[i] >= midPrices[i] {
			t.Errorf("Bid price %f should be less than mid price %f", bidPrices[i], midPrices[i])
		}
		if askPrices[i] <= midPrices[i] {
			t.Errorf("Ask price %f should be greater than mid price %f", askPrices[i], midPrices[i])
		}
		if askPrices[i] <= bidPrices[i] {
			t.Errorf("Ask price %f should be greater than bid price %f", askPrices[i], bidPrices[i])
		}

		midFromQuotes := (bidPrices[i] + askPrices[i]) * 0.5
		if math.Abs(midFromQuotes-midPrices[i]) > mmEpsilon {
			t.Errorf("Mid from quotes %f != input mid %f", midFromQuotes, midPrices[i])
		}
	}
}

func TestMarketMakerQuotesWithInventory(t *testing.T) {
	midPrices := []float64{100.0}
	inventories := []float64{50.0}
	volatilities := []float64{0.02}
	arrivalRates := []float64{10.0}
	riskAversion := 0.1
	timeHorizon := 1.0

	bidPrices, askPrices, err := MarketMakerQuotes(
		midPrices, inventories, volatilities, arrivalRates, riskAversion, timeHorizon,
	)

	if err != nil {
		t.Fatalf("MarketMakerQuotes failed: %v", err)
	}

	reservationPrice := midPrices[0] - inventories[0]*riskAversion*timeHorizon*
		volatilities[0]*volatilities[0]

	if reservationPrice >= midPrices[0] {
		t.Errorf("Reservation price should be less than mid with positive inventory")
	}

	if bidPrices[0] >= reservationPrice {
		t.Errorf("Bid should be less than reservation price")
	}

	if askPrices[0] <= reservationPrice {
		t.Errorf("Ask should be greater than reservation price")
	}
}

func TestMarketMakerQuotesNegativeInventory(t *testing.T) {
	midPrices := []float64{100.0}
	inventories := []float64{-50.0}
	volatilities := []float64{0.02}
	arrivalRates := []float64{10.0}
	riskAversion := 0.1
	timeHorizon := 1.0

	bidPrices, askPrices, err := MarketMakerQuotes(
		midPrices, inventories, volatilities, arrivalRates, riskAversion, timeHorizon,
	)

	if err != nil {
		t.Fatalf("MarketMakerQuotes failed: %v", err)
	}

	reservationPrice := midPrices[0] - inventories[0]*riskAversion*timeHorizon*
		volatilities[0]*volatilities[0]

	if reservationPrice <= midPrices[0] {
		t.Errorf("Reservation price should be greater than mid with negative inventory")
	}

	if bidPrices[0] >= reservationPrice {
		t.Errorf("Bid should be less than reservation price")
	}

	if askPrices[0] <= reservationPrice {
		t.Errorf("Ask should be greater than reservation price")
	}
}

func TestMarketMakerQuotesBatch1000(t *testing.T) {
	n := 1000
	midPrices := make([]float64, n)
	inventories := make([]float64, n)
	volatilities := make([]float64, n)
	arrivalRates := make([]float64, n)

	for i := 0; i < n; i++ {
		midPrices[i] = 100.0 + float64(i)*0.1
		if i%2 == 0 {
			inventories[i] = 10.0
		} else {
			inventories[i] = -10.0
		}
		volatilities[i] = 0.01 + float64(i%100)*0.0001
		arrivalRates[i] = 5.0 + float64(i%50)*0.5
	}

	bidPrices, askPrices, err := MarketMakerQuotes(
		midPrices, inventories, volatilities, arrivalRates, 0.1, 1.0,
	)

	if err != nil {
		t.Fatalf("MarketMakerQuotes failed: %v", err)
	}

	if len(bidPrices) != n || len(askPrices) != n {
		t.Fatalf("Expected %d prices, got %d bids and %d asks", n, len(bidPrices), len(askPrices))
	}

	for i := 0; i < n; i++ {
		if bidPrices[i] <= 0 || askPrices[i] <= 0 {
			t.Errorf("Prices must be positive at index %d", i)
		}
		if askPrices[i] <= bidPrices[i] {
			t.Errorf("Ask must be greater than bid at index %d", i)
		}
	}
}

func TestMarketMakerQuotesInvalidArgs(t *testing.T) {
	midPrices := []float64{100.0}
	inventories := []float64{0.0}
	volatilities := []float64{0.02}
	arrivalRates := []float64{10.0}

	_, _, err := MarketMakerQuotes(
		midPrices, inventories, volatilities, arrivalRates, -0.1, 1.0,
	)
	if err == nil {
		t.Error("Expected error for negative risk aversion")
	}

	_, _, err = MarketMakerQuotes(
		midPrices, inventories, volatilities, arrivalRates, 0.1, -1.0,
	)
	if err == nil {
		t.Error("Expected error for negative time horizon")
	}

	_, _, err = MarketMakerQuotes(
		midPrices, []float64{}, volatilities, arrivalRates, 0.1, 1.0,
	)
	if err == nil {
		t.Error("Expected error for mismatched slice lengths")
	}

	_, _, err = MarketMakerQuotes(
		midPrices, inventories, volatilities, []float64{0.0}, 0.1, 1.0,
	)
	if err == nil {
		t.Error("Expected error for zero arrival rate")
	}

	_, _, err = MarketMakerQuotes(
		midPrices, inventories, volatilities, []float64{-5.0}, 0.1, 1.0,
	)
	if err == nil {
		t.Error("Expected error for negative arrival rate")
	}
}

func TestMarketMakerQuotesEmpty(t *testing.T) {
	bidPrices, askPrices, err := MarketMakerQuotes(
		[]float64{}, []float64{}, []float64{}, []float64{}, 0.1, 1.0,
	)

	if err != nil {
		t.Fatalf("Empty input should not error: %v", err)
	}

	if len(bidPrices) != 0 || len(askPrices) != 0 {
		t.Error("Expected empty output for empty input")
	}
}

func TestMarketMakerReservationPriceBasic(t *testing.T) {
	midPrices := []float64{100.0, 200.0, 50.0}
	inventories := []float64{10.0, -5.0, 0.0}
	volatilities := []float64{0.02, 0.03, 0.01}
	riskAversion := 0.1
	timeHorizon := 1.0

	reservationPrices, err := MarketMakerReservationPrice(
		midPrices, inventories, volatilities, riskAversion, timeHorizon,
	)

	if err != nil {
		t.Fatalf("MarketMakerReservationPrice failed: %v", err)
	}

	if len(reservationPrices) != 3 {
		t.Fatalf("Expected 3 reservation prices, got %d", len(reservationPrices))
	}

	gammaSigma2T := riskAversion * timeHorizon
	for i, expected := range []float64{
		100.0 - 10.0*gammaSigma2T*0.02*0.02,
		200.0 - (-5.0)*gammaSigma2T*0.03*0.03,
		50.0 - 0.0*gammaSigma2T*0.01*0.01,
	} {
		if math.Abs(reservationPrices[i]-expected) > mmEpsilon {
			t.Errorf("Reservation price[%d] = %f, expected %f", i, reservationPrices[i], expected)
		}
	}
}

func TestMarketMakerOptimalSpreadBasic(t *testing.T) {
	volatilities := []float64{0.02, 0.03, 0.01}
	arrivalRates := []float64{10.0, 20.0, 5.0}
	riskAversion := 0.1
	timeHorizon := 1.0

	spreads, err := MarketMakerOptimalSpread(
		volatilities, arrivalRates, riskAversion, timeHorizon,
	)

	if err != nil {
		t.Fatalf("MarketMakerOptimalSpread failed: %v", err)
	}

	if len(spreads) != 3 {
		t.Fatalf("Expected 3 spreads, got %d", len(spreads))
	}

	for i := 0; i < 3; i++ {
		sigma2 := volatilities[i] * volatilities[i]
		term1 := riskAversion * sigma2 * timeHorizon
		term2 := (2.0 / riskAversion) * math.Log(1.0+riskAversion/arrivalRates[i])
		expected := term1 + term2

		if math.Abs(spreads[i]-expected) > mmEpsilon {
			t.Errorf("Spread[%d] = %f, expected %f", i, spreads[i], expected)
		}

		if spreads[i] <= 0 {
			t.Errorf("Spread must be positive, got %f", spreads[i])
		}
	}
}

func TestMarketMakerOptimalSpreadHighArrivalRate(t *testing.T) {
	volatilities := []float64{0.02}
	arrivalRates := []float64{1000.0}
	riskAversion := 0.1
	timeHorizon := 1.0

	spreads, err := MarketMakerOptimalSpread(
		volatilities, arrivalRates, riskAversion, timeHorizon,
	)

	if err != nil {
		t.Fatalf("MarketMakerOptimalSpread failed: %v", err)
	}

	sigma2 := volatilities[0] * volatilities[0]
	term1 := riskAversion * sigma2 * timeHorizon
	term2 := (2.0 / riskAversion) * math.Log(1.0+riskAversion/arrivalRates[0])
	expected := term1 + term2

	if math.Abs(spreads[0]-expected) > mmEpsilon {
		t.Errorf("Spread = %f, expected %f", spreads[0], expected)
	}
}

func TestMarketMakerConsistency(t *testing.T) {
	n := 100
	midPrices := make([]float64, n)
	inventories := make([]float64, n)
	volatilities := make([]float64, n)
	arrivalRates := make([]float64, n)

	for i := 0; i < n; i++ {
		midPrices[i] = 100.0
		inventories[i] = 0.0
		volatilities[i] = 0.02
		arrivalRates[i] = 10.0
	}

	bidPrices, askPrices, err := MarketMakerQuotes(
		midPrices, inventories, volatilities, arrivalRates, 0.1, 1.0,
	)

	if err != nil {
		t.Fatalf("MarketMakerQuotes failed: %v", err)
	}

	for i := 1; i < n; i++ {
		if math.Abs(bidPrices[i]-bidPrices[0]) > mmEpsilon {
			t.Errorf("Bid prices should be consistent, got %f != %f", bidPrices[i], bidPrices[0])
		}
		if math.Abs(askPrices[i]-askPrices[0]) > mmEpsilon {
			t.Errorf("Ask prices should be consistent, got %f != %f", askPrices[i], askPrices[0])
		}
	}
}

func BenchmarkMarketMakerQuotes100(b *testing.B) {
	n := 100
	midPrices := make([]float64, n)
	inventories := make([]float64, n)
	volatilities := make([]float64, n)
	arrivalRates := make([]float64, n)

	for i := 0; i < n; i++ {
		midPrices[i] = 100.0 + float64(i)*0.1
		inventories[i] = float64(i%10) - 5.0
		volatilities[i] = 0.01 + float64(i%10)*0.001
		arrivalRates[i] = 5.0 + float64(i%20)*0.5
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _, _ = MarketMakerQuotes(midPrices, inventories, volatilities, arrivalRates, 0.1, 1.0)
	}
}

func BenchmarkMarketMakerQuotes1000(b *testing.B) {
	n := 1000
	midPrices := make([]float64, n)
	inventories := make([]float64, n)
	volatilities := make([]float64, n)
	arrivalRates := make([]float64, n)

	for i := 0; i < n; i++ {
		midPrices[i] = 100.0 + float64(i)*0.1
		inventories[i] = float64(i%100) - 50.0
		volatilities[i] = 0.01 + float64(i%100)*0.0001
		arrivalRates[i] = 5.0 + float64(i%50)*0.5
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _, _ = MarketMakerQuotes(midPrices, inventories, volatilities, arrivalRates, 0.1, 1.0)
	}
}

func BenchmarkMarketMakerQuotes5000(b *testing.B) {
	n := 5000
	midPrices := make([]float64, n)
	inventories := make([]float64, n)
	volatilities := make([]float64, n)
	arrivalRates := make([]float64, n)

	for i := 0; i < n; i++ {
		midPrices[i] = 100.0 + float64(i)*0.1
		inventories[i] = float64(i%500) - 250.0
		volatilities[i] = 0.01 + float64(i%100)*0.0001
		arrivalRates[i] = 5.0 + float64(i%50)*0.5
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _, _ = MarketMakerQuotes(midPrices, inventories, volatilities, arrivalRates, 0.1, 1.0)
	}
}
