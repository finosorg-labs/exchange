/**
 * @file bench_factor_risk.c
 * @brief Performance benchmarks for factor risk decomposition
 */

#include "bench_framework.h"
#include "risk/factor_risk.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define ALIGN_SIZE(size) (((size) + 63) / 64 * 64)

/* Benchmark data structures */
typedef struct {
    double* portfolio_exposures;
    const double* weights;
    const double* factor_exposures;
    size_t n_assets;
    size_t n_factors;
} bench_portfolio_exposures_data_t;

typedef struct {
    double* factor_risk;
    const double* portfolio_exposures;
    const double* factor_covariance;
    size_t n_factors;
    double* work_buffer;
} bench_factor_risk_data_t;

typedef struct {
    double* specific_risk;
    const double* weights;
    const double* specific_variance;
    size_t n_assets;
} bench_specific_risk_data_t;

typedef struct {
    double* factor_risk;
    double* specific_risk;
    double* total_risk;
    double* portfolio_exposures;
    const double* weights;
    const double* factor_exposures;
    const double* factor_covariance;
    const double* specific_variance;
    size_t n_assets;
    size_t n_factors;
    double* work_buffer;
} bench_decomposition_data_t;

/* Benchmark functions */
static void bench_portfolio_exposures_fn(void* user_data) {
    bench_portfolio_exposures_data_t* data = (bench_portfolio_exposures_data_t*) user_data;
    fc_ex_risk_portfolio_factor_exposures(
        data->portfolio_exposures,
        data->weights,
        data->factor_exposures,
        data->n_assets,
        data->n_factors
    );
}

static void bench_decomposition_fn(void* user_data) {
    bench_decomposition_data_t* data = (bench_decomposition_data_t*) user_data;
    fc_ex_risk_factor_decomposition(
        data->factor_risk,
        data->specific_risk,
        data->total_risk,
        data->portfolio_exposures,
        data->weights,
        data->factor_exposures,
        data->factor_covariance,
        data->specific_variance,
        data->n_assets,
        data->n_factors,
        data->work_buffer
    );
}

/* Benchmark runner for portfolio exposures */
static void bench_portfolio_exposures(size_t n_assets, size_t n_factors, const char* name) {
    size_t weights_size   = ALIGN_SIZE(n_assets * sizeof(double));
    size_t exposures_size = ALIGN_SIZE(n_assets * n_factors * sizeof(double));
    size_t portfolio_size = ALIGN_SIZE(n_factors * sizeof(double));

    double* weights             = aligned_alloc(64, weights_size);
    double* factor_exposures    = aligned_alloc(64, exposures_size);
    double* portfolio_exposures = aligned_alloc(64, portfolio_size);

    if (!weights || !factor_exposures || !portfolio_exposures) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (size_t i = 0; i < n_assets; i++) {
        weights[i] = 1.0 / (double) n_assets;
        for (size_t k = 0; k < n_factors; k++) {
            factor_exposures[i * n_factors + k] = 0.5 + 0.5 * sin((double) (i + k));
        }
    }

    bench_portfolio_exposures_data_t data = {
        .portfolio_exposures = portfolio_exposures,
        .weights             = weights,
        .factor_exposures    = factor_exposures,
        .n_assets            = n_assets,
        .n_factors           = n_factors
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = exposures_size * sizeof(double);
    fc_bench_result_t result;

    fc_bench_run(&config, bench_portfolio_exposures_fn, &data, &result);
    fc_bench_result_print(&result);

    free(weights);
    free(factor_exposures);
    free(portfolio_exposures);
}

/* Benchmark runner for complete decomposition */
static void bench_decomposition(size_t n_assets, size_t n_factors, const char* name) {
    size_t weights_size    = ALIGN_SIZE(n_assets * sizeof(double));
    size_t exposures_size  = ALIGN_SIZE(n_assets * n_factors * sizeof(double));
    size_t covariance_size = ALIGN_SIZE(n_factors * n_factors * sizeof(double));
    size_t variance_size   = ALIGN_SIZE(n_assets * sizeof(double));
    size_t portfolio_size  = ALIGN_SIZE(n_factors * sizeof(double));
    size_t work_size       = ALIGN_SIZE(n_factors * sizeof(double));

    double* weights             = aligned_alloc(64, weights_size);
    double* factor_exposures    = aligned_alloc(64, exposures_size);
    double* factor_covariance   = aligned_alloc(64, covariance_size);
    double* specific_variance   = aligned_alloc(64, variance_size);
    double* portfolio_exposures = aligned_alloc(64, portfolio_size);
    double* work_buffer         = aligned_alloc(64, work_size);
    double factor_risk, specific_risk, total_risk;

    if (!weights || !factor_exposures || !factor_covariance || !specific_variance ||
        !portfolio_exposures || !work_buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (size_t i = 0; i < n_assets; i++) {
        weights[i]           = 1.0 / (double) n_assets;
        specific_variance[i] = 0.01 + 0.05 * ((double) i / (double) n_assets);
        for (size_t k = 0; k < n_factors; k++) {
            factor_exposures[i * n_factors + k] = 0.5 + 0.5 * sin((double) (i + k));
        }
    }

    for (size_t k = 0; k < n_factors; k++) {
        for (size_t l = 0; l < n_factors; l++) {
            if (k == l) {
                factor_covariance[k * n_factors + l] = 0.04;
            } else {
                factor_covariance[k * n_factors + l] = 0.005 * cos((double) (k + l));
            }
        }
    }

    bench_decomposition_data_t data = {
        .factor_risk         = &factor_risk,
        .specific_risk       = &specific_risk,
        .total_risk          = &total_risk,
        .portfolio_exposures = portfolio_exposures,
        .weights             = weights,
        .factor_exposures    = factor_exposures,
        .factor_covariance   = factor_covariance,
        .specific_variance   = specific_variance,
        .n_assets            = n_assets,
        .n_factors           = n_factors,
        .work_buffer         = work_buffer
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = exposures_size * sizeof(double);
    fc_bench_result_t result;

    fc_bench_run(&config, bench_decomposition_fn, &data, &result);
    fc_bench_result_print(&result);

    free(weights);
    free(factor_exposures);
    free(factor_covariance);
    free(specific_variance);
    free(portfolio_exposures);
    free(work_buffer);
}

void bench_factor_risk_run(void) {
    printf("\n=== Factor Risk Decomposition Benchmarks ===\n\n");

    printf("--- Portfolio Factor Exposures ---\n");
    bench_portfolio_exposures(50, 5, "portfolio_exposures_50x5");
    bench_portfolio_exposures(100, 10, "portfolio_exposures_100x10");
    bench_portfolio_exposures(500, 30, "portfolio_exposures_500x30");
    bench_portfolio_exposures(1000, 50, "portfolio_exposures_1000x50");

    printf("\n--- Complete Factor Decomposition ---\n");
    bench_decomposition(50, 5, "decomposition_50x5");
    bench_decomposition(100, 10, "decomposition_100x10");
    bench_decomposition(500, 30, "decomposition_500x30");
    bench_decomposition(1000, 50, "decomposition_1000x50");

    printf("\n--- Large Portfolio Benchmarks ---\n");
    bench_decomposition(2000, 50, "decomposition_2000x50");
}
