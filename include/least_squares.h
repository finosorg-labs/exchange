#ifndef FC_OPTIM_LEAST_SQUARES_H
#define FC_OPTIM_LEAST_SQUARES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Solve ordinary least squares regression using QR decomposition.
 *
 * Solves the linear regression problem: min ||y - X*beta||^2
 * where X is the design matrix and y is the response vector.
 *
 * Uses QR decomposition with Householder reflections for numerical stability.
 * This method is more numerically stable than normal equations, especially
 * for ill-conditioned matrices.
 *
 * @param X Design matrix in column-major order (n x p), where n is the number
 *          of observations and p is the number of predictors. Must be non-NULL.
 * @param y Response vector (n x 1). Must be non-NULL.
 * @param n Number of observations. Must be > 0 and >= p.
 * @param p Number of predictors. Must be > 0.
 * @param fit_intercept If non-zero, fit an intercept term; if zero, force regression through
 * origin.
 * @param beta Output: regression coefficients. Must be pre-allocated by caller.
 *             If fit_intercept != 0: size >= (p+1), beta[0] = intercept, beta[1..p] = coefficients
 *             If fit_intercept == 0: size >= p, beta[0..p-1] = coefficients
 *             Must be non-NULL.
 * @return 0 on success
 *         -1 if any pointer is NULL
 *         -2 if n <= 0 or p <= 0 or (fit_intercept && n <= p) or (!fit_intercept && n < p)
 *         -3 if matrix is rank deficient or QR decomposition fails
 *
 * Time complexity: O(n*p^2)
 * Space complexity: O(n*p) (modifies X in-place, allocates O(n+p) temp space)
 * Thread safety: Thread-safe if inputs are not shared across threads
 *
 * @note The input matrix X is modified during computation (used as workspace).
 *       If you need to preserve X, pass a copy.
 * @note This function performs heap allocation for workspace. For better performance
 *       in hot paths, use fc_optim_least_squares_work() with caller-provided workspace.
 */
int fc_optim_least_squares(
    double* X,
    const double* y,
    size_t n,
    size_t p,
    int fit_intercept,
    double* beta
);

/**
 * Solve ordinary least squares regression with caller-provided workspace.
 *
 * Same as fc_optim_least_squares() but uses caller-provided workspace to avoid
 * heap allocation. This is significantly faster in hot paths and batch processing.
 *
 * @param X Design matrix in column-major order (n x p). Must be non-NULL.
 * @param y Response vector (n x 1). Must be non-NULL.
 * @param n Number of observations. Must be > 0 and >= p.
 * @param p Number of predictors. Must be > 0.
 * @param fit_intercept If non-zero, fit an intercept term; if zero, force regression through
 * origin.
 * @param beta Output: regression coefficients. Must be pre-allocated by caller.
 *             If fit_intercept != 0: size >= (p+1), beta[0] = intercept, beta[1..p] = coefficients
 *             If fit_intercept == 0: size >= p, beta[0..p-1] = coefficients
 *             Must be non-NULL.
 * @param work Workspace buffer. Must be pre-allocated.
 *             If fit_intercept != 0: size >= (n*(p+1) + n + p + 1) * sizeof(double)
 *             If fit_intercept == 0: size >= (n + p) * sizeof(double)
 *             Must be non-NULL.
 * @return 0 on success, negative error code on failure
 *
 * Time complexity: O(n*p^2)
 * Space complexity: O(n*p) (modifies X in-place, uses provided workspace)
 * Thread safety: Thread-safe if inputs and workspace are not shared
 *
 * @note The input matrix X is modified during computation.
 * @note Workspace layout without intercept: [tau: p doubles][qtb: n doubles]
 * @note Workspace layout with intercept: [X_aug: n*(p+1) doubles][tau: (p+1) doubles][qtb: n
 * doubles]
 */
int fc_optim_least_squares_work(
    double* X,
    const double* y,
    size_t n,
    size_t p,
    int fit_intercept,
    double* beta,
    double* work
);

/**
 * Solve multiple independent least squares regressions in batch.
 *
 * Efficiently solves batch_size independent regression problems, each with
 * the same dimensions (n observations, p predictors). This is more efficient
 * than calling fc_optim_least_squares() in a loop due to better cache usage.
 *
 * @param X Design matrices in column-major order (batch_size x n x p).
 *          Each regression has its own n x p matrix. Must be non-NULL.
 * @param y Response vectors (batch_size x n). Each regression has its own
 *          n x 1 response vector. Must be non-NULL.
 * @param n Number of observations per regression. Must be > 0 and >= p.
 * @param p Number of predictors per regression. Must be > 0.
 * @param batch_size Number of independent regressions to solve. Must be > 0.
 * @param fit_intercept If non-zero, fit intercept terms; if zero, force regression through origin.
 * @param beta Output: regression coefficients. Must be pre-allocated by caller.
 *             If fit_intercept != 0: size >= batch_size * (p+1), layout per batch: [intercept,
 * coef1, ..., coefp] If fit_intercept == 0: size >= batch_size * p Must be non-NULL.
 * @return Number of successfully solved regressions (0 to batch_size)
 *         Returns 0 if any pointer is NULL or dimensions are invalid
 *
 * Time complexity: O(batch_size * n * p^2)
 * Space complexity: O(batch_size * n * p)
 * Thread safety: Thread-safe if inputs are not shared across threads
 *
 * @note If a regression fails (e.g., rank deficiency), the corresponding beta
 *       coefficients are set to 0.0 and the count is not incremented.
 * @note Input matrices X are modified during computation.
 */
size_t fc_optim_least_squares_batch(
    double* X,
    const double* y,
    size_t n,
    size_t p,
    size_t batch_size,
    int fit_intercept,
    double* beta
);

/**
 * Solve ordinary least squares regression and compute additional statistics.
 *
 * Extended version that computes regression coefficients plus residuals,
 * R-squared, and residual standard error.
 *
 * @param X Design matrix in column-major order (n x p). Must be non-NULL.
 * @param y Response vector (n x 1). Must be non-NULL.
 * @param n Number of observations. Must be > 0 and >= p.
 * @param p Number of predictors. Must be > 0.
 * @param fit_intercept If non-zero, fit an intercept term; if zero, force regression through
 * origin.
 * @param beta Output: regression coefficients. Must be pre-allocated.
 *             If fit_intercept != 0: size >= (p+1), beta[0] = intercept, beta[1..p] = coefficients
 *             If fit_intercept == 0: size >= p, beta[0..p-1] = coefficients
 *             Must be non-NULL.
 * @param residuals Output: residuals (n x 1), can be NULL if not needed.
 *                  If non-NULL, must be pre-allocated with size >= n.
 * @param r_squared Output: coefficient of determination (R^2), can be NULL.
 *                  If fit_intercept != 0: R^2 = 1 - SS_res/SS_tot (centered total sum of squares)
 *                  If fit_intercept == 0: R^2 = 1 - SS_res/SS_tot_uncent (uncentered, may be
 * negative or > 1)
 * @param std_error Output: residual standard error, can be NULL.
 * @return 0 on success, negative error code on failure (same as fc_optim_least_squares)
 *
 * Time complexity: O(n*p^2 + n*p)
 * Space complexity: O(n*p)
 * Thread safety: Thread-safe if inputs are not shared across threads
 *
 * @note Input matrix X is modified during computation.
 */
int fc_optim_least_squares_ext(
    double* X,
    const double* y,
    size_t n,
    size_t p,
    int fit_intercept,
    double* beta,
    double* residuals,
    double* r_squared,
    double* std_error
);

#ifdef __cplusplus
}
#endif

#endif
