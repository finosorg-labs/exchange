/**
 * @file test_market_maker_quotes.c
 * @brief Unit tests for market maker quote calculation
 */

#include "market_maker_quotes.h"
#include "platform.h"
#include "test_framework.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-9

TEST(test_reservation_price_basic) {
    double mid_prices[]   = {100.0, 200.0, 50.0};
    double inventories[]  = {10.0, -5.0, 0.0};
    double volatilities[] = {0.02, 0.03, 0.01};
    double risk_aversion  = 0.1;
    double time_horizon   = 1.0;
    double reservation_prices[3];

    fc_status_t status = fc_market_maker_reservation_price(
        reservation_prices, mid_prices, inventories, volatilities, risk_aversion, time_horizon, 3
    );

    ASSERT_EQ(status, FC_OK);

    double gamma_sigma2_T = risk_aversion * time_horizon;
    double expected0      = 100.0 - 10.0 * gamma_sigma2_T * 0.02 * 0.02;
    double expected1      = 200.0 - (-5.0) * gamma_sigma2_T * 0.03 * 0.03;
    double expected2      = 50.0 - 0.0 * gamma_sigma2_T * 0.01 * 0.01;

    FC_TEST_ASSERT_DOUBLE_EQ(reservation_prices[0], expected0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(reservation_prices[1], expected1, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(reservation_prices[2], expected2, EPSILON);
}

TEST(test_reservation_price_positive_inventory) {
    double mid_prices[]   = {100.0};
    double inventories[]  = {100.0};
    double volatilities[] = {0.05};
    double risk_aversion  = 0.5;
    double time_horizon   = 2.0;
    double reservation_prices[1];

    fc_status_t status = fc_market_maker_reservation_price(
        reservation_prices, mid_prices, inventories, volatilities, risk_aversion, time_horizon, 1
    );

    ASSERT_EQ(status, FC_OK);

    double gamma_sigma2_T = risk_aversion * time_horizon;
    double expected       = 100.0 - 100.0 * gamma_sigma2_T * 0.05 * 0.05;
    FC_TEST_ASSERT_DOUBLE_EQ(reservation_prices[0], expected, EPSILON);
    ASSERT_TRUE(reservation_prices[0] < 100.0);
}

TEST(test_reservation_price_negative_inventory) {
    double mid_prices[]   = {100.0};
    double inventories[]  = {-100.0};
    double volatilities[] = {0.05};
    double risk_aversion  = 0.5;
    double time_horizon   = 2.0;
    double reservation_prices[1];

    fc_status_t status = fc_market_maker_reservation_price(
        reservation_prices, mid_prices, inventories, volatilities, risk_aversion, time_horizon, 1
    );

    ASSERT_EQ(status, FC_OK);

    double gamma_sigma2_T = risk_aversion * time_horizon;
    double expected       = 100.0 - (-100.0) * gamma_sigma2_T * 0.05 * 0.05;
    FC_TEST_ASSERT_DOUBLE_EQ(reservation_prices[0], expected, EPSILON);
    ASSERT_TRUE(reservation_prices[0] > 100.0);
}

TEST(test_optimal_spread_basic) {
    double volatilities[]  = {0.02, 0.03, 0.01};
    double arrival_rates[] = {10.0, 20.0, 5.0};
    double risk_aversion   = 0.1;
    double time_horizon    = 1.0;
    double spreads[3];

    fc_status_t status = fc_market_maker_optimal_spread(
        spreads, volatilities, arrival_rates, risk_aversion, time_horizon, 3
    );

    ASSERT_EQ(status, FC_OK);

    for (int i = 0; i < 3; i++) {
        double sigma2   = volatilities[i] * volatilities[i];
        double term1    = risk_aversion * sigma2 * time_horizon;
        double term2    = (2.0 / risk_aversion) * log(1.0 + risk_aversion / arrival_rates[i]);
        double expected = term1 + term2;
        FC_TEST_ASSERT_DOUBLE_EQ(spreads[i], expected, EPSILON);
        ASSERT_TRUE(spreads[i] > 0.0);
    }
}

TEST(test_optimal_spread_high_arrival_rate) {
    double volatilities[]  = {0.02};
    double arrival_rates[] = {1000.0};
    double risk_aversion   = 0.1;
    double time_horizon    = 1.0;
    double spreads[1];

    fc_status_t status = fc_market_maker_optimal_spread(
        spreads, volatilities, arrival_rates, risk_aversion, time_horizon, 1
    );

    ASSERT_EQ(status, FC_OK);

    double sigma2   = volatilities[0] * volatilities[0];
    double term1    = risk_aversion * sigma2 * time_horizon;
    double term2    = (2.0 / risk_aversion) * log(1.0 + risk_aversion / arrival_rates[0]);
    double expected = term1 + term2;
    FC_TEST_ASSERT_DOUBLE_EQ(spreads[0], expected, EPSILON);
}

TEST(test_market_maker_quotes_basic) {
    double mid_prices[]    = {100.0, 200.0};
    double inventories[]   = {0.0, 0.0};
    double volatilities[]  = {0.02, 0.03};
    double arrival_rates[] = {10.0, 20.0};
    double risk_aversion   = 0.1;
    double time_horizon    = 1.0;
    double bid_prices[2];
    double ask_prices[2];

    fc_status_t status = fc_market_maker_quotes(
        bid_prices,
        ask_prices,
        mid_prices,
        inventories,
        volatilities,
        arrival_rates,
        risk_aversion,
        time_horizon,
        2
    );

    ASSERT_EQ(status, FC_OK);

    for (int i = 0; i < 2; i++) {
        ASSERT_TRUE(bid_prices[i] < mid_prices[i]);
        ASSERT_TRUE(ask_prices[i] > mid_prices[i]);
        ASSERT_TRUE(ask_prices[i] > bid_prices[i]);

        double mid_from_quotes = (bid_prices[i] + ask_prices[i]) * 0.5;
        FC_TEST_ASSERT_DOUBLE_EQ(mid_from_quotes, mid_prices[i], EPSILON);
    }
}

TEST(test_market_maker_quotes_with_inventory) {
    double mid_prices[]    = {100.0};
    double inventories[]   = {50.0};
    double volatilities[]  = {0.02};
    double arrival_rates[] = {10.0};
    double risk_aversion   = 0.1;
    double time_horizon    = 1.0;
    double bid_prices[1];
    double ask_prices[1];

    fc_status_t status = fc_market_maker_quotes(
        bid_prices,
        ask_prices,
        mid_prices,
        inventories,
        volatilities,
        arrival_rates,
        risk_aversion,
        time_horizon,
        1
    );

    ASSERT_EQ(status, FC_OK);

    double reservation_price = mid_prices[0] - inventories[0] * risk_aversion * time_horizon *
                                                   volatilities[0] * volatilities[0];
    ASSERT_TRUE(reservation_price < mid_prices[0]);
    ASSERT_TRUE(bid_prices[0] < reservation_price);
    ASSERT_TRUE(ask_prices[0] > reservation_price);
}

TEST(test_market_maker_quotes_negative_inventory) {
    double mid_prices[]    = {100.0};
    double inventories[]   = {-50.0};
    double volatilities[]  = {0.02};
    double arrival_rates[] = {10.0};
    double risk_aversion   = 0.1;
    double time_horizon    = 1.0;
    double bid_prices[1];
    double ask_prices[1];

    fc_status_t status = fc_market_maker_quotes(
        bid_prices,
        ask_prices,
        mid_prices,
        inventories,
        volatilities,
        arrival_rates,
        risk_aversion,
        time_horizon,
        1
    );

    ASSERT_EQ(status, FC_OK);

    double reservation_price = mid_prices[0] - inventories[0] * risk_aversion * time_horizon *
                                                   volatilities[0] * volatilities[0];
    ASSERT_TRUE(reservation_price > mid_prices[0]);
    ASSERT_TRUE(bid_prices[0] < reservation_price);
    ASSERT_TRUE(ask_prices[0] > reservation_price);
}

TEST(test_market_maker_quotes_batch_1000) {
    const size_t n        = 1000;
    double* mid_prices    = (double*) malloc(n * sizeof(double));
    double* inventories   = (double*) malloc(n * sizeof(double));
    double* volatilities  = (double*) malloc(n * sizeof(double));
    double* arrival_rates = (double*) malloc(n * sizeof(double));
    double* bid_prices    = (double*) malloc(n * sizeof(double));
    double* ask_prices    = (double*) malloc(n * sizeof(double));

    if (!mid_prices || !inventories || !volatilities || !arrival_rates || !bid_prices ||
        !ask_prices) {
        free(mid_prices);
        free(inventories);
        free(volatilities);
        free(arrival_rates);
        free(bid_prices);
        free(ask_prices);
        ASSERT_NOT_NULL(NULL);
    }

    for (size_t i = 0; i < n; i++) {
        mid_prices[i]    = 100.0 + i * 0.1;
        inventories[i]   = (i % 2 == 0) ? 10.0 : -10.0;
        volatilities[i]  = 0.01 + (i % 100) * 0.0001;
        arrival_rates[i] = 5.0 + (i % 50) * 0.5;
    }

    double risk_aversion = 0.1;
    double time_horizon  = 1.0;

    fc_status_t status = fc_market_maker_quotes(
        bid_prices,
        ask_prices,
        mid_prices,
        inventories,
        volatilities,
        arrival_rates,
        risk_aversion,
        time_horizon,
        n
    );

    ASSERT_EQ(status, FC_OK);

    for (size_t i = 0; i < n; i++) {
        ASSERT_TRUE(bid_prices[i] > 0.0);
        ASSERT_TRUE(ask_prices[i] > 0.0);
        ASSERT_TRUE(ask_prices[i] > bid_prices[i]);
    }

    free(mid_prices);
    free(inventories);
    free(volatilities);
    free(arrival_rates);
    free(bid_prices);
    free(ask_prices);
}

TEST(test_market_maker_quotes_invalid_args) {
    double mid_prices[]    = {100.0};
    double inventories[]   = {0.0};
    double volatilities[]  = {0.02};
    double arrival_rates[] = {10.0};
    double bid_prices[1];
    double ask_prices[1];

    fc_status_t status = fc_market_maker_quotes(
        NULL, ask_prices, mid_prices, inventories, volatilities, arrival_rates, 0.1, 1.0, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_market_maker_quotes(
        bid_prices, ask_prices, mid_prices, inventories, volatilities, arrival_rates, -0.1, 1.0, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_market_maker_quotes(
        bid_prices, ask_prices, mid_prices, inventories, volatilities, arrival_rates, 0.1, -1.0, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_market_maker_quotes_empty) {
    double bid_prices[1];
    double ask_prices[1];
    double mid_prices[]    = {100.0};
    double inventories[]   = {0.0};
    double volatilities[]  = {0.02};
    double arrival_rates[] = {10.0};

    fc_status_t status = fc_market_maker_quotes(
        bid_prices, ask_prices, mid_prices, inventories, volatilities, arrival_rates, 0.1, 1.0, 0
    );

    ASSERT_EQ(status, FC_OK);
}

TEST(test_reservation_price_invalid_args) {
    double mid_prices[]   = {100.0};
    double inventories[]  = {0.0};
    double volatilities[] = {0.02};
    double reservation_prices[1];

    fc_status_t status =
        fc_market_maker_reservation_price(NULL, mid_prices, inventories, volatilities, 0.1, 1.0, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_market_maker_reservation_price(
        reservation_prices, mid_prices, inventories, volatilities, 0.0, 1.0, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_market_maker_reservation_price(
        reservation_prices, mid_prices, inventories, volatilities, 0.1, 0.0, 1
    );
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_optimal_spread_invalid_args) {
    double volatilities[]  = {0.02};
    double arrival_rates[] = {10.0};
    double spreads[1];

    fc_status_t status =
        fc_market_maker_optimal_spread(NULL, volatilities, arrival_rates, 0.1, 1.0, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_market_maker_optimal_spread(spreads, volatilities, arrival_rates, 0.0, 1.0, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    status = fc_market_maker_optimal_spread(spreads, volatilities, arrival_rates, 0.1, 0.0, 1);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_reservation_price_zero_volatility) {
    double mid_prices[]   = {100.0};
    double inventories[]  = {10.0};
    double volatilities[] = {0.0};
    double reservation_prices[1];

    fc_status_t status = fc_market_maker_reservation_price(
        reservation_prices, mid_prices, inventories, volatilities, 0.1, 1.0, 1
    );

    ASSERT_EQ(status, FC_OK);
    FC_TEST_ASSERT_DOUBLE_EQ(reservation_prices[0], mid_prices[0], EPSILON);
}

TEST(test_optimal_spread_varying_parameters) {
    double volatilities[]   = {0.01, 0.05, 0.10};
    double arrival_rates[]  = {1.0, 10.0, 100.0};
    double risk_aversions[] = {0.01, 0.1, 1.0};
    double time_horizons[]  = {0.5, 1.0, 2.0};

    for (int v = 0; v < 3; v++) {
        for (int a = 0; a < 3; a++) {
            for (int r = 0; r < 3; r++) {
                for (int t = 0; t < 3; t++) {
                    double vol    = volatilities[v];
                    double lambda = arrival_rates[a];
                    double gamma  = risk_aversions[r];
                    double T      = time_horizons[t];
                    double spreads[1];
                    double vols[1]    = {vol};
                    double lambdas[1] = {lambda};

                    fc_status_t status =
                        fc_market_maker_optimal_spread(spreads, vols, lambdas, gamma, T, 1);

                    ASSERT_EQ(status, FC_OK);
                    ASSERT_TRUE(spreads[0] > 0.0);

                    double sigma2   = vol * vol;
                    double term1    = gamma * sigma2 * T;
                    double term2    = (2.0 / gamma) * log(1.0 + gamma / lambda);
                    double expected = term1 + term2;
                    FC_TEST_ASSERT_DOUBLE_EQ(spreads[0], expected, EPSILON);
                }
            }
        }
    }
}

TEST(test_market_maker_quotes_large_inventory) {
    double mid_prices[]    = {100.0};
    double inventories[]   = {1000.0};
    double volatilities[]  = {0.05};
    double arrival_rates[] = {10.0};
    double risk_aversion   = 0.5;
    double time_horizon    = 1.0;
    double bid_prices[1];
    double ask_prices[1];

    fc_status_t status = fc_market_maker_quotes(
        bid_prices,
        ask_prices,
        mid_prices,
        inventories,
        volatilities,
        arrival_rates,
        risk_aversion,
        time_horizon,
        1
    );

    ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(ask_prices[0] > bid_prices[0]);

    double reservation_price = mid_prices[0] - inventories[0] * risk_aversion * time_horizon *
                                                   volatilities[0] * volatilities[0];
    ASSERT_TRUE(reservation_price < mid_prices[0]);
}

TEST(test_market_maker_quotes_consistency) {
    const size_t n        = 100;
    double* mid_prices    = (double*) malloc(n * sizeof(double));
    double* inventories   = (double*) malloc(n * sizeof(double));
    double* volatilities  = (double*) malloc(n * sizeof(double));
    double* arrival_rates = (double*) malloc(n * sizeof(double));
    double* bid_prices    = (double*) malloc(n * sizeof(double));
    double* ask_prices    = (double*) malloc(n * sizeof(double));

    for (size_t i = 0; i < n; i++) {
        mid_prices[i]    = 100.0;
        inventories[i]   = 0.0;
        volatilities[i]  = 0.02;
        arrival_rates[i] = 10.0;
    }

    fc_status_t status = fc_market_maker_quotes(
        bid_prices, ask_prices, mid_prices, inventories, volatilities, arrival_rates, 0.1, 1.0, n
    );

    ASSERT_EQ(status, FC_OK);

    for (size_t i = 1; i < n; i++) {
        FC_TEST_ASSERT_DOUBLE_EQ(bid_prices[i], bid_prices[0], EPSILON);
        FC_TEST_ASSERT_DOUBLE_EQ(ask_prices[i], ask_prices[0], EPSILON);
    }

    free(mid_prices);
    free(inventories);
    free(volatilities);
    free(arrival_rates);
    free(bid_prices);
    free(ask_prices);
}

void register_market_maker_quotes_tests(void) {
    RUN_TEST(test_reservation_price_basic);
    RUN_TEST(test_reservation_price_positive_inventory);
    RUN_TEST(test_reservation_price_negative_inventory);
    RUN_TEST(test_optimal_spread_basic);
    RUN_TEST(test_optimal_spread_high_arrival_rate);
    RUN_TEST(test_market_maker_quotes_basic);
    RUN_TEST(test_market_maker_quotes_with_inventory);
    RUN_TEST(test_market_maker_quotes_negative_inventory);
    RUN_TEST(test_market_maker_quotes_batch_1000);
    RUN_TEST(test_market_maker_quotes_invalid_args);
    RUN_TEST(test_market_maker_quotes_empty);
    RUN_TEST(test_reservation_price_invalid_args);
    RUN_TEST(test_optimal_spread_invalid_args);
    RUN_TEST(test_reservation_price_zero_volatility);
    RUN_TEST(test_optimal_spread_varying_parameters);
    RUN_TEST(test_market_maker_quotes_large_inventory);
    RUN_TEST(test_market_maker_quotes_consistency);
}
