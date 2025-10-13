/*
 * Copyright (C) 2025 - Embedded Systems Optimizations
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
 * @file picoembedded.h
 *
 * Configuration for embedded systems optimization (Phase 1)
 * Provides compile-time flags for memory-constrained environments
 * like ESP32, with support for:
 * - Reduced buffer sizes (streaming architecture)
 * - XIP (Execute-In-Place) flash access
 * - Configurable memory limits
 *
 * Usage:
 *   Define PICO_EMBEDDED_TARGET before including this header:
 *   - PICO_EMBEDDED_ESP32: ESP32 optimization profile
 *   - PICO_EMBEDDED_MINIMAL: Minimal memory profile
 *   - PICO_EMBEDDED_CUSTOM: Custom configuration
 */

#ifndef PICOEMBEDDED_H_
#define PICOEMBEDDED_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Platform Detection and Configuration Profiles
 * ============================================================================ */

/* Detect ESP32 platform */
#if defined(ESP32) || defined(ESP_PLATFORM) || defined(PICO_EMBEDDED_ESP32)
    #define PICO_EMBEDDED_PLATFORM 1
    #define PICO_EMBEDDED_ESP32_TARGET 1
#endif

/* Detect other embedded platforms */
#if defined(__arm__) || defined(__thumb__) || defined(PICO_EMBEDDED_MINIMAL)
    #ifndef PICO_EMBEDDED_PLATFORM
        #define PICO_EMBEDDED_PLATFORM 1
    #endif
#endif

/* ============================================================================
 * Buffer Size Configuration (Phase 1.3: Streaming Architecture)
 * ============================================================================ */

#ifdef PICO_EMBEDDED_PLATFORM

    /* Reduced buffer sizes for streaming architecture */
    #ifndef PICO_EMBEDDED_IN_BUFF_SIZE
        #define PICO_EMBEDDED_IN_BUFF_SIZE      512     /* Was 2048 */
    #endif
    
    #ifndef PICO_EMBEDDED_OUT_BUFF_SIZE
        #define PICO_EMBEDDED_OUT_BUFF_SIZE     512     /* Was 2048 */
    #endif
    
    #ifndef PICO_EMBEDDED_SIG_BUFF_SIZE
        #define PICO_EMBEDDED_SIG_BUFF_SIZE     512     /* Was 2048 */
    #endif
    
    /* Streaming phoneme buffer (reduced from 400 to 32 phonemes) */
    #ifndef PICO_EMBEDDED_MAX_PH_PER_CHUNK
        #define PICO_EMBEDDED_MAX_PH_PER_CHUNK  32      /* Was 400 */
    #endif
    
    /* Lookahead for prosody prediction */
    #ifndef PICO_EMBEDDED_PHONEME_LOOKAHEAD
        #define PICO_EMBEDDED_PHONEME_LOOKAHEAD 4
    #endif
    
    /* Enable streaming mode */
    #ifndef PICO_EMBEDDED_STREAMING_MODE
        #define PICO_EMBEDDED_STREAMING_MODE    1
    #endif

#else

    /* Default buffer sizes for desktop/server */
    #define PICO_EMBEDDED_IN_BUFF_SIZE      2048
    #define PICO_EMBEDDED_OUT_BUFF_SIZE     2048
    #define PICO_EMBEDDED_SIG_BUFF_SIZE     2048
    #define PICO_EMBEDDED_MAX_PH_PER_CHUNK  400
    #define PICO_EMBEDDED_PHONEME_LOOKAHEAD 0
    #define PICO_EMBEDDED_STREAMING_MODE    0

#endif

/* ============================================================================
 * XIP (Execute-In-Place) Support (Phase 1.1)
 * ============================================================================ */

/* Enable XIP mode for ESP32 (access knowledge bases directly from flash) */
#ifdef PICO_EMBEDDED_ESP32_TARGET
    #ifndef PICO_EMBEDDED_XIP_ENABLE
        #define PICO_EMBEDDED_XIP_ENABLE 1
    #endif
#endif

/* XIP macros for marking read-only data that can stay in flash */
#ifdef PICO_EMBEDDED_XIP_ENABLE
    /* ESP32-specific: use DRAM_ATTR for frequently accessed data,
       otherwise let it stay in flash */
    #ifdef ESP_PLATFORM
        #include "esp_attr.h"
        #define PICO_XIP_DATA       /* Data can stay in flash (cached access) */
        #define PICO_XIP_CONST      const  /* Explicitly const for XIP */
        #define PICO_RAM_DATA       DRAM_ATTR  /* Force to RAM if needed */
    #else
        #define PICO_XIP_DATA       const
        #define PICO_XIP_CONST      const
        #define PICO_RAM_DATA       
    #endif
#else
    #define PICO_XIP_DATA
    #define PICO_XIP_CONST
    #define PICO_RAM_DATA
#endif

/* ============================================================================
 * Memory Allocation Hints (Phase 1)
 * ============================================================================ */

#ifdef PICO_EMBEDDED_ESP32_TARGET
    /* ESP32 has multiple heaps - provide hints for optimal placement */
    #ifdef ESP_PLATFORM
        #include "esp_heap_caps.h"
        
        /* Allocate from SPIRAM if available (for large buffers) */
        #define PICO_MALLOC_SPIRAM(size) \
            heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
        
        /* Allocate from internal RAM (for frequently accessed data) */
        #define PICO_MALLOC_INTERNAL(size) \
            heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
        
        /* Allocate DMA-capable memory (for I2S audio output) */
        #define PICO_MALLOC_DMA(size) \
            heap_caps_malloc(size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT)
            
        #define PICO_FREE(ptr) heap_caps_free(ptr)
    #else
        #define PICO_MALLOC_SPIRAM(size)    malloc(size)
        #define PICO_MALLOC_INTERNAL(size)  malloc(size)
        #define PICO_MALLOC_DMA(size)       malloc(size)
        #define PICO_FREE(ptr)              free(ptr)
    #endif
#else
    /* Standard allocation for non-ESP32 platforms */
    #define PICO_MALLOC_SPIRAM(size)    malloc(size)
    #define PICO_MALLOC_INTERNAL(size)  malloc(size)
    #define PICO_MALLOC_DMA(size)       malloc(size)
    #define PICO_FREE(ptr)              free(ptr)
#endif

/* ============================================================================
 * Performance Configuration
 * ============================================================================ */

/* Enable optimizations for embedded platforms */
#ifdef PICO_EMBEDDED_PLATFORM
    /* Use smaller decision tree cache */
    #ifndef PICO_EMBEDDED_TREE_CACHE_SIZE
        #define PICO_EMBEDDED_TREE_CACHE_SIZE 128  /* Reduced from potential larger */
    #endif
    
    /* Limit max sentence length */
    #ifndef PICO_EMBEDDED_MAX_SENTENCE_LENGTH
        #define PICO_EMBEDDED_MAX_SENTENCE_LENGTH 512  /* Characters */
    #endif
#endif

/* ============================================================================
 * Debug and Profiling (can be disabled to save memory)
 * ============================================================================ */

#ifdef PICO_EMBEDDED_MINIMAL
    /* Disable debug output to save code size */
    #ifndef PICO_EMBEDDED_NO_DEBUG
        #define PICO_EMBEDDED_NO_DEBUG 1
    #endif
#endif

/* ============================================================================
 * ESP32-Specific I2S Audio Output Configuration (Phase 1.4)
 * ============================================================================ */

#ifdef PICO_EMBEDDED_ESP32_TARGET
    /* I2S DMA buffer configuration */
    #ifndef PICO_ESP32_I2S_DMA_BUF_COUNT
        #define PICO_ESP32_I2S_DMA_BUF_COUNT    4
    #endif
    
    #ifndef PICO_ESP32_I2S_DMA_BUF_LEN
        #define PICO_ESP32_I2S_DMA_BUF_LEN      256
    #endif
    
    /* Sample rate */
    #ifndef PICO_ESP32_SAMPLE_RATE
        #define PICO_ESP32_SAMPLE_RATE          16000
    #endif
#endif

/* ============================================================================
 * Feature Flags
 * ============================================================================ */

/* Enable/disable features based on platform */
#ifdef PICO_EMBEDDED_MINIMAL
    #define PICO_EMBEDDED_DISABLE_FILE_IO   1  /* No file I/O, XIP only */
#endif

/* ============================================================================
 * Helper Macros
 * ============================================================================ */

/* Check if we're in embedded mode */
#define PICO_IS_EMBEDDED() (defined(PICO_EMBEDDED_PLATFORM))

/* Check if XIP is enabled */
#define PICO_IS_XIP_ENABLED() (defined(PICO_EMBEDDED_XIP_ENABLE))

/* Get effective buffer size */
#define PICO_GET_IN_BUFF_SIZE()  PICO_EMBEDDED_IN_BUFF_SIZE
#define PICO_GET_OUT_BUFF_SIZE() PICO_EMBEDDED_OUT_BUFF_SIZE
#define PICO_GET_SIG_BUFF_SIZE() PICO_EMBEDDED_SIG_BUFF_SIZE

#ifdef __cplusplus
}
#endif

#endif /* PICOEMBEDDED_H_ */
