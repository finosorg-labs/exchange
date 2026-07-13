/**
 * @file bench_vwap_dev.c
 * @brief Performance benchmarks for VWAP deviation computation
 */

#include "bench_framework.h"
#include "platform.h"
#include "signal/vwap_dev.h"
#include "simd_detect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    double* z_out;
    double* price;
    double* vwap;
    double* sigma;
    size_t n_symbols;
    int n_windows;
} bench_vwap_dev_batch_data_t;

typedef struct {
    double* pv_acc;
    double* v_acc;
    double* pv_comp;
    double* v_comp;
    double* vwap_out;
    double* price;
    double* volume;
    size_t n;
} bench_vwap_update_data_t;

static void bench_vwap_dev_batch_fn(void* user_data) {
    bench_vwap_dev_batch_data_t* data = (bench_vwap_dev_batch_data_t*) user_data;
    fc_ex_sig_vwap_dev_batch(
        data->z_out, data->price, data->vwap, data->sigma, data->n_symbols, data->n_windows
    );
}

static void bench_vwap_update_fn(void* user_data) {
    bench_vwap_update_data_t* data = (bench_vwap_update_data_t*) user_data;
    fc_ex_sig_vwap_update(
        data->pv_acc,
        data->v_acc,
        data->pv_comp,
        data->v_comp,
        data->vwap_out,
        data->price,
        data->volume,
        data->n
    );
}

static void bench_vwap_dev_batch_impl(size_t n_symbols, int n_windows, const char* name) {
    const size_t total_elements = n_symbols * n_windows;

    double* price = aligned_alloc(64, n_symbols * sizeof(double));
    double* vwap  = aligned_alloc(64, total_elements * sizeof(double));
    double* sigma = aligned_alloc(64, total_elements * sizeof(double));
    double* z_out = aligned_alloc(64, total_elements * sizeof(double));

    if (!price || !vwap || !sigma || !z_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    /* Initialize test data */
    for (size_t i = 0; i < n_symbols; i++) {
        price[i] = 100.0 + (double) i * 0.5;

        for (int w = 0; w < n_windows; w++) {
            size_t idx = i * n_windows + w;
            /* VWAP slightly below/above price, varying by window */
            vwap[idx] = price[i] + ((w % 2 == 0) ? -1.0 : 1.0) * (double) (w + 1) * 0.5;
            /* Standard deviation increases with window size */
            sigma[idx] = 0.5 + (double) w * 0.25;
        }
    }

    bench_vwap_dev_batch_data_t data = {
        .z_out     = z_out,
        .price     = price,
        .vwap      = vwap,
        .sigma     = sigma,
        .n_symbols = n_symbols,
        .n_windows = n_windows
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = total_elements * sizeof(double);
    config.min_iterations    = 100;
    config.min_time_ms       = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_vwap_dev_batch_fn, &data, &result);
    fc_bench_result_print(&result);

    free(price);
    free(vwap);
    free(sigma);
    free(z_out);
}

static void bench_vwap_update_impl(size_t n, const char* name) {
    double* pv_acc   = aligned_alloc(64, n * sizeof(double));
    double* v_acc    = aligned_alloc(64, n * sizeof(double));
    double* pv_comp  = aligned_alloc(64, n * sizeof(double));
    double* v_comp   = aligned_alloc(64, n * sizeof(double));
    double* vwap_out = aligned_alloc(64, n * sizeof(double));
    double* price    = aligned_alloc(64, n * sizeof(double));
    double* volume   = aligned_alloc(64, n * sizeof(double));

    if (!pv_acc || !v_acc || !pv_comp || !v_comp || !vwap_out || !price || !volume) {
        fprintf(stderr, "Memory allocation failed\n");
        free(pv_acc);
        free(v_acc);
        free(pv_comp);
        free(v_comp);
        free(vwap_out);
        free(price);
        free(volume);
        return;
    }

    /* Initialize accumulators and compensators to zero */
    memset(pv_acc, 0, n * sizeof(double));
    memset(v_acc, 0, n * sizeof(double));
    memset(pv_comp, 0, n * sizeof(double));
    memset(v_comp, 0, n * sizeof(double));

    /* Initialize test data */
    for (size_t i = 0; i < n; i++) {
        price[i]  = 100.0 + (double) (i % 100) * 0.1;
        volume[i] = 1000.0 + (double) (i % 500);
    }

    bench_vwap_update_data_t data = {
        .pv_acc   = pv_acc,
        .v_acc    = v_acc,
        .pv_comp  = pv_comp,
        .v_comp   = v_comp,
        .vwap_out = vwap_out,
        .price    = price,
        .volume   = volume,
        .n        = n
    };

    fc_bench_config_t config = FC_BENCH_CONFIG_DEFAULT;
    config.name              = name;
    config.data_size         = n * sizeof(double);
    config.min_iterations    = 100;
    config.min_time_ms       = 100.0;

    fc_bench_result_t result;
    fc_bench_run(&config, bench_vwap_update_fn, &data, &result);
    fc_bench_result_print(&result);

    free(pv_acc);
    free(v_acc);
    free(pv_comp);
    free(v_comp);
    free(vwap_out);
    free(price);
    free(volume);
}

static void print_header(void) {
    printf("\nVWAP Deviation Signal Benchmarks\n");
    printf("------------------------------------------------------------\n");
}

static void print_cpu_info(void) {
    fc_simd_level_t level = fc_get_simd_level();
    printf("\nCPU Features: ");
    if (level >= FC_SIMD_AVX512)
        printf("AVX-512 ");
    else if (level >= FC_SIMD_AVX2)
        printf("AVX2 ");
    else if (level >= FC_SIMD_SSE42)
        printf("SSE4.2 ");
    else
        printf("Scalar ");
    printf("\n");
}

void bench_vwap_dev_run(void) {
    print_header();
    print_cpu_info();

    bench_vwap_dev_batch_impl(100, 4, "VWAPDev/Batch/Symbols=100/Windows=4");
    bench_vwap_dev_batch_impl(1000, 4, "VWAPDev/Batch/Symbols=1000/Windows=4");
    bench_vwap_dev_batch_impl(5000, 4, "VWAPDev/Batch/Symbols=5000/Windows=4");
    bench_vwap_dev_batch_impl(1000, 8, "VWAPDev/Batch/Symbols=1000/Windows=8");

    bench_vwap_update_impl(100, "VWAPUpdate/Batch/N=100");
    bench_vwap_update_impl(1000, "VWAPUpdate/Batch/N=1000");
    bench_vwap_update_impl(5000, "VWAPUpdate/Batch/N=5000");
    bench_vwap_update_impl(10000, "VWAPUpdate/Batch/N=10000");

    printf("\nPerformance Targets:\n");
    printf("  VWAP deviation (SIMD):          ~50-100 ns per symbol\n");
    printf("  VWAP update (Kahan):            ~20-50 ns per symbol\n");
    printf("  Signal layer total budget:      ~0.5-2 μs\n");
}
