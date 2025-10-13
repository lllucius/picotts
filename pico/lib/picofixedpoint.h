/*
 * Copyright (C) 2025 - Phase 2 Performance Optimizations
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file picofixedpoint.h
 *
 * Fixed-point arithmetic support for embedded systems (Phase 2)
 * Provides Q15 and Q31 fixed-point operations for DSP algorithms
 * to replace floating-point math with faster integer operations.
 *
 * Q15 format: 1 sign bit + 15 fractional bits (range: -1.0 to ~1.0)
 * Q31 format: 1 sign bit + 31 fractional bits (range: -1.0 to ~1.0)
 *
 * Usage:
 *   Enable with: -DPICO_USE_FIXED_POINT=1
 */

#ifndef PICOFIXEDPOINT_H_
#define PICOFIXEDPOINT_H_

#include "picodefs.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Fixed-Point Configuration
 * ============================================================================ */

/* Enable fixed-point arithmetic (Phase 2 optimization) */
#if defined(PICO_USE_FIXED_POINT) || defined(PICO_EMBEDDED_PLATFORM)
    #define PICO_FIXED_POINT_ENABLED 1
#endif

/* Q15 format: 16-bit fixed point with 15 fractional bits */
typedef int16_t pico_q15_t;
#define PICO_Q15_SHIFT 15
#define PICO_Q15_ONE (1 << PICO_Q15_SHIFT)
#define PICO_Q15_MIN ((int16_t)0x8000)
#define PICO_Q15_MAX ((int16_t)0x7FFF)

/* Q31 format: 32-bit fixed point with 31 fractional bits */
typedef int32_t pico_q31_t;
#define PICO_Q31_SHIFT 31
#define PICO_Q31_ONE (1 << PICO_Q31_SHIFT)
#define PICO_Q31_MIN ((int32_t)0x80000000)
#define PICO_Q31_MAX ((int32_t)0x7FFFFFFF)

/* ============================================================================
 * Q15 Fixed-Point Operations
 * ============================================================================ */

/**
 * Convert float to Q15
 * @param f Float value in range [-1.0, 1.0)
 * @return Q15 fixed-point value
 */
static inline pico_q15_t pico_float_to_q15(float f) {
    if (f >= 1.0f) return PICO_Q15_MAX;
    if (f <= -1.0f) return PICO_Q15_MIN;
    return (pico_q15_t)(f * PICO_Q15_ONE);
}

/**
 * Convert Q15 to float
 * @param q Q15 fixed-point value
 * @return Float value
 */
static inline float pico_q15_to_float(pico_q15_t q) {
    return (float)q / (float)PICO_Q15_ONE;
}

/**
 * Q15 multiplication
 * result = (a * b) >> 15
 * @param a First Q15 operand
 * @param b Second Q15 operand
 * @return Q15 result
 */
static inline pico_q15_t pico_q15_mult(pico_q15_t a, pico_q15_t b) {
    return (pico_q15_t)(((int32_t)a * (int32_t)b) >> PICO_Q15_SHIFT);
}

/**
 * Q15 multiply-accumulate
 * acc = acc + (a * b) >> 15
 * @param acc Accumulator (Q15)
 * @param a First Q15 operand
 * @param b Second Q15 operand
 * @return Q15 result
 */
static inline pico_q15_t pico_q15_mac(pico_q15_t acc, pico_q15_t a, pico_q15_t b) {
    return acc + pico_q15_mult(a, b);
}

/**
 * Q15 addition with saturation
 * @param a First Q15 operand
 * @param b Second Q15 operand
 * @return Q15 result (saturated)
 */
static inline pico_q15_t pico_q15_add_sat(pico_q15_t a, pico_q15_t b) {
    int32_t result = (int32_t)a + (int32_t)b;
    if (result > PICO_Q15_MAX) return PICO_Q15_MAX;
    if (result < PICO_Q15_MIN) return PICO_Q15_MIN;
    return (pico_q15_t)result;
}

/**
 * Q15 subtraction with saturation
 * @param a First Q15 operand
 * @param b Second Q15 operand
 * @return Q15 result (saturated)
 */
static inline pico_q15_t pico_q15_sub_sat(pico_q15_t a, pico_q15_t b) {
    int32_t result = (int32_t)a - (int32_t)b;
    if (result > PICO_Q15_MAX) return PICO_Q15_MAX;
    if (result < PICO_Q15_MIN) return PICO_Q15_MIN;
    return (pico_q15_t)result;
}

/* ============================================================================
 * Q31 Fixed-Point Operations
 * ============================================================================ */

/**
 * Convert float to Q31
 * @param f Float value in range [-1.0, 1.0)
 * @return Q31 fixed-point value
 */
static inline pico_q31_t pico_float_to_q31(float f) {
    if (f >= 1.0f) return PICO_Q31_MAX;
    if (f <= -1.0f) return PICO_Q31_MIN;
    return (pico_q31_t)(f * (double)PICO_Q31_ONE);
}

/**
 * Convert Q31 to float
 * @param q Q31 fixed-point value
 * @return Float value
 */
static inline float pico_q31_to_float(pico_q31_t q) {
    return (float)((double)q / (double)PICO_Q31_ONE);
}

/**
 * Q31 multiplication
 * result = (a * b) >> 31
 * @param a First Q31 operand
 * @param b Second Q31 operand
 * @return Q31 result
 */
static inline pico_q31_t pico_q31_mult(pico_q31_t a, pico_q31_t b) {
    return (pico_q31_t)(((int64_t)a * (int64_t)b) >> PICO_Q31_SHIFT);
}

/* ============================================================================
 * DSP Helper Functions
 * ============================================================================ */

/**
 * Dot product using Q15 fixed-point
 * result = sum(a[i] * b[i]) for i in [0, len)
 * @param a First Q15 array
 * @param b Second Q15 array
 * @param len Array length
 * @return Q31 result (extended precision for accumulation)
 */
static inline pico_q31_t pico_q15_dot_product(const pico_q15_t *a, 
                                               const pico_q15_t *b, 
                                               int len) {
    pico_q31_t acc = 0;
    for (int i = 0; i < len; i++) {
        acc += ((pico_q31_t)a[i] * (pico_q31_t)b[i]);
    }
    return acc >> PICO_Q15_SHIFT;
}

/**
 * Vector scale using Q15 fixed-point
 * out[i] = in[i] * scale for i in [0, len)
 * @param in Input Q15 array
 * @param scale Q15 scale factor
 * @param out Output Q15 array
 * @param len Array length
 */
static inline void pico_q15_vector_scale(const pico_q15_t *in,
                                         pico_q15_t scale,
                                         pico_q15_t *out,
                                         int len) {
    for (int i = 0; i < len; i++) {
        out[i] = pico_q15_mult(in[i], scale);
    }
}

/**
 * Vector add using Q15 fixed-point
 * out[i] = a[i] + b[i] for i in [0, len)
 * @param a First Q15 array
 * @param b Second Q15 array
 * @param out Output Q15 array
 * @param len Array length
 */
static inline void pico_q15_vector_add(const pico_q15_t *a,
                                       const pico_q15_t *b,
                                       pico_q15_t *out,
                                       int len) {
    for (int i = 0; i < len; i++) {
        out[i] = pico_q15_add_sat(a[i], b[i]);
    }
}

/* ============================================================================
 * Conditional Compilation Support
 * ============================================================================ */

#ifdef PICO_FIXED_POINT_ENABLED
    /* Use fixed-point implementations */
    #define PICO_DSP_TYPE           pico_q15_t
    #define PICO_DSP_FLOAT_TO(f)    pico_float_to_q15(f)
    #define PICO_DSP_TO_FLOAT(q)    pico_q15_to_float(q)
    #define PICO_DSP_MULT(a, b)     pico_q15_mult(a, b)
    #define PICO_DSP_ADD(a, b)      pico_q15_add_sat(a, b)
    #define PICO_DSP_SUB(a, b)      pico_q15_sub_sat(a, b)
#else
    /* Use floating-point implementations */
    #define PICO_DSP_TYPE           float
    #define PICO_DSP_FLOAT_TO(f)    (f)
    #define PICO_DSP_TO_FLOAT(f)    (f)
    #define PICO_DSP_MULT(a, b)     ((a) * (b))
    #define PICO_DSP_ADD(a, b)      ((a) + (b))
    #define PICO_DSP_SUB(a, b)      ((a) - (b))
#endif

/* ============================================================================
 * Fast Approximations (Phase 2)
 * ============================================================================ */

/**
 * Fast Q15 approximate square root
 * Uses Newton-Raphson iteration
 * @param x Q15 input (must be positive)
 * @return Q15 approximate square root
 */
pico_q15_t pico_q15_sqrt_approx(pico_q15_t x);

/**
 * Fast Q15 approximate reciprocal
 * Uses Newton-Raphson iteration
 * @param x Q15 input (must be non-zero)
 * @return Q15 approximate 1/x
 */
pico_q15_t pico_q15_recip_approx(pico_q15_t x);

#ifdef __cplusplus
}
#endif

#endif /* PICOFIXEDPOINT_H_ */
