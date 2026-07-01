/**
 * @file test_market_maker.c
 * @brief Unit tests for market maker strategy
 */

#include "test_framework.h"
#include "strategy/market_maker.h"
#include "platform.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-9

TEST(test_strat_mm_quotes_basic) {
    double mid[] = {100.0, 200.0, 50.0};
    double inventory[] = {10.0, -5.0, 0.0};
    double sigma[] = {0.02, 0.03, 0.01};
    double kappa[] = {10.0, 20.0, 5.0};
    double bid_out[3];
    double ask_out[3];
    double reserve_out[3];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = 3
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, reserve_out, &params);
    ASSERT_EQ(status, FC_OK);

    for (size_t i = 0; i < 3; i++) {
        ASSERT_TRUE(bid_out[i] > 0.0);
        ASSERT_TRUE(ask_out[i] > 0.0);
        ASSERT_TRUE(ask_out[i] > bid_out[i]);
        ASSERT_TRUE(reserve_out[i] > 0.0);
    }

    ASSERT_TRUE(reserve_out[0] < mid[0]);
    ASSERT_TRUE(reserve_out[1] > mid[1]);
    ASSERT_TRUE(fabs(reserve_out[2] - mid[2]) < EPSILON);
}

TEST(test_strat_mm_quotes_positive_inventory) {
    double mid[] = {100.0};
    double inventory[] = {50.0};
    double sigma[] = {0.05};
    double kappa[] = {15.0};
    double bid_out[1];
    double ask_out[1];
    double reserve_out[1];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.2,
        .t_minus_t = 1.0,
        .n = 1
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, reserve_out, &params);
    ASSERT_EQ(status, FC_OK);

    ASSERT_TRUE(reserve_out[0] < mid[0]);
    ASSERT_TRUE(bid_out[0] < reserve_out[0]);
    ASSERT_TRUE(ask_out[0] > reserve_out[0]);
}

TEST(test_strat_mm_quotes_negative_inventory) {
    double mid[] = {100.0};
    double inventory[] = {-50.0};
    double sigma[] = {0.05};
    double kappa[] = {15.0};
    double bid_out[1];
    double ask_out[1];
    double reserve_out[1];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.2,
        .t_minus_t = 1.0,
        .n = 1
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, reserve_out, &params);
    ASSERT_EQ(status, FC_OK);

    ASSERT_TRUE(reserve_out[0] > mid[0]);
    ASSERT_TRUE(bid_out[0] < reserve_out[0]);
    ASSERT_TRUE(ask_out[0] > reserve_out[0]);
}

TEST(test_strat_mm_quotes_zero_inventory) {
    double mid[] = {100.0};
    double inventory[] = {0.0};
    double sigma[] = {0.02};
    double kappa[] = {10.0};
    double bid_out[1];
    double ask_out[1];
    double reserve_out[1];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = 1
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, reserve_out, &params);
    ASSERT_EQ(status, FC_OK);

    FC_TEST_ASSERT_DOUBLE_EQ(reserve_out[0], mid[0], EPSILON);
    double spread = ask_out[0] - bid_out[0];
    FC_TEST_ASSERT_DOUBLE_EQ(bid_out[0], reserve_out[0] - spread / 2.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(ask_out[0], reserve_out[0] + spread / 2.0, EPSILON);
}

TEST(test_strat_mm_quotes_null_reserve) {
    double mid[] = {100.0};
    double inventory[] = {0.0};
    double sigma[] = {0.02};
    double kappa[] = {10.0};
    double bid_out[1];
    double ask_out[1];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = 1
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, NULL, &params);
    ASSERT_EQ(status, FC_OK);
    ASSERT_TRUE(ask_out[0] > bid_out[0]);
}

TEST(test_strat_mm_quotes_large_batch) {
    const size_t n = 1000;
    double* mid = (double*)malloc(n * sizeof(double));
    double* inventory = (double*)malloc(n * sizeof(double));
    double* sigma = (double*)malloc(n * sizeof(double));
    double* kappa = (double*)malloc(n * sizeof(double));
    double* bid_out = (double*)malloc(n * sizeof(double));
    double* ask_out = (double*)malloc(n * sizeof(double));
    double* reserve_out = (double*)malloc(n * sizeof(double));

    ASSERT_NOT_NULL(mid);
    ASSERT_NOT_NULL(inventory);
    ASSERT_NOT_NULL(sigma);
    ASSERT_NOT_NULL(kappa);
    ASSERT_NOT_NULL(bid_out);
    ASSERT_NOT_NULL(ask_out);
    ASSERT_NOT_NULL(reserve_out);

    for (size_t i = 0; i < n; i++) {
        mid[i] = 100.0 + i * 0.5;
        inventory[i] = (double)((int)i - 500);
        sigma[i] = 0.01 + (i % 100) * 0.0001;
        kappa[i] = 5.0 + (i % 50) * 0.5;
    }

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = n
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, reserve_out, &params);
    ASSERT_EQ(status, FC_OK);

    for (size_t i = 0; i < n; i++) {
        ASSERT_TRUE(bid_out[i] > 0.0);
        ASSERT_TRUE(ask_out[i] > 0.0);
        ASSERT_TRUE(ask_out[i] > bid_out[i]);
    }

    free(mid);
    free(inventory);
    free(sigma);
    free(kappa);
    free(bid_out);
    free(ask_out);
    free(reserve_out);
}

TEST(test_strat_mm_reservation_price_only) {
    double mid[] = {100.0, 200.0};
    double inventory[] = {10.0, -5.0};
    double sigma[] = {0.02, 0.03};
    double kappa[] = {10.0, 20.0};
    double reserve_out[2];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = 2
    };

    fc_status_t status = fc_ex_strat_mm_reservation_price(reserve_out, &params);
    ASSERT_EQ(status, FC_OK);

    double gamma_sigma2_T = params.gamma * params.t_minus_t;
    double expected0 = mid[0] - inventory[0] * gamma_sigma2_T * sigma[0] * sigma[0];
    double expected1 = mid[1] - inventory[1] * gamma_sigma2_T * sigma[1] * sigma[1];

    FC_TEST_ASSERT_DOUBLE_EQ(reserve_out[0], expected0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(reserve_out[1], expected1, EPSILON);
}

TEST(test_strat_mm_optimal_spread_only) {
    double mid[] = {100.0, 200.0};
    double inventory[] = {0.0, 0.0};
    double sigma[] = {0.02, 0.03};
    double kappa[] = {10.0, 20.0};
    double spread_out[2];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = 2
    };

    fc_status_t status = fc_ex_strat_mm_optimal_spread(spread_out, &params);
    ASSERT_EQ(status, FC_OK);

    for (size_t i = 0; i < 2; i++) {
        double sigma2 = sigma[i] * sigma[i];
        double term1 = params.gamma * sigma2 * params.t_minus_t;
        double term2 = (2.0 / params.gamma) * log(1.0 + params.gamma / kappa[i]);
        double expected = term1 + term2;
        FC_TEST_ASSERT_DOUBLE_EQ(spread_out[i], expected, EPSILON);
    }
}

TEST(test_strat_mm_invalid_null_params) {
    double bid_out[1];
    double ask_out[1];

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, NULL, NULL);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_strat_mm_invalid_null_output) {
    double mid[] = {100.0};
    double inventory[] = {0.0};
    double sigma[] = {0.02};
    double kappa[] = {10.0};

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = 1
    };

    double bid_out[1];
    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, NULL, NULL, &params);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);

    double ask_out[1];
    status = fc_ex_strat_mm_quotes(NULL, ask_out, NULL, &params);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_strat_mm_invalid_null_inputs) {
    double bid_out[1];
    double ask_out[1];
    double inventory[] = {0.0};
    double sigma[] = {0.02};
    double kappa[] = {10.0};

    fc_ex_strat_mm_params_t params = {
        .mid = NULL,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = 1
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, NULL, &params);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_strat_mm_invalid_zero_n) {
    double mid[] = {100.0};
    double inventory[] = {0.0};
    double sigma[] = {0.02};
    double kappa[] = {10.0};
    double bid_out[1];
    double ask_out[1];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = 1.0,
        .n = 0
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, NULL, &params);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_strat_mm_invalid_negative_gamma) {
    double mid[] = {100.0};
    double inventory[] = {0.0};
    double sigma[] = {0.02};
    double kappa[] = {10.0};
    double bid_out[1];
    double ask_out[1];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = -0.1,
        .t_minus_t = 1.0,
        .n = 1
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, NULL, &params);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

TEST(test_strat_mm_invalid_negative_time_horizon) {
    double mid[] = {100.0};
    double inventory[] = {0.0};
    double sigma[] = {0.02};
    double kappa[] = {10.0};
    double bid_out[1];
    double ask_out[1];

    fc_ex_strat_mm_params_t params = {
        .mid = mid,
        .inventory = inventory,
        .sigma = sigma,
        .kappa = kappa,
        .gamma = 0.1,
        .t_minus_t = -1.0,
        .n = 1
    };

    fc_status_t status = fc_ex_strat_mm_quotes(bid_out, ask_out, NULL, &params);
    ASSERT_EQ(status, FC_ERR_INVALID_ARG);
}

void register_market_maker_tests(void) {
    RUN_TEST(test_strat_mm_quotes_basic);
    RUN_TEST(test_strat_mm_quotes_positive_inventory);
    RUN_TEST(test_strat_mm_quotes_negative_inventory);
    RUN_TEST(test_strat_mm_quotes_zero_inventory);
    RUN_TEST(test_strat_mm_quotes_null_reserve);
    RUN_TEST(test_strat_mm_quotes_large_batch);
    RUN_TEST(test_strat_mm_reservation_price_only);
    RUN_TEST(test_strat_mm_optimal_spread_only);
    RUN_TEST(test_strat_mm_invalid_null_params);
    RUN_TEST(test_strat_mm_invalid_null_output);
    RUN_TEST(test_strat_mm_invalid_null_inputs);
    RUN_TEST(test_strat_mm_invalid_zero_n);
    RUN_TEST(test_strat_mm_invalid_negative_gamma);
    RUN_TEST(test_strat_mm_invalid_negative_time_horizon);
}
