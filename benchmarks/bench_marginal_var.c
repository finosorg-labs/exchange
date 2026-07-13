/**
 * @file bench_marginal_var.c
 * @brief Performance benchmarks for marginal VaR calculation
 *
 * Benchmarks include:
 * - Portfolio returns calculation (various sizes)
 * - Marginal VaR correlation method (various sizes)
 * - Component VaR calculation (various sizes)
 * - Marginal VaR perturbation method (small portfolios)
 * - Large portfolio benchmarks (100-1000 assets)
 */

#include "bench_framework.h"
#include "risk/marginal_risk.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define ALIGN_SIZE(size) (((size) + 63) / 64 * 64)

/* Benchmark data structures */
typedef struct {
    double* portfolio_returns;
    const double* returns;
    const double* weights;
    size_t n_assets;
    size_t n_days;
} bench_portfolio_returns_data_t;

typedef struct {
    double* marginal_var;
    const double* returns;
    const double* weights;
    double portfolio_var;
    size_t n_assets;
    size_t n_days;
} bench_marginal_var_data_t;

typedef struct {
    double* component_var;
    const double* marginal_var;
    const double* weights;
    size_t n_assets;
} bench_component_var_data_t;

typedef struct {
    double* marginal_var;
    const double* returns;
    const double* weights;
    double portfolio_var;
    double confidence;
    double epsilon;
    double* work_buffer;
    size_t n_assets;
    size_t n_days;
} bench_perturbation_data_t;

/* Benchmark functions */
static void bench_portfolio_returns_fn(void* user_data) {
    bench_portfolio_returns_data_t* data = (bench_portfolio_returns_data_t*) user_data;
    fc_ex_risk_portfolio_returns(
        data->portfolio_returns, data->returns, data->weights, data->n_assets, data->n_days
    );
}

static void bench_marginal_var_fn(void* user_data) {
    bench_marginal_var_data_t* data = (bench_marginal_var_data_t*) user_data;
    fc_ex_risk_marginal_var_correlation(
        data->marginal_var,
        data->returns,
        data->weights,
        data->portfolio_var,
        data->n_assets,
        data->n_days
    );
}

static void bench_component_var_fn(void* user_data) {
    bench_component_var_data_t* data = (bench_component_var_data_t*) user_data;
    fc_ex_risk_component_var(
        data->component_var, data->marginal_var, data->weights, data->n_assets
    );
}

static void bench_perturbation_fn(void* user_data) {
    bench_perturbation_data_t* data = (bench_perturbation_data_t*) user_data;
    fc_ex_risk_marginal_var_perturbation(
        data->marginal_var,
        data->returns,
        data->weights,
        data->portfolio_var,
        data->confidence,
        data->epsilon,
        data->work_buffer,
        data->n_assets,
        data->n_days
    );
}

/* Benchmark portfolio returns calculation */
static void bench_portfolio_returns(size_t n_assets, size_t n_days, const char* name) {
    size_t returns_size = n_assets * n_days;

    double* returns           = aligned_alloc(64, ALIGN_SIZE(returns_size * sizeof(double)));
    double* weights           = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double* portfolio_returns = aligned_alloc(64, ALIGN_SIZE(n_days * sizeof(double)));

    if (!returns || !weights || !portfolio_returns) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize with realistic returns */
    for (size_t i = 0; i < n_assets; i++) {
        for (size_t t = 0; t < n_days; t++) {
            double common           = sin((double) t * 0.1) * 0.01;
            double specific         = sin((double) (i * t) * 0.05) * 0.005;
            returns[i * n_days + t] = common + specific;
        }
    }

    /* Equal weights */
    for (size_t i = 0; i < n_assets; i++) {
        weights[i] = 1.0 / (double) n_assets;
    }

    bench_portfolio_returns_data_t data = {
        .portfolio_returns = portfolio_returns,
        .returns           = returns,
        .weights           = weights,
        .n_assets          = n_assets,
        .n_days            = n_days
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = returns_size * sizeof(double);
    fc_bench_result_t result;

    fc_bench_run(&config, bench_portfolio_returns_fn, &data, &result);
    fc_bench_result_print(&result);

    free(returns);
    free(weights);
    free(portfolio_returns);
}

/* Benchmark marginal VaR correlation method */
static void bench_marginal_var_correlation(size_t n_assets, size_t n_days, const char* name) {
    size_t returns_size = n_assets * n_days;

    double* returns      = aligned_alloc(64, ALIGN_SIZE(returns_size * sizeof(double)));
    double* weights      = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double* marginal_var = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));

    if (!returns || !weights || !marginal_var) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize with realistic correlated returns */
    for (size_t i = 0; i < n_assets; i++) {
        for (size_t t = 0; t < n_days; t++) {
            double market_factor    = sin((double) t * 0.1) * 0.015;
            double sector_factor    = sin((double) ((i / 10) * t) * 0.08) * 0.008;
            double idiosyncratic    = sin((double) (i * t) * 0.03) * 0.004;
            returns[i * n_days + t] = market_factor + sector_factor + idiosyncratic;
        }
    }

    /* Market-cap weighted */
    double total_weight = 0.0;
    for (size_t i = 0; i < n_assets; i++) {
        weights[i] = 1.0 / (1.0 + (double) i * 0.1);
        total_weight += weights[i];
    }
    for (size_t i = 0; i < n_assets; i++) {
        weights[i] /= total_weight;
    }

    double portfolio_var = 100000.0;

    bench_marginal_var_data_t data = {
        .marginal_var  = marginal_var,
        .returns       = returns,
        .weights       = weights,
        .portfolio_var = portfolio_var,
        .n_assets      = n_assets,
        .n_days        = n_days
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = returns_size * sizeof(double);
    fc_bench_result_t result;

    fc_bench_run(&config, bench_marginal_var_fn, &data, &result);
    fc_bench_result_print(&result);

    free(returns);
    free(weights);
    free(marginal_var);
}

/* Benchmark component VaR calculation */
static void bench_component_var(size_t n_assets, const char* name) {
    double* marginal_var  = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double* weights       = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double* component_var = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));

    if (!marginal_var || !weights || !component_var) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize data */
    for (size_t i = 0; i < n_assets; i++) {
        marginal_var[i] = 1000.0 + (double) i * 100.0;
        weights[i]      = 1.0 / (double) n_assets;
    }

    bench_component_var_data_t data = {
        .component_var = component_var,
        .marginal_var  = marginal_var,
        .weights       = weights,
        .n_assets      = n_assets
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = n_assets * sizeof(double) * 3;
    fc_bench_result_t result;

    fc_bench_run(&config, bench_component_var_fn, &data, &result);
    fc_bench_result_print(&result);

    free(marginal_var);
    free(weights);
    free(component_var);
}

/* Benchmark perturbation method (small portfolios only) */
static void bench_marginal_var_perturbation(size_t n_assets, size_t n_days, const char* name) {
    size_t returns_size = n_assets * n_days;

    double* returns      = aligned_alloc(64, ALIGN_SIZE(returns_size * sizeof(double)));
    double* weights      = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double* marginal_var = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double* work_buffer  = aligned_alloc(64, ALIGN_SIZE(n_days * sizeof(double)));

    if (!returns || !weights || !marginal_var || !work_buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize data */
    for (size_t i = 0; i < n_assets; i++) {
        for (size_t t = 0; t < n_days; t++) {
            returns[i * n_days + t] = sin((double) (i + t) * 0.3) * 0.02;
        }
        weights[i] = 1.0 / (double) n_assets;
    }

    bench_perturbation_data_t data = {
        .marginal_var  = marginal_var,
        .returns       = returns,
        .weights       = weights,
        .portfolio_var = 50000.0,
        .confidence    = 0.95,
        .epsilon       = 0.0001,
        .work_buffer   = work_buffer,
        .n_assets      = n_assets,
        .n_days        = n_days
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = returns_size * sizeof(double);
    config.min_iterations    = 5;
    config.min_time_ms       = 50.0;
    fc_bench_result_t result;

    fc_bench_run(&config, bench_perturbation_fn, &data, &result);
    fc_bench_result_print(&result);

    free(returns);
    free(weights);
    free(marginal_var);
    free(work_buffer);
}

/* Register all marginal VaR benchmarks */
void bench_marginal_var_run(void) {
    printf("\n=================================================================\n");
    printf("Marginal VaR Performance Benchmarks\n");
    printf("=================================================================\n\n");

    fc_bench_print_header();

    printf("\n--- Portfolio Returns Calculation ---\n");
    bench_portfolio_returns(50, 252, "portfolio_returns_50x252");
    bench_portfolio_returns(100, 252, "portfolio_returns_100x252");
    bench_portfolio_returns(500, 252, "portfolio_returns_500x252");
    bench_portfolio_returns(1000, 252, "portfolio_returns_1000x252");

    printf("\n--- Marginal VaR (Correlation Method) ---\n");
    bench_marginal_var_correlation(50, 252, "marginal_var_corr_50x252");
    bench_marginal_var_correlation(100, 252, "marginal_var_corr_100x252");
    bench_marginal_var_correlation(250, 252, "marginal_var_corr_250x252");
    bench_marginal_var_correlation(500, 252, "marginal_var_corr_500x252");
    bench_marginal_var_correlation(1000, 252, "marginal_var_corr_1000x252");

    printf("\n--- Component VaR Calculation ---\n");
    bench_component_var(100, "component_var_100");
    bench_component_var(500, "component_var_500");
    bench_component_var(1000, "component_var_1000");
    bench_component_var(5000, "component_var_5000");

    printf("\n--- Marginal VaR (Perturbation Method) ---\n");
    printf("Note: Perturbation method is O(n) slower than correlation method\n");
    bench_marginal_var_perturbation(5, 50, "marginal_var_perturb_5x50");
    bench_marginal_var_perturbation(10, 100, "marginal_var_perturb_10x100");
    bench_marginal_var_perturbation(20, 252, "marginal_var_perturb_20x252");

    printf("\n--- Large Portfolio Stress Test ---\n");
    bench_marginal_var_correlation(2000, 252, "marginal_var_corr_2000x252");
    bench_marginal_var_correlation(5000, 252, "marginal_var_corr_5000x252");

    printf("\n=================================================================\n");
}
