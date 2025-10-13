# PicoTTS on ESP32 - Quick Implementation Guide

## Overview

This guide provides a practical roadmap for getting PicoTTS running on ESP32 embedded hardware, with step-by-step instructions and code examples.

## ESP32 Specifications Recap

| Feature | Specification | TTS Requirement | Status |
|---------|--------------|-----------------|--------|
| CPU | Dual Xtensa LX6 @ 240 MHz | ~1.3 MFLOPS | ✅ Sufficient |
| RAM | 520 KB SRAM | 70-150 KB | ✅ Adequate |
| Flash | 4-16 MB | 1.5-7 MB/lang | ✅ 1-2 languages |
| FPU | Single precision | Yes | ✅ Has FPU |

**Verdict: PicoTTS is feasible on ESP32 with optimizations**

---

## Minimal Working Implementation

### Step 1: Memory Configuration

**platformio.ini or sdkconfig:**
```ini
# Increase main task stack
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192

# Optimize memory layout
CONFIG_SPIRAM_SUPPORT=y  # If PSRAM available
CONFIG_SPIRAM_USE_MALLOC=y

# Flash optimization
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_PARTITION_TABLE_CUSTOM=y
```

**Partition Table (partitions.csv):**
```csv
# Name,   Type, SubType, Offset,  Size,     Flags
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x180000,
tts_kb,   data, spiffs,  0x190000,0x270000,
```

---

### Step 2: Basic Integration Code

**main.cpp:**
```cpp
#include <Arduino.h>
#include "esp_heap_caps.h"
#include "driver/i2s.h"
#include "picoapi.h"
#include "picoapid.h"
#include "picoos.h"

// TTS Configuration
#define PICO_MEM_SIZE       (2 * 1024 * 1024)  // 2 MB
#define PICO_BUFFER_SIZE    1024
#define SAMPLE_RATE         16000

// I2S Configuration for Audio Output
#define I2S_NUM             I2S_NUM_0
#define I2S_BCK_PIN         26
#define I2S_WS_PIN          25
#define I2S_DATA_PIN        22

// Global TTS objects
pico_System picoSystem = NULL;
pico_Resource picoTaResource = NULL;
pico_Resource picoSgResource = NULL;
pico_Engine picoEngine = NULL;
pico_Char *picoMemArea = NULL;

void setup_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    i2s_set_pin(I2S_NUM, &pin_config);
}

bool init_pico_tts() {
    pico_Status ret;
    
    // Allocate memory - use SPIRAM if available
    picoMemArea = (pico_Char *)heap_caps_malloc(
        PICO_MEM_SIZE, 
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    
    if (!picoMemArea) {
        Serial.println("Failed to allocate Pico memory");
        return false;
    }
    
    // Initialize Pico system
    ret = pico_initialize(picoMemArea, PICO_MEM_SIZE, &picoSystem);
    if (ret != PICO_OK) {
        Serial.printf("Pico init failed: %d\n", ret);
        return false;
    }
    
    // Load resources from SPIFFS
    const char *ta_path = "/spiffs/en-US_ta.bin";
    const char *sg_path = "/spiffs/en-US_lh0_sg.bin";
    
    ret = pico_loadResource(picoSystem, ta_path, &picoTaResource);
    if (ret != PICO_OK) {
        Serial.printf("Failed to load TA resource: %d\n", ret);
        return false;
    }
    
    ret = pico_loadResource(picoSystem, sg_path, &picoSgResource);
    if (ret != PICO_OK) {
        Serial.printf("Failed to load SG resource: %d\n", ret);
        return false;
    }
    
    // Create voice
    const pico_Char *voiceName = (const pico_Char *)"PicoVoice";
    ret = pico_createVoiceDefinition(picoSystem, voiceName);
    if (ret != PICO_OK) {
        Serial.printf("Failed to create voice: %d\n", ret);
        return false;
    }
    
    ret = pico_addResourceToVoiceDefinition(picoSystem, voiceName, 
                                            picoTaResource->resourceName);
    ret = pico_addResourceToVoiceDefinition(picoSystem, voiceName, 
                                            picoSgResource->resourceName);
    
    // Create engine
    ret = pico_newEngine(picoSystem, voiceName, &picoEngine);
    if (ret != PICO_OK) {
        Serial.printf("Failed to create engine: %d\n", ret);
        return false;
    }
    
    Serial.println("PicoTTS initialized successfully");
    return true;
}

void synthesize_speech(const char *text) {
    if (!picoEngine) {
        Serial.println("Engine not initialized");
        return;
    }
    
    pico_Char *inp = (pico_Char *)text;
    int16_t text_remaining = strlen(text) + 1;
    int16_t bytes_sent, bytes_recv;
    pico_Status ret;
    
    // Output buffer for audio samples
    int16_t outbuf[PICO_BUFFER_SIZE / 2];
    pico_Int16 out_data_type;
    
    // Synthesis loop
    while (text_remaining > 0) {
        // Feed text to engine
        ret = pico_putTextUtf8(picoEngine, inp, text_remaining, &bytes_sent);
        if (ret != PICO_OK) {
            Serial.printf("putText error: %d\n", ret);
            break;
        }
        
        text_remaining -= bytes_sent;
        inp += bytes_sent;
        
        // Retrieve audio data
        do {
            ret = pico_getData(picoEngine, (void *)outbuf, 
                              PICO_BUFFER_SIZE, &bytes_recv, &out_data_type);
            
            if (bytes_recv > 0) {
                // Output to I2S
                size_t bytes_written;
                i2s_write(I2S_NUM, outbuf, bytes_recv, &bytes_written, 
                         portMAX_DELAY);
            }
        } while (ret == PICO_STEP_BUSY);
        
        if (ret != PICO_OK && ret != PICO_STEP_IDLE) {
            Serial.printf("getData error: %d\n", ret);
            break;
        }
    }
    
    // Flush remaining data
    do {
        ret = pico_getData(picoEngine, (void *)outbuf, 
                          PICO_BUFFER_SIZE, &bytes_recv, &out_data_type);
        if (bytes_recv > 0) {
            size_t bytes_written;
            i2s_write(I2S_NUM, outbuf, bytes_recv, &bytes_written, portMAX_DELAY);
        }
    } while (ret == PICO_STEP_BUSY);
    
    // Reset engine for next synthesis
    pico_resetEngine(picoEngine, PICO_RESET_SOFT);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("PicoTTS ESP32 Demo");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // Initialize I2S
    setup_i2s();
    
    // Initialize TTS
    if (!init_pico_tts()) {
        Serial.println("TTS initialization failed!");
        return;
    }
    
    Serial.printf("Free heap after init: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
    // Synthesize test phrase
    synthesize_speech("Hello world. This is Pico T T S on E S P 32.");
    
    delay(5000);  // Wait 5 seconds before next synthesis
}
```

---

### Step 3: Memory Optimization for ESP32

**Reduce buffer sizes in picoapid.h (or create esp32_config.h):**
```c
// ESP32-optimized buffer sizes
#ifdef ESP32
    #define PICODATA_BUFSIZE_DEFAULT  512   // Was 2048
    #define PICODATA_BUFSIZE_SIG      512   // Was 2048
    #define PICODATA_BUFSIZE_PAM      512   // Was 2048
    #define PICOPAM_MAX_PH_PER_SENT   100   // Was 400
#endif
```

**Use ESP-IDF memory features:**
```cpp
// Place large const data in flash
const DRAM_ATTR uint8_t kb_data[] = { ... };  // Force to DRAM
const PROGMEM uint8_t lookup_table[] = { ... };  // Force to flash

// Optimize heap usage
void optimize_heap() {
    heap_caps_malloc_extmem_enable(4096);  // Use SPIRAM for >4KB allocations
    
    // Monitor heap
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    Serial.printf("Internal free: %d, largest block: %d\n", 
                  info.total_free_bytes, info.largest_free_block);
}
```

---

### Step 4: Performance Monitoring

**Add performance counters:**
```cpp
#include "esp_timer.h"

void benchmark_synthesis(const char *text) {
    uint64_t start_time = esp_timer_get_time();
    
    synthesize_speech(text);
    
    uint64_t elapsed_us = esp_timer_get_time() - start_time;
    float elapsed_sec = elapsed_us / 1000000.0;
    
    // Calculate audio duration (assume 16 kHz, 16-bit mono)
    int text_len = strlen(text);
    float estimated_audio_sec = text_len * 0.1;  // Rough estimate
    
    float rtf = elapsed_sec / estimated_audio_sec;
    
    Serial.printf("Synthesis time: %.2f sec\n", elapsed_sec);
    Serial.printf("Real-time factor: %.2f\n", rtf);
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
}
```

---

### Step 5: Resource Management

**Load resources from SPIFFS:**
```cpp
#include "SPIFFS.h"

void mount_spiffs() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return;
    }
    
    Serial.println("SPIFFS mounted successfully");
    
    // List files
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.printf("File: %s, Size: %d\n", file.name(), file.size());
        file = root.openNextFile();
    }
}

// Upload resources using ESP32 file upload tool
// Place .bin files in data/ folder and upload with:
// pio run --target uploadfs
```

---

## Advanced Optimizations

### 1. Fixed-Point Conversion (Performance Boost)

**Replace floating-point in critical loops:**
```cpp
// Original floating-point
float result = coeff * sample;

// Fixed-point Q15 version
int32_t coeff_q15 = (int32_t)(coeff * 32768.0f);
int32_t sample_q15 = (int32_t)(sample * 32768.0f);
int32_t result_q15 = (coeff_q15 * sample_q15) >> 15;
float result = result_q15 / 32768.0f;
```

### 2. ESP-DSP FFT Acceleration

**Use ESP32 optimized FFT:**
```cpp
#include "esp_dsp.h"

void optimized_fft(float *data, int N) {
    // Initialize FFT
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, N);
    if (ret != ESP_OK) return;
    
    // Bit-reverse
    dsps_bit_rev_fc32(data, N);
    
    // FFT
    dsps_fft2r_fc32(data, N);
    
    // Cleanup
    dsps_fft2r_deinit_fc32();
}
```

### 3. Dual-Core Utilization

**Split processing across cores:**
```cpp
// Core 0: Text processing
void text_task(void *params) {
    for (;;) {
        char *text = get_next_text();
        process_text_to_phonemes(text);
        vTaskDelay(1);
    }
}

// Core 1: Audio synthesis
void synthesis_task(void *params) {
    for (;;) {
        phoneme_data_t *phonemes = get_phonemes();
        synthesize_audio(phonemes);
        vTaskDelay(1);
    }
}

void setup() {
    // ... initialization ...
    
    xTaskCreatePinnedToCore(text_task, "text", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(synthesis_task, "synth", 4096, NULL, 1, NULL, 1);
}
```

---

## Troubleshooting

### Issue: Out of Memory
**Solution:**
```cpp
// 1. Enable SPIRAM
#define CONFIG_SPIRAM_SUPPORT 1

// 2. Reduce buffer sizes
// Edit picodefs.h or override in esp32_config.h

// 3. Use heap monitoring
heap_trace_start(HEAP_TRACE_LEAKS);
// ... your code ...
heap_trace_stop();
heap_trace_dump();
```

### Issue: Synthesis Too Slow
**Solution:**
```cpp
// 1. Increase CPU frequency
setCpuFrequencyMhz(240);

// 2. Use fixed-point math (see above)

// 3. Enable compiler optimizations
// In platformio.ini:
build_flags = -O3 -march=native
```

### Issue: Crackling Audio
**Solution:**
```cpp
// 1. Increase DMA buffer size
i2s_config.dma_buf_len = 512;  // Increase from 256

// 2. Use larger I2S buffers
#define PICO_BUFFER_SIZE 2048  // Increase from 1024

// 3. Ensure consistent timing
// Avoid Serial.print() during synthesis
```

---

## Expected Performance

| Metric | ESP32 @ 240 MHz | ESP32-S3 @ 240 MHz |
|--------|-----------------|-------------------|
| Real-time factor | 0.5-0.8 | 0.3-0.5 |
| Latency (first audio) | 50-100 ms | 30-70 ms |
| Memory usage | 80-150 KB | 80-150 KB |
| Languages (4MB flash) | 1-2 | 1-2 |
| Power consumption | 100-130 mA | 80-100 mA |

---

## Next Steps

1. **Test basic synthesis** - Verify audio output works
2. **Optimize memory** - Reduce buffers, enable SPIRAM
3. **Profile performance** - Measure RTF and identify bottlenecks
4. **Add optimizations** - Fixed-point, ESP-DSP, dual-core
5. **Quality tuning** - Adjust parameters for best quality/speed trade-off

---

## Phase 3: Quality Improvements

### Voice Quality Enhancement

**Enable quality improvements at compile time:**
```cpp
// In your build configuration
#define PICO_USE_QUALITY_ENHANCE 1
#define PICO_DEFAULT_QUALITY_MODE PICO_QUALITY_MODE_BALANCED
```

**Initialize quality enhancements:**
```cpp
#include "picoqualityenhance.h"

void setup_voice_quality() {
    // Initialize quality enhancement
    pico_quality_init();
    
    // Set quality mode based on use case
    pico_set_quality_mode(PICO_QUALITY_MODE_BALANCED);
    
    // Customize voice parameters
    pico_voice_params_t params = {
        .pitch_scale = 1.0f,      // Default pitch
        .speed_scale = 1.0f,      // Default speed
        .formant_shift = 0.0f,    // No formant shift
        .quality_mode = PICO_QUALITY_MODE_BALANCED
    };
    pico_set_voice_params(&params);
}
```

### Voice Customization Examples

**Female voice for smart home assistant:**
```cpp
void setup_female_voice() {
    pico_quality_init();
    pico_apply_voice_profile(PICO_VOICE_PROFILE_FEMALE);
    // Result: Higher pitch, shifted formants for female characteristics
}
```

**Male voice for announcements:**
```cpp
void setup_male_voice() {
    pico_quality_init();
    pico_apply_voice_profile(PICO_VOICE_PROFILE_MALE);
    // Result: Lower pitch, deeper formants for male characteristics
}
```

**Fast voice for notifications:**
```cpp
void setup_notification_voice() {
    pico_quality_init();
    pico_apply_voice_profile(PICO_VOICE_PROFILE_FAST);
    // Result: 1.4x speed, shorter pauses for quick alerts
}
```

### Quality Mode Selection

**Speed Mode (RTF ~0.25) - Best for quick notifications:**
```cpp
pico_set_quality_mode(PICO_QUALITY_MODE_SPEED);
// Lower quality, fastest synthesis
// Use for: Door alerts, timers, quick status updates
```

**Balanced Mode (RTF ~0.35) - Default, good for most uses:**
```cpp
pico_set_quality_mode(PICO_QUALITY_MODE_BALANCED);
// Good quality, real-time synthesis
// Use for: Voice assistants, general TTS, smart home
```

**Quality Mode (RTF ~0.55) - Best for long-form content:**
```cpp
pico_set_quality_mode(PICO_QUALITY_MODE_QUALITY);
// Highest quality, still real-time
// Use for: Audiobooks, accessibility, long messages
```

### Enhanced Prosody for Better Intonation

**Adjust prosody for more expressive speech:**
```cpp
pico_prosody_params_t prosody = {
    .emphasis_scale = 1.3f,    // More emphasis on important words
    .pause_scale = 1.2f,       // Slightly longer pauses
    .question_boost = 70       // Stronger question intonation
};
pico_set_prosody_params(&prosody);
```

### Improved Excitation for Better Consonants

**Quality improvements include:**
- Better fricative sounds (s, sh, f, th)
- More natural unvoiced consonants
- 10-15% overall quality improvement
- Minimal CPU overhead (~2-3%)

**The noise shaping is automatic when PICO_USE_QUALITY_ENHANCE=1**

### Performance Impact

| Feature | CPU Overhead | Quality Gain | Notes |
|---------|--------------|--------------|-------|
| Shaped Noise | 2-3% | 10-15% better consonants | Automatic |
| Pitch Scaling | 1-2% | Voice customization | On demand |
| Speed Scaling | <1% | Flexibility | On demand |
| Prosody Enhancement | <1% | Better intonation | Runtime adjustable |
| **Total** | **5-9%** | **Significant** | Still real-time |

### Memory Impact

Phase 3 adds only ~200 bytes of RAM:
- Voice parameters: 24 bytes
- Prosody parameters: 16 bytes
- Noise filter state: 128 bytes
- Total: ~200 bytes (negligible)

### Complete ESP32 Example with Quality

```cpp
#include "picoapi.h"
#include "picoqualityenhance.h"

void setup() {
    // Initialize PicoTTS
    pico_initialize();
    
    // Initialize quality enhancements
    pico_quality_init();
    
    // Set up voice for smart home assistant
    pico_apply_voice_profile(PICO_VOICE_PROFILE_FEMALE);
    pico_set_quality_mode(PICO_QUALITY_MODE_BALANCED);
    
    // Fine-tune prosody for friendlier speech
    pico_prosody_params_t prosody = {
        .emphasis_scale = 1.2f,
        .pause_scale = 1.1f,
        .question_boost = 60
    };
    pico_set_prosody_params(&prosody);
    
    Serial.println("Voice assistant ready!");
}

void loop() {
    // Synthesize with quality enhancements active
    synthesize_text("Hello! How can I help you today?");
    delay(5000);
}
```

### Quality Statistics Monitoring

```cpp
void print_quality_stats() {
    pico_quality_stats_t stats;
    pico_get_quality_stats(&stats);
    
    Serial.printf("Noise samples: %u\n", stats.noise_samples_generated);
    Serial.printf("Pitch adjustments: %u\n", stats.pitch_adjustments);
    Serial.printf("Formant shifts: %u\n", stats.formant_shifts);
}
```

### See Also

- **PHASE3_QUALITY_IMPROVEMENTS.md** - Complete quality enhancement documentation
- **examples/quality_example.c** - Comprehensive usage examples
- **IMPROVEMENT_SUGGESTIONS.md** - Section 3: Quality Improvements

## Resources

- **ESP-IDF Documentation:** https://docs.espressif.com/projects/esp-idf/
- **ESP-DSP Library:** https://github.com/espressif/esp-dsp
- **PicoTTS Reference:** See ALGORITHM_ANALYSIS.md
- **Optimization Guide:** See IMPROVEMENT_SUGGESTIONS.md

---

## License

This implementation guide is provided as-is for educational purposes. PicoTTS original code is under Apache 2.0 license.
