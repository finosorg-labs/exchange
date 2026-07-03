/**
 * @file microprice.h
 * @brief Micro-price signal computation
 *
 * Micro-price is a volume-weighted midpoint price that provides a more accurate
 * prediction of the next trade price compared to traditional midpoint:
 *
 *     mp = bid × ask_q/(bid_q+ask_q) + ask × bid_q/(bid_q+ask_q)
 *
 * When bid_q >> ask_q, mp is closer to ask (more selling pressure)
 * When ask_q >> bid_q, mp is closer to bid (more buying pressure)
 *
 * Performance:
 * - Single tick (Go): O(1), ~50-100ns
 * - Batch (C+SIMD): ~5-10ns per symbol (AVX-512)
 *
 * @note All batch functions require 64-byte aligned output buffers for optimal SIMD performance
 */

#ifndef FC_EX_SIGNAL_MICROPRICE_H
#define FC_EX_SIGNAL_MICROPRICE_H

#include "error.h"
#include <platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute micro-price for a batch of symbols
 *
 * Calculates volume-weighted midpoint price using best bid/ask prices and quantities:
 *
 *     mp[i] = bid_p[i] × ask_q[i]/(bid_q[i]+ask_q[i]) + ask_p[i] × bid_q[i]/(bid_q[i]+ask_q[i])
 *
 * If bid_q[i] + ask_q[i] <= 0, mp_out[i] is set to NaN.
 *
 * @param[out] mp_out      Micro-price output (n elements, 64-byte aligned recommended)
 * @param[in]  bid_p       Best bid prices (n elements)
 * @param[in]  bid_q       Best bid quantities (n elements)
 * @param[in]  ask_p       Best ask prices (n elements)
 * @param[in]  ask_q       Best ask quantities (n elements)
 * @param[in]  n           Number of symbols
 *
 * @return FC_OK on success, error code otherwise:
 *         - FC_ERR_INVALID_ARG: any pointer is NULL or n is 0
 *
 * @note Thread-safe (no shared state)
 * @note Time complexity: O(n)
 * @note Space complexity: O(1) auxiliary
 * @note SIMD: Auto-dispatches to AVX-512/AVX2/SSE4.2/Scalar based on CPU capabilities
 */
FC_API fc_status_t fc_ex_sig_microprice_batch(
    double* mp_out,
    const double* bid_p,
    const double* bid_q,
    const double* ask_p,
    const double* ask_q,
    size_t n
);

#ifdef __cplusplus
}
#endif

#endif /* FC_EX_SIGNAL_MICROPRICE_H */
