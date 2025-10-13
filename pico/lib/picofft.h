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
 * @file picofft.h
 *
 * FFT abstraction layer for platform-optimized implementations (Phase 2)
 * 
 * Provides unified FFT interface that can use:
 * - Generic implementation (picofftsg.c)
 * - ESP-DSP optimized FFT (ESP32)
 * - ARM CMSIS-DSP FFT (ARM Cortex)
 * - Other platform-specific optimizations
 * 
 * Expected performance gain: 40-60% on ESP32 with ESP-DSP
 * 
 * Usage:
 *   Enable ESP-DSP with: -DPICO_USE_ESP_DSP=1
 *   Enable CMSIS-DSP with: -DPICO_USE_CMSIS_DSP=1
 */

#ifndef PICOFFT_H_
#define PICOFFT_H_

#include "picodefs.h"
#include "picoos.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FFT Configuration
 * ============================================================================ */

/* FFT sizes used in PicoTTS */
#define PICO_FFT_SIZE_256  256
#define PICO_FFT_SIZE_512  512

/* Detect and configure platform-specific FFT */
#if defined(ESP_PLATFORM) && defined(PICO_USE_ESP_DSP)
    #define PICO_FFT_USE_ESP_DSP 1
    #include "esp_dsp.h"
#elif defined(ARM_MATH_CM4) && defined(PICO_USE_CMSIS_DSP)
    #define PICO_FFT_USE_CMSIS_DSP 1
    #include "arm_math.h"
#else
    #define PICO_FFT_USE_GENERIC 1
#endif

/* ============================================================================
 * FFT Context Structure
 * ============================================================================ */

typedef struct pico_fft_context {
    picoos_uint16 fft_size;        /* FFT size (256 or 512) */
    picoos_uint8  initialized;     /* Is context initialized? */
    
#ifdef PICO_FFT_USE_ESP_DSP
    /* ESP-DSP specific data */
    float *window;                 /* Window coefficients */
    float *twiddle_factors;        /* Pre-computed twiddle factors */
#elif defined(PICO_FFT_USE_CMSIS_DSP)
    /* CMSIS-DSP specific data */
    arm_rfft_fast_instance_f32 fft_instance;
#else
    /* Generic implementation needs no extra data */
    void *reserved;
#endif
} pico_fft_context_t;

/* ============================================================================
 * FFT API
 * ============================================================================ */

/**
 * Initialize FFT context
 * @param mm Memory manager
 * @param fft_size FFT size (must be power of 2: 256 or 512)
 * @param context Output: initialized FFT context
 * @return PICO_OK on success
 */
picoos_uint8 pico_fft_initialize(picoos_MemoryManager mm,
                                 picoos_uint16 fft_size,
                                 pico_fft_context_t **context);

/**
 * Deallocate FFT context
 * @param mm Memory manager
 * @param context FFT context to deallocate
 */
void pico_fft_deallocate(picoos_MemoryManager mm,
                         pico_fft_context_t **context);

/**
 * Perform real-to-complex FFT
 * 
 * Input: real[0..N-1] (time domain)
 * Output: real[0..N-1], imag[0..N-1] (frequency domain)
 * 
 * Note: real and imag can point to same array for in-place operation
 * 
 * @param context FFT context
 * @param real Input real samples, output real part of FFT
 * @param imag Output imaginary part of FFT
 * @return PICO_OK on success
 */
picoos_uint8 pico_fft_forward(pico_fft_context_t *context,
                              float *real,
                              float *imag);

/**
 * Perform complex-to-real inverse FFT
 * 
 * Input: real[0..N-1], imag[0..N-1] (frequency domain)
 * Output: real[0..N-1] (time domain)
 * 
 * @param context FFT context
 * @param real Input real part, output time domain samples
 * @param imag Input imaginary part
 * @return PICO_OK on success
 */
picoos_uint8 pico_fft_inverse(pico_fft_context_t *context,
                              float *real,
                              float *imag);

/**
 * Compute magnitude spectrum from FFT result
 * mag[i] = sqrt(real[i]^2 + imag[i]^2)
 * 
 * @param real Real part of FFT
 * @param imag Imaginary part of FFT
 * @param mag Output magnitude spectrum
 * @param size FFT size
 */
void pico_fft_magnitude(const float *real,
                        const float *imag,
                        float *mag,
                        picoos_uint16 size);

/**
 * Compute power spectrum from FFT result
 * power[i] = real[i]^2 + imag[i]^2
 * 
 * @param real Real part of FFT
 * @param imag Imaginary part of FFT
 * @param power Output power spectrum
 * @param size FFT size
 */
void pico_fft_power(const float *real,
                    const float *imag,
                    float *power,
                    picoos_uint16 size);

/* ============================================================================
 * Window Functions
 * ============================================================================ */

/**
 * Apply Hamming window to signal
 * @param signal Input/output signal
 * @param size Signal size
 */
void pico_fft_hamming_window(float *signal, picoos_uint16 size);

/**
 * Apply Hann window to signal
 * @param signal Input/output signal
 * @param size Signal size
 */
void pico_fft_hann_window(float *signal, picoos_uint16 size);

/**
 * Apply Blackman window to signal
 * @param signal Input/output signal
 * @param size Signal size
 */
void pico_fft_blackman_window(float *signal, picoos_uint16 size);

/* ============================================================================
 * Platform-Specific Optimizations
 * ============================================================================ */

#ifdef PICO_FFT_USE_ESP_DSP

/**
 * Initialize ESP-DSP FFT tables
 * Called automatically by pico_fft_initialize()
 */
picoos_uint8 pico_fft_esp_dsp_init_tables(pico_fft_context_t *context);

/**
 * Get ESP-DSP performance info
 * @param context FFT context
 * @param cycles Output: estimated CPU cycles per FFT
 */
void pico_fft_esp_dsp_get_perf(pico_fft_context_t *context,
                               picoos_uint32 *cycles);

#endif /* PICO_FFT_USE_ESP_DSP */

#ifdef PICO_FFT_USE_CMSIS_DSP

/**
 * Initialize CMSIS-DSP FFT instance
 * Called automatically by pico_fft_initialize()
 */
picoos_uint8 pico_fft_cmsis_dsp_init(pico_fft_context_t *context);

#endif /* PICO_FFT_USE_CMSIS_DSP */

/* ============================================================================
 * Inline Helpers
 * ============================================================================ */

/**
 * Check if FFT context is valid
 */
static inline picoos_uint8 pico_fft_is_valid(pico_fft_context_t *context) {
    return (context != NULL && context->initialized);
}

/**
 * Get FFT size from context
 */
static inline picoos_uint16 pico_fft_get_size(pico_fft_context_t *context) {
    return context ? context->fft_size : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* PICOFFT_H_ */
