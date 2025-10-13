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
 * @file picofixedpoint.c
 *
 * Fixed-point arithmetic implementation
 */

#include "picofixedpoint.h"

/**
 * Fast Q15 approximate square root using Newton-Raphson
 * Converges in 3-4 iterations
 */
pico_q15_t pico_q15_sqrt_approx(pico_q15_t x) {
    if (x <= 0) return 0;
    if (x == PICO_Q15_ONE) return PICO_Q15_ONE;
    
    /* Initial guess: x/2 */
    pico_q31_t guess = ((pico_q31_t)x) >> 1;
    
    /* Newton-Raphson: x_new = (x_old + x/x_old) / 2 */
    for (int i = 0; i < 4; i++) {
        pico_q31_t x_scaled = ((pico_q31_t)x << PICO_Q15_SHIFT);
        pico_q31_t div = x_scaled / guess;
        guess = (guess + div) >> 1;
    }
    
    return (pico_q15_t)guess;
}

/**
 * Fast Q15 approximate reciprocal using Newton-Raphson
 * Calculates 1/x
 */
pico_q15_t pico_q15_recip_approx(pico_q15_t x) {
    if (x == 0) return PICO_Q15_MAX;
    if (x == PICO_Q15_ONE) return PICO_Q15_ONE;
    
    /* Initial guess using bit manipulation */
    pico_q31_t guess;
    if (x > 0) {
        guess = ((pico_q31_t)PICO_Q15_ONE << PICO_Q15_SHIFT) / x;
    } else {
        guess = -((pico_q31_t)PICO_Q15_ONE << PICO_Q15_SHIFT) / (-x);
    }
    
    /* Newton-Raphson: x_new = x_old * (2 - a * x_old) */
    for (int i = 0; i < 3; i++) {
        pico_q31_t prod = ((pico_q31_t)x * guess) >> PICO_Q15_SHIFT;
        pico_q31_t diff = (2 << PICO_Q15_SHIFT) - prod;
        guess = (guess * diff) >> PICO_Q15_SHIFT;
    }
    
    return (pico_q15_t)guess;
}
