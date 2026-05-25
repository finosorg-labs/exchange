/**
 * @file test_ticker_simd.c
 * @brief Unit tests for SIMD-optimized ticker batch processing
 */

#include "test_framework.h"
#include "error.h"
#include "simd_detect.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPSILON 1e-10

// External function from ticker_simd.c
void fc_ticker_update_ohlcv_batch_simd(
    double* high,
    double* low,
    double* volume_sum,
    double* amount_sum,
    const double* prices,
    const double* volumes,
    const double* amounts,
    size_t count
);

TEST(test_simd_batch_empty) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    fc_ticker_update_ohlcv_batch_simd(&high, &low, &volume_sum, &amount_sum, NULL, NULL, NULL, 0);

    ASSERT_TRUE(fabs(high - 100.0) < EPSILON);
    ASSERT_TRUE(fabs(low - 100.0) < EPSILON);
    ASSERT_TRUE(fabs(volume_sum - 0.0) < EPSILON);
    ASSERT_TRUE(fabs(amount_sum - 0.0) < EPSILON);
}

TEST(test_simd_batch_single_element) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    double prices[]  = {105.5};
    double volumes[] = {1000.0};
    double amounts[] = {105500.0};

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 1
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 105.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 1000.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 105500.0, EPSILON);
}

TEST(test_simd_batch_two_elements) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    double prices[]  = {105.5, 98.3};
    double volumes[] = {1000.0, 2000.0};
    double amounts[] = {105500.0, 196600.0};

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 2
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 105.5, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 98.3, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 3000.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 302100.0, EPSILON);
}

TEST(test_simd_batch_four_elements) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    double prices[]  = {105.5, 98.3, 110.2, 95.7};
    double volumes[] = {1000.0, 2000.0, 1500.0, 2500.0};
    double amounts[] = {105500.0, 196600.0, 165300.0, 239250.0};

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 4
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 110.2, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 95.7, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 7000.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 706650.0, EPSILON);
}

TEST(test_simd_batch_eight_elements) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    double prices[]  = {105.5, 98.3, 110.2, 95.7, 102.1, 107.8, 99.5, 103.3};
    double volumes[] = {1000.0, 2000.0, 1500.0, 2500.0, 1800.0, 1200.0, 2200.0, 1600.0};
    double amounts[] = {
        105500.0, 196600.0, 165300.0, 239250.0, 183780.0, 129360.0, 218900.0, 165280.0
    };

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 8
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 110.2, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 95.7, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 13800.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 1403970.0, EPSILON);
}

TEST(test_simd_batch_non_aligned_count) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    // 10 elements - tests tail handling for all SIMD levels
    double prices[]  = {105.5, 98.3, 110.2, 95.7, 102.1, 107.8, 99.5, 103.3, 96.2, 108.9};
    double volumes[] = {1000.0, 2000.0, 1500.0, 2500.0, 1800.0, 1200.0, 2200.0, 1600.0, 1900.0, 1400.0};
    double amounts[] = {
        105500.0, 196600.0, 165300.0, 239250.0, 183780.0, 129360.0, 218900.0, 165280.0, 182780.0, 152460.0
    };

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 10
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 110.2, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 95.7, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 17100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 1739210.0, EPSILON);
}

TEST(test_simd_batch_preserve_initial_high) {
    double high       = 150.0; // Higher than all prices
    double low        = 50.0;  // Lower than all prices
    double volume_sum = 100.0;
    double amount_sum = 10000.0;

    double prices[]  = {105.5, 98.3, 110.2, 95.7};
    double volumes[] = {1000.0, 2000.0, 1500.0, 2500.0};
    double amounts[] = {105500.0, 196600.0, 165300.0, 239250.0};

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 4
    );

    // Should preserve initial high/low since they are more extreme
    FC_TEST_ASSERT_DOUBLE_EQ(high, 150.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 50.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 7100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 716650.0, EPSILON);
}

TEST(test_simd_batch_all_same_price) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    double prices[]  = {100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0};
    double volumes[] = {1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0};
    double amounts[] = {100000.0, 100000.0, 100000.0, 100000.0, 100000.0, 100000.0, 100000.0, 100000.0};

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 8
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 8000.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 800000.0, EPSILON);
}

TEST(test_simd_batch_ascending_prices) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    double prices[]  = {100.0, 101.0, 102.0, 103.0, 104.0, 105.0, 106.0, 107.0};
    double volumes[] = {1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0};
    double amounts[] = {100000.0, 101000.0, 102000.0, 103000.0, 104000.0, 105000.0, 106000.0, 107000.0};

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 8
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 107.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 8000.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 828000.0, EPSILON);
}

TEST(test_simd_batch_descending_prices) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    double prices[]  = {107.0, 106.0, 105.0, 104.0, 103.0, 102.0, 101.0, 100.0};
    double volumes[] = {1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0, 1000.0};
    double amounts[] = {107000.0, 106000.0, 105000.0, 104000.0, 103000.0, 102000.0, 101000.0, 100000.0};

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, 8
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 107.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 100.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, 8000.0, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, 828000.0, EPSILON);
}

TEST(test_simd_batch_large_count) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    const size_t count = 1000;
    double* prices     = (double*)malloc(count * sizeof(double));
    double* volumes    = (double*)malloc(count * sizeof(double));
    double* amounts    = (double*)malloc(count * sizeof(double));

    double expected_high = 100.0;
    double expected_low  = 100.0;
    double expected_vol  = 0.0;
    double expected_amt  = 0.0;

    for (size_t i = 0; i < count; i++) {
        prices[i]  = 100.0 + (i % 20) - 10.0; // Range: 90.0 to 110.0
        volumes[i] = 1000.0 + (i % 100);
        amounts[i] = prices[i] * volumes[i];

        if (prices[i] > expected_high)
            expected_high = prices[i];
        if (prices[i] < expected_low)
            expected_low = prices[i];
        expected_vol += volumes[i];
        expected_amt += amounts[i];
    }

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, count
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, expected_high, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, expected_low, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(volume_sum, expected_vol, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(amount_sum, expected_amt, EPSILON);

    free(prices);
    free(volumes);
    free(amounts);
}

TEST(test_simd_batch_precision) {
    double high       = 100.0;
    double low        = 100.0;
    double volume_sum = 0.0;
    double amount_sum = 0.0;

    // Test with small increments to check precision
    const size_t count = 100;
    double* prices     = (double*)malloc(count * sizeof(double));
    double* volumes    = (double*)malloc(count * sizeof(double));
    double* amounts    = (double*)malloc(count * sizeof(double));

    double expected_vol = 0.0;
    double expected_amt = 0.0;

    for (size_t i = 0; i < count; i++) {
        prices[i]  = 100.0 + i * 0.01; // Small increments
        volumes[i] = 10.0 + i * 0.1;
        amounts[i] = prices[i] * volumes[i];

        expected_vol += volumes[i];
        expected_amt += amounts[i];
    }

    fc_ticker_update_ohlcv_batch_simd(
        &high, &low, &volume_sum, &amount_sum, prices, volumes, amounts, count
    );

    FC_TEST_ASSERT_DOUBLE_EQ(high, 100.99, EPSILON);
    FC_TEST_ASSERT_DOUBLE_EQ(low, 100.0, EPSILON);
    ASSERT_TRUE(fabs(volume_sum - expected_vol) < 1e-6);
    ASSERT_TRUE(fabs(amount_sum - expected_amt) < 1e-3);

    free(prices);
    free(volumes);
    free(amounts);
}

TEST(test_simd_detection) {
    fc_simd_level_t level = fc_get_simd_level();

    printf("\n=== SIMD Detection ===\n");
    printf("Detected SIMD level: ");

    switch (level) {
    case FC_SIMD_SCALAR:
        printf("Scalar\n");
        break;
    case FC_SIMD_SSE42:
        printf("SSE4.2\n");
        break;
    case FC_SIMD_AVX2:
        printf("AVX2\n");
        break;
    case FC_SIMD_AVX512:
        printf("AVX-512\n");
        break;
    default:
        printf("Unknown\n");
        break;
    }

    ASSERT_TRUE(level >= FC_SIMD_SCALAR);
}

// Test suite definition
static fc_test_fn ticker_simd_tests[] = {
    test_simd_batch_empty,
    test_simd_batch_single_element,
    test_simd_batch_two_elements,
    test_simd_batch_four_elements,
    test_simd_batch_eight_elements,
    test_simd_batch_non_aligned_count,
    test_simd_batch_preserve_initial_high,
    test_simd_batch_all_same_price,
    test_simd_batch_ascending_prices,
    test_simd_batch_descending_prices,
    test_simd_batch_large_count,
    test_simd_batch_precision,
    test_simd_detection,
};

static fc_test_suite_t ticker_simd_suite = {
    .name = "ticker_simd",
    .description = "SIMD-optimized ticker batch processing tests",
    .tests = ticker_simd_tests,
    .num_tests = sizeof(ticker_simd_tests) / sizeof(ticker_simd_tests[0]),
};

void register_ticker_simd_tests(void) {
    fc_test_register_suite(&ticker_simd_suite);
}
