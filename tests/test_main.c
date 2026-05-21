/**
 * @file test_main.c
 * @brief Test runner entry point for exchange module
 */

#include "test_framework.h"
#include <platform.h>
#include <simd_detect.h>
#include <stdio.h>
#include <stdlib.h>

/* External test registration functions */
extern void register_exchange_tests(void);

int main(int argc, char** argv) {
    /* Initialize test framework with command line arguments */
    fc_test_init_with_args(argc, argv);

    fc_init();

    /* Register all test suites */
    register_exchange_tests();

    int result = fc_test_run_all();

    /* Generate coverage report if requested */
    fc_test_generate_coverage_report();

    fc_test_cleanup();

    printf("\nexchange self-test: %s\n", result == 0 ? "PASS" : "FAIL");
    return result;
}
