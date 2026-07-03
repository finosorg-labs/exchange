#ifndef FC_STATS_ZSCORE_H
#define FC_STATS_ZSCORE_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Compute Z-scores (standardization) for batch data
 *
 * Computes Z-score = (x - mean) / std for each element in the input array.
 * This function standardizes data to have mean=0 and std=1.
 *
 * @param[out] out       Output array for Z-scores (n elements)
 * @param[in]  data      Input data array (n elements)
 * @param[in]  n         Number of elements
 * @param[in]  sample    1 for sample std (N-1), 0 for population std (N)
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note If std is zero or very close to zero, output will be NaN
 * @note Input and output arrays can be the same (in-place operation)
 * @note Time complexity: O(n)
 * @note Thread-safe: Yes (no shared state)
 *
 * Example:
 * @code
 * double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
 * double out[5];
 * fc_stat_zscore_f64(out, data, 5, 1);
 * // out ≈ {-1.414, -0.707, 0.0, 0.707, 1.414}
 * @endcode
 */
FC_API fc_status_t fc_stat_zscore_f64(double* out, const double* data, size_t n, int sample);

/**
 * @brief Compute Z-scores for multiple groups in batch
 *
 * Computes Z-scores independently for each group. Each group is standardized
 * to have mean=0 and std=1 within that group.
 *
 * @param[out] out         Output array (n_groups * group_size elements)
 * @param[in]  data        Input data array (n_groups * group_size elements)
 * @param[in]  n_groups    Number of groups
 * @param[in]  group_size  Number of elements per group
 * @param[in]  sample      1 for sample std (N-1), 0 for population std (N)
 *
 * @return FC_OK on success, error code otherwise
 *
 * @note Data layout: [group0_elem0, group0_elem1, ..., group1_elem0, ...]
 * @note If any group's std is zero, that group's output will be NaN
 * @note Time complexity: O(n_groups * group_size)
 * @note Thread-safe: Yes (no shared state)
 *
 * Example:
 * @code
 * // 3 groups, 4 elements each
 * double data[] = {1,2,3,4,  10,20,30,40,  100,200,300,400};
 * double out[12];
 * fc_stat_zscore_batch_f64(out, data, 3, 4, 1);
 * // Each group independently standardized
 * @endcode
 */
FC_API fc_status_t fc_stat_zscore_batch_f64(
    double* out,
    const double* data,
    size_t n_groups,
    size_t group_size,
    int sample
);

FC_END_DECLS

#endif // FC_STATS_ZSCORE_H
