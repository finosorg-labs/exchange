/**
 * @file test_exchange.c
 * @brief exchange module test entry point
 *
 * This file serves as the main test registration point for the exchange module.
 * Individual test modules are in separate files:
 */

#include "test_framework.h"

/* External test registration functions from sub-modules */
extern void register_ticker_tests(void);
extern void register_ticker_merge_tests(void);
extern void register_order_book_tests(void);
extern void register_market_indicators_tests(void);
extern void register_market_maker_quotes_tests(void);
extern void register_ofi_tests(void);
extern void register_microprice_tests(void);
extern void register_spread_tests(void);
extern void register_vwap_dev_tests(void);
extern void register_kyle_lambda_tests(void);
extern void register_arbitrage_tests(void);
extern void register_feature_tests(void);
extern void register_normalize_tests(void);
extern void register_postprocess_tests(void);
extern void register_alpha_tests(void);
extern void register_market_maker_tests(void);
extern void register_stat_arb_tests(void);
extern void register_latency_arb_tests(void);
extern void register_index_rebal_tests(void);
extern void register_delta_hedge_tests(void);
extern void register_marginal_var_tests(void);
extern void register_factor_risk_tests(void);

/* Entry point for exchange tests */
void register_exchange_tests(void) {
    /* Register all sub-module tests */
    register_ticker_tests();
    register_ticker_merge_tests();
    register_order_book_tests();
    register_market_indicators_tests();
    register_market_maker_quotes_tests();
    register_ofi_tests();
    register_microprice_tests();
    register_spread_tests();
    register_vwap_dev_tests();
    register_kyle_lambda_tests();
    register_arbitrage_tests();
    register_feature_tests();
    register_normalize_tests();
    register_postprocess_tests();
    register_alpha_tests();
    register_market_maker_tests();
    register_stat_arb_tests();
    register_latency_arb_tests();
    register_index_rebal_tests();
    register_delta_hedge_tests();
    register_marginal_var_tests();
    register_factor_risk_tests();
}
