package exchange

/*
#include "market_maker_quotes.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"unsafe"
)

// MarketMakerQuotes calculates optimal market maker bid/ask quotes using the Avellaneda-Stoikov model.
//
// The Avellaneda-Stoikov model computes optimal quotes for market makers by balancing inventory risk
// and order flow. It accounts for:
//   - Current inventory position (positive = long, negative = short)
//   - Market volatility (price variance)
//   - Order arrival rate (liquidity measure)
//   - Risk aversion parameter (trader's risk preference)
//   - Time horizon (remaining trading period)
//
// Model formulas:
//   reservation_price = mid - inventory × γ × σ² × T
//   spread = γ × σ² × T + (2/γ) × ln(1 + γ/λ)
//   bid = reservation_price - spread/2
//   ask = reservation_price + spread/2
//
// Parameters:
//   - midPrices: Current mid prices for each symbol
//   - inventories: Current inventory positions (positive = long, negative = short)
//   - volatilities: Price volatilities (σ) for each symbol
//   - arrivalRates: Order arrival rates (λ) for each symbol
//   - riskAversion: Risk aversion coefficient (γ, must be positive)
//   - timeHorizon: Time horizon (T, must be positive, same units as volatilities)
//
// Returns:
//   - bidPrices: Optimal bid prices
//   - askPrices: Optimal ask prices
//   - error: nil on success, error otherwise
//
// Performance: Optimized for batch processing of 1000+ symbols to amortize cgo overhead.
// Single symbol latency: <1μs, Batch of 1000: <500μs
func MarketMakerQuotes(
	midPrices []float64,
	inventories []float64,
	volatilities []float64,
	arrivalRates []float64,
	riskAversion float64,
	timeHorizon float64,
) (bidPrices []float64, askPrices []float64, err error) {
	n := len(midPrices)
	if n == 0 {
		return []float64{}, []float64{}, nil
	}

	if len(inventories) != n || len(volatilities) != n || len(arrivalRates) != n {
		return nil, nil, fmt.Errorf("all input slices must have the same length")
	}

	if riskAversion <= 0 {
		return nil, nil, fmt.Errorf("risk aversion must be positive, got %f", riskAversion)
	}

	if timeHorizon <= 0 {
		return nil, nil, fmt.Errorf("time horizon must be positive, got %f", timeHorizon)
	}

	bidPrices = make([]float64, n)
	askPrices = make([]float64, n)

	status := C.fc_market_maker_quotes(
		(*C.double)(unsafe.Pointer(&bidPrices[0])),
		(*C.double)(unsafe.Pointer(&askPrices[0])),
		(*C.double)(unsafe.Pointer(&midPrices[0])),
		(*C.double)(unsafe.Pointer(&inventories[0])),
		(*C.double)(unsafe.Pointer(&volatilities[0])),
		(*C.double)(unsafe.Pointer(&arrivalRates[0])),
		C.double(riskAversion),
		C.double(timeHorizon),
		C.size_t(n),
	)

	if status != C.FC_OK {
		return nil, nil, fmt.Errorf("fc_market_maker_quotes failed with status %d", status)
	}

	return bidPrices, askPrices, nil
}

// MarketMakerReservationPrice calculates the reservation price component of the Avellaneda-Stoikov model.
//
// The reservation price is the mid price adjusted for inventory risk. A market maker with positive
// inventory (long) will have a reservation price below the mid, incentivizing selling. A market maker
// with negative inventory (short) will have a reservation price above the mid, incentivizing buying.
//
// Formula: reservation_price = mid - inventory × γ × σ² × T
//
// Parameters:
//   - midPrices: Current mid prices
//   - inventories: Current inventory positions
//   - volatilities: Price volatilities (σ)
//   - riskAversion: Risk aversion coefficient (γ)
//   - timeHorizon: Time horizon (T)
//
// Returns:
//   - reservationPrices: Inventory-adjusted reservation prices
//   - error: nil on success, error otherwise
func MarketMakerReservationPrice(
	midPrices []float64,
	inventories []float64,
	volatilities []float64,
	riskAversion float64,
	timeHorizon float64,
) (reservationPrices []float64, err error) {
	n := len(midPrices)
	if n == 0 {
		return []float64{}, nil
	}

	if len(inventories) != n || len(volatilities) != n {
		return nil, fmt.Errorf("all input slices must have the same length")
	}

	if riskAversion <= 0 {
		return nil, fmt.Errorf("risk aversion must be positive, got %f", riskAversion)
	}

	if timeHorizon <= 0 {
		return nil, fmt.Errorf("time horizon must be positive, got %f", timeHorizon)
	}

	reservationPrices = make([]float64, n)

	status := C.fc_market_maker_reservation_price(
		(*C.double)(unsafe.Pointer(&reservationPrices[0])),
		(*C.double)(unsafe.Pointer(&midPrices[0])),
		(*C.double)(unsafe.Pointer(&inventories[0])),
		(*C.double)(unsafe.Pointer(&volatilities[0])),
		C.double(riskAversion),
		C.double(timeHorizon),
		C.size_t(n),
	)

	if status != C.FC_OK {
		return nil, fmt.Errorf("fc_market_maker_reservation_price failed with status %d", status)
	}

	return reservationPrices, nil
}

// MarketMakerOptimalSpread calculates the optimal bid-ask spread component of the Avellaneda-Stoikov model.
//
// The optimal spread balances the tradeoff between capturing spread revenue and adverse selection risk.
// Higher volatility increases the spread (more risk). Higher order arrival rates decrease the spread
// (more liquidity, less adverse selection).
//
// Formula: spread = γ × σ² × T + (2/γ) × ln(1 + γ/λ)
//
// Parameters:
//   - volatilities: Price volatilities (σ)
//   - arrivalRates: Order arrival rates (λ)
//   - riskAversion: Risk aversion coefficient (γ)
//   - timeHorizon: Time horizon (T)
//
// Returns:
//   - spreads: Optimal bid-ask spreads
//   - error: nil on success, error otherwise
func MarketMakerOptimalSpread(
	volatilities []float64,
	arrivalRates []float64,
	riskAversion float64,
	timeHorizon float64,
) (spreads []float64, err error) {
	n := len(volatilities)
	if n == 0 {
		return []float64{}, nil
	}

	if len(arrivalRates) != n {
		return nil, fmt.Errorf("volatilities and arrival rates must have the same length")
	}

	if riskAversion <= 0 {
		return nil, fmt.Errorf("risk aversion must be positive, got %f", riskAversion)
	}

	if timeHorizon <= 0 {
		return nil, fmt.Errorf("time horizon must be positive, got %f", timeHorizon)
	}

	spreads = make([]float64, n)

	status := C.fc_market_maker_optimal_spread(
		(*C.double)(unsafe.Pointer(&spreads[0])),
		(*C.double)(unsafe.Pointer(&volatilities[0])),
		(*C.double)(unsafe.Pointer(&arrivalRates[0])),
		C.double(riskAversion),
		C.double(timeHorizon),
		C.size_t(n),
	)

	if status != C.FC_OK {
		return nil, fmt.Errorf("fc_market_maker_optimal_spread failed with status %d", status)
	}

	return spreads, nil
}
