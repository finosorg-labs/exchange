/**
 * @file exchange.h
 * @brief Exchange module public API
 *
 * This header provides the main entry point for the exchange module,
 * which includes market data processing and exchange calculations.
 */

#ifndef FC_EXCHANGE_H
#define FC_EXCHANGE_H

#include "signal/feature.h"
#include "signal/kyle_lambda.h"
#include "signal/microprice.h"
#include "signal/ofi.h"
#include "signal/spread.h"
#include "signal/vwap_dev.h"
#include "strategy/market_maker.h"
#include "ticker.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Module initialization is handled by platform module */

#ifdef __cplusplus
}
#endif

#endif /* FC_EXCHANGE_H */
