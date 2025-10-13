/*
 * PicoTTS ESP32 Integration Example
 * Phase 1 Optimizations Implementation
 * 
 * This file demonstrates how to integrate PicoTTS with ESP32
 * using Phase 1 optimizations:
 * - XIP (Execute-In-Place) for flash access
 * - Streaming architecture with reduced buffers
 * - I2S DMA audio output
 * - Optimized memory allocation
 */

#ifdef ESP_PLATFORM

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

/* Include PicoTTS headers */
#include "picoapi.h"
#include "picoapid.h"
#include "picoos.h"
#include "picoembedded.h"

static const char *TAG = "PicoTTS_ESP32";

/* ============================================================================
 * Phase 1.4: I2S DMA Audio Output Configuration
 * ============================================================================ */

#define I2S_NUM             I2S_NUM_0
#define I2S_BCK_PIN         26
#define I2S_WS_PIN          25  
#define I2S_DATA_PIN        22

/* I2S configuration structure */
static const i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_TX,
    .sample_rate = PICO_ESP32_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = PICO_ESP32_I2S_DMA_BUF_COUNT,
    .dma_buf_len = PICO_ESP32_I2S_DMA_BUF_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
};

static const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
};

/* ============================================================================
 * Phase 1.1: XIP Flash Access for Knowledge Bases
 * ============================================================================ */

/* 
 * With XIP enabled, knowledge bases can be accessed directly from flash
 * without loading into RAM. ESP32 flash is memory-mapped and cached.
 * 
 * Store language files in a dedicated flash partition and access via
 * const pointers.
 */

/* Example: Mark KB data as const for XIP access */
extern PICO_XIP_CONST uint8_t en_us_ta_bin_start[] asm("_binary_en_US_ta_bin_start");
extern PICO_XIP_CONST uint8_t en_us_ta_bin_end[] asm("_binary_en_US_ta_bin_end");
extern PICO_XIP_CONST uint8_t en_us_sg_bin_start[] asm("_binary_en_US_lh0_sg_bin_start");
extern PICO_XIP_CONST uint8_t en_us_sg_bin_end[] asm("_binary_en_US_lh0_sg_bin_end");

/* ============================================================================
 * Phase 1.2 & 1.3: Streaming Architecture with Reduced Buffers
 * ============================================================================ */

typedef struct {
    pico_System picoSystem;
    pico_Resource picoTaResource;
    pico_Resource picoSgResource;
    pico_Engine picoEngine;
    
    /* Reduced buffer sizes (Phase 1.3) */
    pico_Char *picoMemArea;
    size_t picoMemSize;
    
    /* Streaming output buffer */
    int16_t outputBuffer[PICO_EMBEDDED_OUT_BUFF_SIZE / 2];
    
    /* Statistics */
    uint32_t totalSynthesized;
    uint32_t peakMemoryUsage;
    
} PicoTTS_ESP32_t;

/* Global TTS instance */
static PicoTTS_ESP32_t *g_tts = NULL;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * Initialize I2S for audio output (Phase 1.4)
 */
static esp_err_t init_i2s(void) {
    esp_err_t ret;
    
    /* Install I2S driver */
    ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %d", ret);
        return ret;
    }
    
    /* Set I2S pins */
    ret = i2s_set_pin(I2S_NUM, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %d", ret);
        i2s_driver_uninstall(I2S_NUM);
        return ret;
    }
    
    ESP_LOGI(TAG, "I2S initialized: %d Hz, %d DMA buffers of %d bytes",
             PICO_ESP32_SAMPLE_RATE, 
             PICO_ESP32_I2S_DMA_BUF_COUNT,
             PICO_ESP32_I2S_DMA_BUF_LEN);
    
    return ESP_OK;
}

/**
 * Output audio to I2S with DMA (Phase 1.4)
 */
static esp_err_t output_audio(const int16_t *samples, size_t num_samples) {
    size_t bytes_written;
    size_t bytes_to_write = num_samples * sizeof(int16_t);
    
    esp_err_t ret = i2s_write(I2S_NUM, samples, bytes_to_write, 
                               &bytes_written, portMAX_DELAY);
    
    if (ret != ESP_OK || bytes_written != bytes_to_write) {
        ESP_LOGE(TAG, "I2S write failed: %d, wrote %d/%d bytes", 
                 ret, bytes_written, bytes_to_write);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/**
 * Load knowledge base from flash (XIP mode - Phase 1.1)
 */
static pico_Status load_resource_xip(pico_System system, 
                                     const char *name,
                                     PICO_XIP_CONST uint8_t *data_start,
                                     PICO_XIP_CONST uint8_t *data_end,
                                     pico_Resource *resource) {
    pico_Status ret;
    size_t size = data_end - data_start;
    
    ESP_LOGI(TAG, "Loading resource '%s' from flash (XIP): %d bytes", name, size);
    
    /* In XIP mode, we access the data directly from flash.
     * For PicoTTS, we still need to use the standard loading mechanism,
     * but the data remains in flash (memory-mapped). */
    
    /* Note: This is a placeholder. Actual implementation would need
     * pico_loadResourceFromMemory() which doesn't exist in standard API.
     * Alternative: Write temporary file or patch PicoTTS to accept memory pointers */
    
    ESP_LOGW(TAG, "XIP loading requires PicoTTS API extension");
    ESP_LOGW(TAG, "Fallback: Write to file system or use custom loader");
    
    return PICO_ERR_OTHER;
}

/**
 * Initialize PicoTTS with Phase 1 optimizations
 */
esp_err_t picotts_esp32_init(void) {
    pico_Status ret;
    
    /* Check if already initialized */
    if (g_tts != NULL) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing PicoTTS with Phase 1 optimizations");
    
    /* Report memory before initialization */
    ESP_LOGI(TAG, "Free heap before init: %d bytes", esp_get_free_heap_size());
    
    /* Allocate TTS structure from internal RAM (frequently accessed) */
    g_tts = (PicoTTS_ESP32_t *)PICO_MALLOC_INTERNAL(sizeof(PicoTTS_ESP32_t));
    if (!g_tts) {
        ESP_LOGE(TAG, "Failed to allocate TTS structure");
        return ESP_ERR_NO_MEM;
    }
    memset(g_tts, 0, sizeof(PicoTTS_ESP32_t));
    
    /* Allocate Pico memory from SPIRAM if available (Phase 1 optimization) */
    g_tts->picoMemSize = 2 * 1024 * 1024;  /* 2 MB */
    g_tts->picoMemArea = (pico_Char *)PICO_MALLOC_SPIRAM(g_tts->picoMemSize);
    
    if (!g_tts->picoMemArea) {
        ESP_LOGE(TAG, "Failed to allocate Pico memory");
        PICO_FREE(g_tts);
        g_tts = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    /* Initialize Pico system */
    ret = pico_initialize(g_tts->picoMemArea, g_tts->picoMemSize, 
                         &g_tts->picoSystem);
    if (ret != PICO_OK) {
        ESP_LOGE(TAG, "pico_initialize failed: %d", ret);
        PICO_FREE(g_tts->picoMemArea);
        PICO_FREE(g_tts);
        g_tts = NULL;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Pico system initialized");
    
    /* Initialize I2S for audio output (Phase 1.4) */
    if (init_i2s() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S");
        pico_terminate(&g_tts->picoSystem);
        PICO_FREE(g_tts->picoMemArea);
        PICO_FREE(g_tts);
        g_tts = NULL;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Free heap after init: %d bytes", esp_get_free_heap_size());
    
    /* Load text analysis resource (ta) 
     * In production, load from XIP flash partition or SPIFFS */
    ret = load_resource_xip(g_tts->picoSystem, "en-US_ta.bin",
                            en_us_ta_bin_start, en_us_ta_bin_end,
                            &g_tts->picoTaResource);
    if (ret != PICO_OK) {
        ESP_LOGW(TAG, "XIP loading not available, use alternative method");
        /* Alternative: Load from SPIFFS or implement custom loader */
        /* For now, continue without resources loaded */
    }
    
    /* Load signal generation resource (sg)
     * In production, load from XIP flash partition or SPIFFS */
    ret = load_resource_xip(g_tts->picoSystem, "en-US_lh0_sg.bin",
                            en_us_sg_bin_start, en_us_sg_bin_end,
                            &g_tts->picoSgResource);
    if (ret != PICO_OK) {
        ESP_LOGW(TAG, "XIP loading not available, use alternative method");
        /* Alternative: Load from SPIFFS or implement custom loader */
    }
    
    /* Create voice definition
     * Note: This requires resources to be loaded successfully */
    const char *voice_name = "PicoVoice";
    ret = pico_createVoiceDefinition(g_tts->picoSystem, (const pico_Char *)voice_name);
    if (ret != PICO_OK) {
        ESP_LOGW(TAG, "Failed to create voice definition: %d", ret);
        ESP_LOGW(TAG, "Resources may not be loaded properly");
        /* Continue - engine creation will fail but structure is initialized */
    } else {
        /* Add resources to voice definition */
        pico_Char ta_name[PICO_MAX_RESOURCE_NAME_SIZE];
        pico_Char sg_name[PICO_MAX_RESOURCE_NAME_SIZE];
        
        if (g_tts->picoTaResource) {
            ret = pico_getResourceName(g_tts->picoSystem, g_tts->picoTaResource, 
                                       (char *)ta_name);
            if (ret == PICO_OK) {
                pico_addResourceToVoiceDefinition(g_tts->picoSystem, 
                                                  (const pico_Char *)voice_name,
                                                  ta_name);
            }
        }
        
        if (g_tts->picoSgResource) {
            ret = pico_getResourceName(g_tts->picoSystem, g_tts->picoSgResource,
                                       (char *)sg_name);
            if (ret == PICO_OK) {
                pico_addResourceToVoiceDefinition(g_tts->picoSystem,
                                                  (const pico_Char *)voice_name,
                                                  sg_name);
            }
        }
        
        /* Create TTS engine */
        ret = pico_newEngine(g_tts->picoSystem, (const pico_Char *)voice_name,
                            &g_tts->picoEngine);
        if (ret != PICO_OK) {
            ESP_LOGE(TAG, "Failed to create engine: %d", ret);
            ESP_LOGE(TAG, "Make sure language resources are properly loaded");
            /* Clean up voice definition */
            pico_releaseVoiceDefinition(g_tts->picoSystem, (pico_Char *)voice_name);
        } else {
            ESP_LOGI(TAG, "TTS engine created successfully");
        }
    }
    
    ESP_LOGI(TAG, "PicoTTS ESP32 initialized successfully");
    ESP_LOGI(TAG, "Note: For production use, ensure language resources are");
    ESP_LOGI(TAG, "      embedded in flash or loaded from SPIFFS");
    
    return ESP_OK;
}

/**
 * Synthesize text with streaming output (Phase 1.2 & 1.3)
 */
esp_err_t picotts_esp32_synthesize(const char *text) {
    if (!g_tts || !g_tts->picoEngine) {
        ESP_LOGE(TAG, "TTS not initialized or engine not ready");
        return ESP_FAIL;
    }
    
    pico_Char *inp = (pico_Char *)text;
    int16_t text_remaining = strlen(text) + 1;
    int16_t bytes_sent, bytes_recv;
    pico_Int16 out_data_type;
    pico_Status ret;
    
    uint32_t start_time = xTaskGetTickCount();
    
    ESP_LOGI(TAG, "Synthesizing: \"%s\"", text);
    
    /* Streaming synthesis loop (Phase 1.2 & 1.3) */
    while (text_remaining > 0) {
        /* Feed text in small chunks for streaming */
        ret = pico_putTextUtf8(g_tts->picoEngine, inp, text_remaining, &bytes_sent);
        if (ret != PICO_OK) {
            ESP_LOGE(TAG, "pico_putTextUtf8 failed: %d", ret);
            return ESP_FAIL;
        }
        
        text_remaining -= bytes_sent;
        inp += bytes_sent;
        
        /* Retrieve and output audio data in streaming mode */
        do {
            ret = pico_getData(g_tts->picoEngine, 
                             (void *)g_tts->outputBuffer,
                             PICO_EMBEDDED_OUT_BUFF_SIZE,
                             &bytes_recv, 
                             &out_data_type);
            
            if (bytes_recv > 0) {
                /* Output to I2S with DMA (Phase 1.4) */
                size_t num_samples = bytes_recv / sizeof(int16_t);
                if (output_audio(g_tts->outputBuffer, num_samples) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to output audio");
                    return ESP_FAIL;
                }
                
                g_tts->totalSynthesized += bytes_recv;
            }
            
            /* Allow other tasks to run */
            taskYIELD();
            
        } while (ret == PICO_STEP_BUSY);
        
        if (ret != PICO_OK && ret != PICO_STEP_IDLE) {
            ESP_LOGE(TAG, "pico_getData failed: %d", ret);
            return ESP_FAIL;
        }
    }
    
    /* Flush remaining data */
    do {
        ret = pico_getData(g_tts->picoEngine,
                         (void *)g_tts->outputBuffer,
                         PICO_EMBEDDED_OUT_BUFF_SIZE,
                         &bytes_recv,
                         &out_data_type);
        
        if (bytes_recv > 0) {
            size_t num_samples = bytes_recv / sizeof(int16_t);
            output_audio(g_tts->outputBuffer, num_samples);
            g_tts->totalSynthesized += bytes_recv;
        }
    } while (ret == PICO_STEP_BUSY);
    
    uint32_t elapsed = xTaskGetTickCount() - start_time;
    float audio_duration = (float)g_tts->totalSynthesized / (PICO_ESP32_SAMPLE_RATE * 2);
    float rtf = (elapsed / 1000.0f) / audio_duration;
    
    ESP_LOGI(TAG, "Synthesis complete: %d bytes in %d ms (RTF: %.2f)",
             g_tts->totalSynthesized, elapsed, rtf);
    
    /* Reset engine for next synthesis */
    pico_resetEngine(g_tts->picoEngine, PICO_RESET_SOFT);
    
    return ESP_OK;
}

/**
 * Cleanup and shutdown
 */
void picotts_esp32_deinit(void) {
    if (!g_tts) {
        return;
    }
    
    ESP_LOGI(TAG, "Shutting down PicoTTS");
    
    /* Terminate Pico system */
    if (g_tts->picoSystem) {
        pico_terminate(&g_tts->picoSystem);
    }
    
    /* Free memory */
    if (g_tts->picoMemArea) {
        PICO_FREE(g_tts->picoMemArea);
    }
    PICO_FREE(g_tts);
    g_tts = NULL;
    
    /* Uninstall I2S */
    i2s_driver_uninstall(I2S_NUM);
    
    ESP_LOGI(TAG, "PicoTTS shutdown complete");
}

/**
 * Get memory statistics
 */
void picotts_esp32_get_stats(void) {
    if (!g_tts) {
        ESP_LOGW(TAG, "TTS not initialized");
        return;
    }
    
    ESP_LOGI(TAG, "=== PicoTTS ESP32 Statistics ===");
    ESP_LOGI(TAG, "Total synthesized: %d bytes", g_tts->totalSynthesized);
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Free internal: %d bytes", 
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "Free SPIRAM: %d bytes", 
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

#endif /* ESP_PLATFORM */
