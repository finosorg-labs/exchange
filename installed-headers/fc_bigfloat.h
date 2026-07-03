/**
 * @file fc_bigfloat.h
 * @brief High-precision floating-point API
 *
 * Provides arbitrary-precision floating-point arithmetic backed by MPFR.
 * Supports configurable precision and IEEE 754-style rounding modes.
 * All operations are thread-safe after initialization.
 */

#ifndef FC_BIGFLOAT_H
#define FC_BIGFLOAT_H

#include "error.h"
#include "fc_bigint.h"
#include "fc_precision.h"
#include "platform.h"

FC_BEGIN_DECLS

/**
 * @brief Opaque handle for high-precision floating-point
 */
typedef struct fc_bigfloat fc_bigfloat_t;

/**
 * @brief Create a new bigfloat with default precision
 *
 * @param[out] value_out Pointer to receive the created bigfloat handle
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if value_out is NULL,
 *         FC_ERR_NOT_INITIALIZED if library not initialized,
 *         FC_ERR_OUT_OF_MEMORY if allocation fails
 */
FC_API fc_status_t fc_bigfloat_create(fc_bigfloat_t** value_out);

/**
 * @brief Create a new bigfloat with specified precision
 *
 * @param[out] value_out Pointer to receive the created bigfloat handle
 * @param bits Precision in bits (must be > 0 and <= MPFR_PREC_MAX)
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_create_with_prec(fc_bigfloat_t** value_out, fc_uint64_t bits);

/**
 * @brief Destroy a bigfloat and free its resources
 *
 * Safe to call with NULL pointer.
 *
 * @param value Bigfloat handle to destroy
 */
FC_API void fc_bigfloat_destroy(fc_bigfloat_t* value);

/**
 * @brief Copy a bigfloat
 *
 * Destination precision is adjusted to match source.
 *
 * @param dest Destination bigfloat
 * @param src Source bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_copy(fc_bigfloat_t* dest, const fc_bigfloat_t* src);

/**
 * @brief Get the precision of a bigfloat
 *
 * @param value Bigfloat to query
 * @param[out] bits_out Pointer to receive precision in bits
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_get_prec(const fc_bigfloat_t* value, fc_uint64_t* bits_out);

/**
 * @brief Set the precision of a bigfloat
 *
 * Existing value is preserved with rounding if necessary.
 *
 * @param value Bigfloat to modify
 * @param bits New precision in bits
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_set_prec(fc_bigfloat_t* value, fc_uint64_t bits);

/**
 * @brief Set bigfloat from double
 *
 * @param value Bigfloat to set
 * @param input Input value (must be finite)
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if input is NaN or Inf
 */
FC_API fc_status_t fc_bigfloat_set_f64(fc_bigfloat_t* value, fc_double_t input);

/**
 * @brief Set bigfloat from signed 64-bit integer
 *
 * @param value Bigfloat to set
 * @param input Input value
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_set_i64(fc_bigfloat_t* value, fc_int64_t input);

/**
 * @brief Set bigfloat from unsigned 64-bit integer
 *
 * @param value Bigfloat to set
 * @param input Input value
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_set_u64(fc_bigfloat_t* value, fc_uint64_t input);

/**
 * @brief Set bigfloat from decimal string
 *
 * @param value Bigfloat to set
 * @param input Decimal string (e.g., "3.14159", "-1.5e10")
 *
 * @return FC_OK on success, FC_ERR_PARSE if string is invalid,
 *         FC_ERR_INVALID_ARG if result is NaN or Inf
 */
FC_API fc_status_t fc_bigfloat_set_str(fc_bigfloat_t* value, const char* input);

/**
 * @brief Set bigfloat from bigint
 *
 * @param value Bigfloat to set
 * @param input Input bigint
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_set_bigint(fc_bigfloat_t* value, const fc_bigint_t* input);

/**
 * @brief Convert bigfloat to double
 *
 * @param value Bigfloat to convert
 * @param[out] output Pointer to receive the result
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_get_f64(const fc_bigfloat_t* value, fc_double_t* output);

/**
 * @brief Get required buffer size for string representation
 *
 * @param value Bigfloat to measure
 * @param[out] size_out Pointer to receive buffer size (including null terminator)
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_string_size(const fc_bigfloat_t* value, fc_size_t* size_out);

/**
 * @brief Convert bigfloat to string in scientific notation
 *
 * @param value Bigfloat to convert
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 *
 * @return FC_OK on success, FC_ERR_BUFFER_TOO_SMALL if buffer is too small
 */
FC_API fc_status_t
fc_bigfloat_to_string(const fc_bigfloat_t* value, char* buffer, fc_size_t buffer_size);

/**
 * @brief Add two bigfloats: result = lhs + rhs
 *
 * @param result Destination bigfloat
 * @param lhs Left operand
 * @param rhs Right operand
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_bigfloat_add(fc_bigfloat_t* result, const fc_bigfloat_t* lhs, const fc_bigfloat_t* rhs);

/**
 * @brief Subtract two bigfloats: result = lhs - rhs
 *
 * @param result Destination bigfloat
 * @param lhs Left operand
 * @param rhs Right operand
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_bigfloat_sub(fc_bigfloat_t* result, const fc_bigfloat_t* lhs, const fc_bigfloat_t* rhs);

/**
 * @brief Multiply two bigfloats: result = lhs * rhs
 *
 * @param result Destination bigfloat
 * @param lhs Left operand
 * @param rhs Right operand
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_bigfloat_mul(fc_bigfloat_t* result, const fc_bigfloat_t* lhs, const fc_bigfloat_t* rhs);

/**
 * @brief Divide two bigfloats: result = lhs / rhs
 *
 * @param result Destination bigfloat
 * @param lhs Left operand
 * @param rhs Right operand
 *
 * @return FC_OK on success, FC_ERR_DIVIDE_BY_ZERO if rhs is zero
 */
FC_API fc_status_t
fc_bigfloat_div(fc_bigfloat_t* result, const fc_bigfloat_t* lhs, const fc_bigfloat_t* rhs);

/**
 * @brief Negate a bigfloat: result = -input
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_neg(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Absolute value: result = |input|
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_abs(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Square root: result = sqrt(input)
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat (must be non-negative)
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if input is negative
 */
FC_API fc_status_t fc_bigfloat_sqrt(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Exponential function: result = e^input
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_exp(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Natural logarithm: result = ln(input)
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat (must be positive)
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if input is not positive
 */
FC_API fc_status_t fc_bigfloat_log(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Power function: result = base^exponent
 *
 * @param result Destination bigfloat
 * @param base Base value
 * @param exponent Exponent value
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_bigfloat_pow(fc_bigfloat_t* result, const fc_bigfloat_t* base, const fc_bigfloat_t* exponent);

/**
 * @brief Error function: result = erf(input)
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_erf(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Sine function: result = sin(input)
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat (in radians)
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_sin(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Cosine function: result = cos(input)
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat (in radians)
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_cos(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Tangent function: result = tan(input)
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat (in radians)
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_tan(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Exponential minus one: result = e^input - 1
 *
 * More accurate than exp(x) - 1 for small values of x.
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_expm1(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Logarithm of one plus: result = ln(1 + input)
 *
 * More accurate than log(1 + x) for small values of x.
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat (must be > -1)
 *
 * @return FC_OK on success, FC_ERR_INVALID_ARG if input is <= -1
 */
FC_API fc_status_t fc_bigfloat_log1p(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Ceiling function: result = ceil(input)
 *
 * Rounds input up to the nearest integer.
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_ceil(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Floor function: result = floor(input)
 *
 * Rounds input down to the nearest integer.
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_floor(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Round function: result = round(input)
 *
 * Rounds input to the nearest integer, ties to even.
 *
 * @param result Destination bigfloat
 * @param input Input bigfloat
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t fc_bigfloat_round(fc_bigfloat_t* result, const fc_bigfloat_t* input);

/**
 * @brief Compare two bigfloats
 *
 * @param lhs Left operand
 * @param rhs Right operand
 * @param[out] cmp_out Pointer to receive comparison result:
 *                     negative if lhs < rhs, zero if lhs == rhs, positive if lhs > rhs
 *
 * @return FC_OK on success, error code otherwise
 */
FC_API fc_status_t
fc_bigfloat_cmp(const fc_bigfloat_t* lhs, const fc_bigfloat_t* rhs, int* cmp_out);

FC_END_DECLS

#endif /* FC_BIGFLOAT_H */
