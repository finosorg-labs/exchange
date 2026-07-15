/**
 * @file stats.h
 * @brief Statistics module public API
 *
 * This header provides the main entry point for the stats module,
 * which includes various statistical computation implementations.
 */

#ifndef FC_STATS_H
#define FC_STATS_H

#include "correlation.h"
#include "ema.h"
#include "kahan_sum.h"
#include "mean_var.h"
#include "quantile.h"
#include "rank.h"
#include "weighted.h"
#include "welford.h"
#include "winsorize.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Module version */
#define FINKIT_STATS_VERSION_MAJOR 1
#define FINKIT_STATS_VERSION_MINOR 0
#define FINKIT_STATS_VERSION_PATCH 0

#ifdef __cplusplus
}
#endif

#endif /* FC_STATS_H */
