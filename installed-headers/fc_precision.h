/**
 * @file fc_precision.h
 * @brief High-precision floating-point precision and rounding controls
 */

#ifndef FC_PRECISION_H
#define FC_PRECISION_H

#include "error.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Rounding modes for high-precision floating-point operations
 */
typedef enum {
    FC_RNDN = 0, /**< Round to nearest */
    FC_RNDZ = 1, /**< Round toward zero */
    FC_RNDU = 2, /**< Round toward +inf */
    FC_RNDD = 3, /**< Round toward -inf */
    FC_RNDA = 4, /**< Round away from zero */
} fc_rounding_mode_t;

/**
 * @brief Set the default floating-point precision in bits
 *
 * @param bits Precision in bits, must be greater than zero
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_precision_set_default_bits(fc_uint64_t bits);

/**
 * @brief Get the default floating-point precision in bits
 *
 * @param bits_out Output pointer for precision bits
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_precision_get_default_bits(fc_uint64_t* bits_out);

/**
 * @brief Set the default floating-point rounding mode
 *
 * @param mode Rounding mode
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_precision_set_rounding_mode(fc_rounding_mode_t mode);

/**
 * @brief Get the default floating-point rounding mode
 *
 * @param mode_out Output pointer for rounding mode
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_precision_get_rounding_mode(fc_rounding_mode_t* mode_out);

FC_END_DECLS

#endif /* FC_PRECISION_H */
