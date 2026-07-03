#ifndef FC_STATS_COVARIANCE_H
#define FC_STATS_COVARIANCE_H

#include "error.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute covariance matrix for multiple variables
 *
 * Computes the covariance matrix Cov[i,j] = E[(X_i - μ_i)(X_j - μ_j)]
 * using the formula: Cov = (X^T X - n × mean × mean^T) / (n-1)
 *
 * @param[out] cov Output covariance matrix (n_vars × n_vars), row-major
 * @param[in] data Input data matrix (n_samples × n_vars), row-major
 * @param[in] n_samples Number of samples (rows)
 * @param[in] n_vars Number of variables (columns)
 * @param[in] sample_cov If 1, use sample covariance (divide by n-1); if 0, use population
 * covariance (divide by n)
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n_vars^2 × n_samples)
 * @note Space complexity: O(n_vars) for temporary mean storage
 * @note Thread-safe: Yes (no shared state)
 * @note Requires: n_samples >= 2 for sample covariance, n_samples >= 1 for population covariance
 */
fc_status_t fc_stats_covariance_f64(
    double* cov,
    const double* data,
    size_t n_samples,
    size_t n_vars,
    int sample_cov
);

/**
 * @brief Compute correlation coefficient matrix from covariance matrix
 *
 * Computes the correlation matrix Corr[i,j] = Cov[i,j] / (σ_i × σ_j)
 * where σ_i = sqrt(Cov[i,i])
 *
 * @param[out] corr Output correlation matrix (n_vars × n_vars), row-major
 * @param[in] cov Input covariance matrix (n_vars × n_vars), row-major
 * @param[in] n_vars Number of variables
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n_vars^2)
 * @note Space complexity: O(n_vars) for temporary standard deviation storage
 * @note Thread-safe: Yes (no shared state)
 * @note Diagonal elements are always 1.0
 */
fc_status_t fc_stats_correlation_f64(double* corr, const double* cov, size_t n_vars);

/**
 * @brief Compute covariance and correlation matrices in one pass
 *
 * More efficient than calling covariance and correlation separately.
 *
 * @param[out] cov Output covariance matrix (n_vars × n_vars), row-major
 * @param[out] corr Output correlation matrix (n_vars × n_vars), row-major
 * @param[in] data Input data matrix (n_samples × n_vars), row-major
 * @param[in] n_samples Number of samples (rows)
 * @param[in] n_vars Number of variables (columns)
 * @param[in] sample_cov If 1, use sample covariance; if 0, use population covariance
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n_vars^2 × n_samples)
 * @note Thread-safe: Yes (no shared state)
 */
fc_status_t fc_stats_covariance_correlation_f64(
    double* cov,
    double* corr,
    const double* data,
    size_t n_samples,
    size_t n_vars,
    int sample_cov
);

/**
 * @brief Compute Spearman rank correlation coefficient matrix
 *
 * Computes the Spearman rank correlation coefficient, which is the Pearson
 * correlation coefficient computed on the ranks of the data. This is a
 * non-parametric measure of monotonic association.
 *
 * @param[out] corr Output Spearman correlation matrix (n_vars × n_vars), row-major
 * @param[in] data Input data matrix (n_samples × n_vars), row-major
 * @param[in] n_samples Number of samples (rows)
 * @param[in] n_vars Number of variables (columns)
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Time complexity: O(n_vars × n_samples × log(n_samples))
 * @note Space complexity: O(n_samples × n_vars) for temporary rank storage
 * @note Thread-safe: Yes (no shared state)
 * @note Requires: n_samples >= 2
 * @note Ties are handled using average rank method
 * @note Diagonal elements are always 1.0
 * @note Values are in range [-1, 1]
 *
 * Example:
 *   Data: [[1, 10], [2, 20], [3, 15]]
 *   Ranks: [[1, 1], [2, 3], [3, 2]]
 *   Spearman correlation measures monotonic (not necessarily linear) relationship
 */
fc_status_t fc_stats_spearman_f64(
    double* corr,
    const double* data,
    size_t n_samples,
    size_t n_vars
);

#ifdef __cplusplus
}
#endif

#endif // FC_STATS_COVARIANCE_H
