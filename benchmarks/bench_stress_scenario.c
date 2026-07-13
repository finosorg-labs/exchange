/**
 * @file bench_stress_scenario.c
 * @brief Performance benchmarks for stress scenario application
 */

#include "bench_framework.h"
#include "risk/stress_scenario.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define ALIGN_SIZE(size) (((size) + 63) / 64 * 64)

/* Benchmark data structures */
typedef struct {
    double* scenario_pnl;
    const double* position_values;
    const double* shocks;
    fc_shock_type_t shock_type;
    size_t n_assets;
} bench_apply_scenario_data_t;

typedef struct {
    double* scenario_pnls;
    const double* position_values;
    const double* shock_matrix;
    fc_shock_type_t shock_type;
    size_t n_assets;
    size_t n_scenarios;
} bench_apply_scenarios_batch_data_t;

/* Benchmark functions */
static void bench_apply_scenario_fn(void* user_data) {
    bench_apply_scenario_data_t* data = (bench_apply_scenario_data_t*) user_data;
    fc_ex_risk_apply_scenario(
        data->scenario_pnl, data->position_values, data->shocks, data->shock_type, data->n_assets
    );
}

static void bench_apply_scenarios_batch_fn(void* user_data) {
    bench_apply_scenarios_batch_data_t* data = (bench_apply_scenarios_batch_data_t*) user_data;
    fc_ex_risk_apply_scenarios_batch(
        data->scenario_pnls,
        data->position_values,
        data->shock_matrix,
        data->shock_type,
        data->n_assets,
        data->n_scenarios
    );
}

/* Benchmark single scenario application */
static void bench_apply_scenario(size_t n_assets, const char* name) {
    double* position_values = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double* shocks          = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double scenario_pnl;

    if (!position_values || !shocks) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (size_t i = 0; i < n_assets; i++) {
        position_values[i] = 10000.0 + (double) i * 100.0;
        shocks[i]          = -0.01 * (1.0 + 0.001 * (double) i);
    }

    bench_apply_scenario_data_t data = {
        .scenario_pnl    = &scenario_pnl,
        .position_values = position_values,
        .shocks          = shocks,
        .shock_type      = FC_SHOCK_MULTIPLICATIVE,
        .n_assets        = n_assets
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = n_assets * sizeof(double);
    fc_bench_result_t result;

    fc_bench_run(&config, bench_apply_scenario_fn, &data, &result);
    fc_bench_result_print(&result);

    free(position_values);
    free(shocks);
}

/* Benchmark batch scenario application */
static void bench_apply_scenarios_batch(size_t n_assets, size_t n_scenarios, const char* name) {
    double* position_values = aligned_alloc(64, ALIGN_SIZE(n_assets * sizeof(double)));
    double* shock_matrix  = aligned_alloc(64, ALIGN_SIZE(n_assets * n_scenarios * sizeof(double)));
    double* scenario_pnls = aligned_alloc(64, ALIGN_SIZE(n_scenarios * sizeof(double)));

    if (!position_values || !shock_matrix || !scenario_pnls) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    for (size_t i = 0; i < n_assets; i++) {
        position_values[i] = 10000.0 + (double) i * 100.0;
    }

    for (size_t i = 0; i < n_assets; i++) {
        for (size_t s = 0; s < n_scenarios; s++) {
            shock_matrix[i * n_scenarios + s] =
                -0.01 * (double) (s + 1) + 0.001 * (double) (i % 10);
        }
    }

    bench_apply_scenarios_batch_data_t data = {
        .scenario_pnls   = scenario_pnls,
        .position_values = position_values,
        .shock_matrix    = shock_matrix,
        .shock_type      = FC_SHOCK_MULTIPLICATIVE,
        .n_assets        = n_assets,
        .n_scenarios     = n_scenarios
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = n_assets * n_scenarios * sizeof(double);
    fc_bench_result_t result;

    fc_bench_run(&config, bench_apply_scenarios_batch_fn, &data, &result);
    fc_bench_result_print(&result);

    free(position_values);
    free(shock_matrix);
    free(scenario_pnls);
}

void bench_stress_scenario_run(void) {
    printf("\n=================================================================\n");
    printf("Stress Scenario Application Benchmarks\n");
    printf("=================================================================\n");

    printf("\n--- Single Scenario Application ---\n");
    bench_apply_scenario(100, "apply_scenario_100");
    bench_apply_scenario(500, "apply_scenario_500");
    bench_apply_scenario(1000, "apply_scenario_1000");
    bench_apply_scenario(5000, "apply_scenario_5000");

    printf("\n--- Batch Scenario Application ---\n");
    bench_apply_scenarios_batch(100, 10, "batch_100x10");
    bench_apply_scenarios_batch(500, 10, "batch_500x10");
    bench_apply_scenarios_batch(1000, 10, "batch_1000x10");
    bench_apply_scenarios_batch(1000, 50, "batch_1000x50");
    bench_apply_scenarios_batch(1000, 100, "batch_1000x100");

    printf("\n--- Performance Target Verification ---\n");
    printf("Target: 1000 assets × 10 scenarios in ~50 μs (~20K ops/s)\n");
    bench_apply_scenarios_batch(1000, 10, "target_1000x10");

    printf("\n=================================================================\n");
}
