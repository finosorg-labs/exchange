/**
 * @file correlation.h
 * @brief Pearson correlation coefficient computation with SIMD optimization
 *
 * Provides high-performance Pearson correlation coefficient computation.
 * Supports SIMD acceleration (AVX-512, AVX2, SSE4.2) with runtime dispatch.
 *
 * Key features:
 * - Pairwise Pearson correlation for two variables
 * - Pearson correlation matrix computation from raw data
 * - SIMD optimization for large datasets
 * - Numerically stable computation
 * - Thread-safe (no global state)
 *
 * Performance targets:
 * - 500 variables × 250 samples: < 50ms (vs Go > 250ms)
 * - Throughput: > 5x speedup vs Go implementation
 *
 * Mathematical definition:
 * Pearson correlation coefficient between X and Y:
 *   r = Cov(X,Y) / (σ_X * σ_Y)
 *     = Σ((x_i - μ_X)(y_i - μ_Y)) / sqrt(Σ(x_i - μ_X)² * Σ(y_i - μ_Y)²)
 *
 * Range: [-1, 1]
 *   r =  1: perfect positive linear correlation
 *   r =  0: no linear correlation
 *   r = -1: perfect negative linear correlation
 */

#ifndef FC_STATS_CORRELATION_H
#define FC_STATS_CORRELATION_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute Pearson correlation coefficient between two arrays
 *
 * Computes the Pearson correlation coefficient between two variables.
 * Uses numerically stable two-pass algorithm.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @param x First input array (must not be NULL)
 * @param y Second input array (must not be NULL)
 * @param n Number of elements (must be >= 2)
 * @param corr Output correlation coefficient (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note Returns 0.0 if either variable has zero variance
 */
FC_API fc_status_t
fc_stats_pearson_correlation_f64(const double* x, const double* y, size_t n, double* corr);

/**
 * @brief Compute Pearson correlation coefficient matrix from raw data
 *
 * Computes the full Pearson correlation matrix for multiple variables.
 * Input data is organized as row-major: each row is a sample, each column is a variable.
 * Output is a symmetric matrix stored in row-major order.
 *
 * Time complexity: O(n_vars² * n_samples)
 * Space complexity: O(n_vars) - for temporary means and stddevs
 *
 * @param data Input data matrix in row-major order (must not be NULL)
 *             Size: n_samples × n_vars
 *             data[i * n_vars + j] = sample i of variable j
 * @param n_samples Number of samples/observations (must be >= 2)
 * @param n_vars Number of variables (must be >= 1)
 * @param corr_matrix Output correlation matrix in row-major order (must not be NULL)
 *                    Size: n_vars × n_vars
 *                    corr_matrix[i * n_vars + j] = correlation(var_i, var_j)
 *                    Diagonal elements are 1.0
 *                    Matrix is symmetric: corr_matrix[i,j] = corr_matrix[j,i]
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note Variables with zero variance will have correlation 0.0 with all other variables
 * @note Only computes upper triangle and diagonal, then mirrors to lower triangle
 */
FC_API fc_status_t fc_stats_pearson_correlation_matrix_f64(
    const double* data,
    size_t n_samples,
    size_t n_vars,
    double* corr_matrix
);

/**
 * @brief Compute correlation matrix from pre-computed means and standard deviations
 *
 * More efficient when means and standard deviations are already available.
 * Useful for incremental computation or when statistics are cached.
 *
 * Time complexity: O(n_vars² * n_samples)
 * Space complexity: O(1)
 *
 * @param data Input data matrix in row-major order (must not be NULL)
 * @param n_samples Number of samples (must be >= 2)
 * @param n_vars Number of variables (must be >= 1)
 * @param means Pre-computed means for each variable (must not be NULL, size: n_vars)
 * @param stddevs Pre-computed standard deviations (must not be NULL, size: n_vars)
 * @param corr_matrix Output correlation matrix in row-major order (must not be NULL)
 *
 * @return FC_OK on success, error code on failure
 *
 * @note Thread-safe
 * @note Handles NaN: returns FC_ERR_NAN_INPUT if any element is NaN
 * @note Variables with zero stddev will have correlation 0.0 with all other variables
 */
FC_API fc_status_t fc_stats_pearson_correlation_matrix_precomputed_f64(
    const double* data,
    size_t n_samples,
    size_t n_vars,
    const double* means,
    const double* stddevs,
    double* corr_matrix
);

FC_END_DECLS

#endif /* FC_STATS_CORRELATION_H */
