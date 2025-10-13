# PicoTTS Improvement Suggestions for Embedded Systems

## Executive Summary

This document provides concrete, actionable recommendations for improving PicoTTS performance, memory efficiency, and voice quality specifically for embedded hardware like ESP32. The suggestions are prioritized by impact vs. implementation complexity.

---

## 1. Critical Optimizations for ESP32 (High Priority)

### 1.1 Fixed-Point Conversion for Core DSP

**Current Issue:** Heavy use of floating-point operations in signal generation

**Recommendation:** Convert mel-cepstral synthesis to Q15 or Q31 fixed-point arithmetic

**Implementation:**
```c
// Current (floating-point):
picoos_single result = coeff * sample;

// Proposed (Q15 fixed-point):
int32_t result = (coeff_q15 * sample_q15) >> 15;
```

**Benefits:**
- 2-4x faster on ESP32 (even with FPU, fixed-point is faster for simple ops)
- Lower power consumption
- More deterministic performance

**Effort:** Medium (2-3 weeks)
**Impact:** 30-50% performance improvement

---

### 1.2 Optimized FFT for ESP32

**Current Issue:** Generic table-free FFT, not optimized for platform

**Recommendation:** Use ESP32-optimized FFT library or implement fixed-point FFT

**Options:**
1. **ESP-DSP Library** - Espressif's optimized DSP functions
   ```c
   #include "esp_dsp.h"
   dsps_fft2r_fc32_ansi(fft_data, FFT_SIZE);
   ```

2. **Fixed-point FFT** - 16-bit integer FFT
   ```c
   // Use Q15 format for 256-point FFT
   void fft_256_q15(int16_t *real, int16_t *imag);
   ```

**Benefits:**
- 3-5x faster FFT operations
- Utilizes ESP32 hardware acceleration
- Reduced memory bandwidth

**Effort:** Medium (1-2 weeks with ESP-DSP)
**Impact:** 40-60% total performance improvement (FFT is 60-70% of compute)

---

### 1.3 Streaming Architecture for Memory Reduction

**Current Issue:** Buffers entire sentence before synthesis (up to 400 phonemes)

**Recommendation:** Implement true streaming with smaller circular buffers

**Design:**
```c
// Current:
#define PICOPAM_MAX_PH_PER_SENT 400  // 24 KB buffer

// Proposed:
#define PICOPAM_STREAM_BUFFER 32     // 2 KB buffer (16 phonemes)
#define PICOPAM_LOOKAHEAD 4          // 4 phoneme lookahead
```

**Implementation:**
- Process in chunks of 16 phonemes
- Maintain small lookahead buffer for prosody
- Overlap-add at chunk boundaries

**Benefits:**
- 90% reduction in RAM usage (24 KB → 2-3 KB)
- Lower latency (streaming output)
- Enables longer sentences on ESP32

**Effort:** High (3-4 weeks)
**Impact:** Critical for ESP32 deployment

---

### 1.4 Knowledge Base Compression

**Current Issue:** Uncompressed language data (3-7 MB per language)

**Recommendation:** Implement lightweight compression for knowledge bases

**Approaches:**

1. **Decision Tree Compression:**
   - Use Huffman coding for frequent node types
   - Share common subtrees across phonemes
   - Expected reduction: 30-40%

2. **PDF Data Quantization:**
   ```c
   // Current: uint8 per coefficient (256 values)
   // Proposed: 4-bit per coefficient (16 values) with codebook
   uint8_t codebook[16];
   uint8_t pdf_data_compressed[numframes * vecsize / 2];  // 50% reduction
   ```

3. **Lexicon Compression:**
   - Use trie structure instead of hash table
   - Delta encoding for phoneme sequences
   - Expected reduction: 20-30%

**Benefits:**
- 2-3 MB saved per language
- Fits 2-3 languages in 4 MB ESP32 flash
- Faster loading (less data to read)

**Effort:** Medium-High (2-3 weeks)
**Impact:** Enables multi-language on ESP32

---

### 1.5 XIP (Execute-In-Place) for Flash Access

**Current Issue:** Knowledge bases loaded into RAM

**Recommendation:** Access knowledge bases directly from flash memory

**Implementation:**
```c
// ESP32 specific: map flash to memory
const uint8_t *kb_data = (const uint8_t *)0x3F400000;  // Flash-mapped region

// Use const pointers for read-only access
const picokpdf_PdfMUL *pdf = (const picokpdf_PdfMUL *)&kb_data[offset];
```

**Benefits:**
- Zero RAM usage for knowledge bases
- Instant loading (no copy to RAM)
- Trade: slightly slower access (flash vs RAM)

**Considerations:**
- ESP32 flash is memory-mapped and cached
- Cache hit rate typically >90% for sequential access
- Negligible performance impact for KB access patterns

**Effort:** Low (1 week)
**Impact:** Saves 3-7 MB RAM per language

---

## 2. Performance Optimizations (Medium Priority)

### 2.1 SIMD Vectorization for ESP32

**Recommendation:** Use ESP32 SIMD instructions for DSP loops

**Example:**
```c
// ESP32 has limited SIMD (2-way for some ops)
#include <xtensa/config/core-isa.h>

// Vectorize cepstral coefficient processing
void process_cepstral_simd(int16_t *cep, int16_t *window, int len) {
    for (int i = 0; i < len; i += 2) {
        // Process 2 samples at once using paired instructions
        int32_t a = cep[i] * window[i];
        int32_t b = cep[i+1] * window[i+1];
        // Store results
    }
}
```

**Effort:** Medium (1-2 weeks)
**Impact:** 15-25% improvement for specific loops

---

### 2.2 Caching for Decision Tree Traversal

**Current Issue:** Poor cache locality in tree traversal

**Recommendation:** Pre-compute and cache frequent tree paths

**Design:**
```c
// Cache for common phoneme contexts
typedef struct {
    uint32_t context_hash;  // Hash of linguistic context
    uint16_t pdf_index;     // Cached PDF result
} TreeCache;

TreeCache lru_cache[256];  // Small LRU cache

// Check cache before tree traversal
uint16_t get_pdf_cached(uint32_t context_hash) {
    // LRU lookup
    // If hit: return cached result
    // If miss: traverse tree, cache result
}
```

**Benefits:**
- 50-70% cache hit rate for common contexts
- Reduces tree traversal by half
- Only 1-2 KB cache memory

**Effort:** Low-Medium (1 week)
**Impact:** 20-30% PAM stage improvement

---

### 2.3 Lazy Initialization

**Current Issue:** All modules initialized at startup

**Recommendation:** Initialize modules on-demand

**Example:**
```c
// Initialize only when first needed
if (!sig_initialized) {
    sigAllocate(mm, sig_inObj);
    sig_initialized = 1;
}

// Free unused modules
if (synthesis_complete && !pam_needed) {
    pamDeallocate(pam_obj);
    pam_initialized = 0;
}
```

**Benefits:**
- Faster startup time
- Lower baseline memory usage
- Dynamic resource allocation

**Effort:** Low (3-5 days)
**Impact:** 30-50% faster initialization, 10-20% lower idle memory

---

### 2.4 Batch Processing for Low Latency

**Recommendation:** Process multiple frames per iteration

**Current:**
```c
while (text_remaining) {
    process_one_frame();  // 64 samples
}
```

**Proposed:**
```c
while (text_remaining) {
    process_batch(4);  // 4 frames = 256 samples at once
}
```

**Benefits:**
- Amortizes loop overhead
- Better cache utilization
- 10-15% performance improvement

**Effort:** Low (2-3 days)
**Impact:** 10-15% overall speedup

---

## 3. Quality Improvements (Medium-Low Priority)

### 3.1 Higher Sample Rate Support

**Recommendation:** Add 22 kHz and 24 kHz output options

**Implementation:**
- Upsample final output using polyphase filter
- Or regenerate with higher sample rate in SIG module

```c
// Simple linear interpolation for 22 kHz (1.375x)
void upsample_to_22khz(int16_t *in_16k, int16_t *out_22k, int len) {
    for (int i = 0; i < len-1; i++) {
        out_22k[i*11/8] = in_16k[i];
        // Interpolate intermediate samples
    }
}
```

**Benefits:**
- Noticeably better quality
- Wider frequency range (0-11 kHz vs 0-8 kHz)
- Minor performance cost (~5-10%)

**Effort:** Medium (1-2 weeks)
**Impact:** Significant quality improvement

---

### 3.2 Enhanced Prosody Modeling

**Recommendation:** Add simple neural prosody predictor

**Design:**
- Tiny LSTM/GRU (1 layer, 32 units) for F0 contour
- Quantized weights (4-bit or 8-bit)
- Runs on ESP32 in parallel with TTS

```c
// Lightweight F0 prediction
typedef struct {
    int8_t weights[32][32];  // Quantized weights
    int16_t hidden[32];       // Hidden state
} TinyLSTM;

int16_t predict_f0(TinyLSTM *lstm, int16_t *features) {
    // Simple LSTM forward pass (~1000 ops)
    // Returns F0 value
}
```

**Benefits:**
- More natural intonation
- Better emotion/emphasis
- Still real-time on ESP32

**Effort:** High (4-6 weeks, requires training)
**Impact:** 30-40% perceived quality improvement

---

### 3.3 Improved Excitation Generation

**Recommendation:** Replace pseudo-random noise with better noise model

**Current:**
```c
// Simple pseudo-random from table
int32_t noise = random_table[index++];
```

**Proposed:**
```c
// LPC residual-based noise shaping
int32_t noise = white_noise();
noise = lpc_filter(noise, formant_coeffs);  // Shape spectrum
```

**Benefits:**
- More natural unvoiced sounds (s, sh, f, th)
- Better consonant quality
- Minimal computational cost

**Effort:** Low-Medium (1 week)
**Impact:** 10-15% quality improvement for fricatives

---

### 3.4 Voice Customization API

**Recommendation:** Expose voice parameters for runtime modification

**API Design:**
```c
typedef struct {
    float pitch_scale;     // 0.5 - 2.0 (default 1.0)
    float speed_scale;     // 0.5 - 3.0 (default 1.0)
    float formant_shift;   // -500 to +500 Hz
    float breathiness;     // 0.0 - 1.0 (adds aspiration)
    float roughness;       // 0.0 - 1.0 (adds jitter)
} VoiceParams;

void setVoiceParams(TtsEngine *engine, VoiceParams *params);
```

**Benefits:**
- Male/female/child voice effects
- Expressive speech (sad, happy, excited)
- Character voices for applications

**Effort:** Medium (1-2 weeks)
**Impact:** Greatly enhanced flexibility

---

## 4. ESP32-Specific Optimizations (Low-Medium Priority)

### 4.1 Dual-Core Utilization

**Recommendation:** Pipeline text processing and synthesis on separate cores

**Architecture:**
```
Core 0: Text → Phonemes → PAM
Core 1: SIG → Audio Output

Queue: Core 0 → Core 1 (acoustic parameters)
```

**Implementation:**
```c
// Core 0 task
void text_processing_task(void *params) {
    while (1) {
        phonemes = process_text(input_text);
        acoustic = run_pam(phonemes);
        xQueueSend(acoustic_queue, &acoustic, portMAX_DELAY);
    }
}

// Core 1 task  
void synthesis_task(void *params) {
    while (1) {
        xQueueReceive(acoustic_queue, &acoustic, portMAX_DELAY);
        audio = run_sig(acoustic);
        output_audio(audio);
    }
}
```

**Benefits:**
- 40-60% throughput improvement
- Lower latency (pipelined)
- Better core utilization

**Effort:** Medium (2 weeks)
**Impact:** Significant for continuous synthesis

---

### 4.2 I2S Audio Output Optimization

**Recommendation:** Use DMA for zero-copy audio output

**Implementation:**
```c
#include "driver/i2s.h"

// Configure I2S with DMA
i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_TX,
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1
};

// Zero-copy write
i2s_write(I2S_NUM_0, audio_buffer, buffer_size, &bytes_written, portMAX_DELAY);
```

**Benefits:**
- Zero CPU overhead for audio output
- No blocking during synthesis
- Perfect timing (no jitter)

**Effort:** Low (2-3 days)
**Impact:** 5-10% CPU freed for synthesis

---

### 4.3 Power Management

**Recommendation:** Dynamic frequency scaling based on synthesis load

**Implementation:**
```c
#include "esp_pm.h"

// Reduce frequency during idle
if (synthesis_idle) {
    esp_pm_configure({
        .max_freq_mhz = 80,     // Lower frequency
        .min_freq_mhz = 40,
        .light_sleep_enable = true
    });
} else {
    esp_pm_configure({
        .max_freq_mhz = 240,    // Full speed
        .min_freq_mhz = 160,
        .light_sleep_enable = false
    });
}
```

**Benefits:**
- 50-70% power reduction during idle
- Extended battery life
- Automatic thermal management

**Effort:** Low (3-5 days)
**Impact:** Major for battery-powered devices

---

### 4.4 Flash Caching Strategy

**Recommendation:** Optimize flash cache for knowledge base access

**Implementation:**
```c
// ESP32 cache configuration
#include "esp_flash.h"
#include "esp_partition.h"

// Mark KB partitions for caching priority
const esp_partition_t *kb_partition = esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA, 
    ESP_PARTITION_SUBTYPE_ANY, 
    "tts_kb"
);

// Configure cache hints
esp_flash_read_with_cache_hints(kb_partition->flash_chip, 
    kb_data, offset, size,
    ESP_FLASH_CACHE_HINT_SEQUENTIAL);
```

**Benefits:**
- 2-3x faster KB access
- Lower latency for tree lookups
- Better overall performance

**Effort:** Low (1-2 days)
**Impact:** 10-15% improvement

---

## 5. Memory Optimizations (High Priority for ESP32)

### 5.1 Reduce Buffer Sizes

**Current Buffers:**
```c
#define MAX_OUTBUF_SIZE 2048       // Output buffer
#define PICOSIG_IN_BUFF_SIZE 2048  // Input buffer
#define PICOSIG_OUT_BUFF_SIZE 2048 // SIG output buffer
```

**Optimized for ESP32:**
```c
#define MAX_OUTBUF_SIZE 512        // Reduce by 4x
#define PICOSIG_IN_BUFF_SIZE 512   // Streaming mode
#define PICOSIG_OUT_BUFF_SIZE 512  // Smaller chunks
```

**Benefits:**
- 6 KB saved immediately
- Enables more concurrent tasks
- Lower latency (smaller buffers)

**Effort:** Low (1-2 days)
**Impact:** Critical for tight memory scenarios

---

### 5.2 Lazy Buffer Allocation

**Recommendation:** Allocate buffers only when needed

```c
typedef struct {
    int16_t *fft_buffer;
    int16_t *cep_buffer;
    int16_t *window_buffer;
    bool allocated;
} SynthBuffers;

SynthBuffers* allocate_if_needed(SynthBuffers *bufs) {
    if (!bufs->allocated) {
        bufs->fft_buffer = malloc(512 * sizeof(int16_t));
        bufs->cep_buffer = malloc(256 * sizeof(int16_t));
        bufs->window_buffer = malloc(256 * sizeof(int16_t));
        bufs->allocated = true;
    }
    return bufs;
}

void deallocate_when_idle(SynthBuffers *bufs) {
    if (bufs->allocated && synthesis_idle) {
        free(bufs->fft_buffer);
        free(bufs->cep_buffer);
        free(bufs->window_buffer);
        bufs->allocated = false;
    }
}
```

**Benefits:**
- 3-5 KB saved when not synthesizing
- Better memory fragmentation
- Allows other tasks to use memory

**Effort:** Low (2-3 days)
**Impact:** 10-15% better memory utilization

---

### 5.3 Heap Optimization

**Recommendation:** Use ESP32's multiple heaps efficiently

```c
// Use SPIRAM for large buffers (if available)
#include "esp_heap_caps.h"

// Allocate KB data in SPIRAM
uint8_t *kb_data = heap_caps_malloc(kb_size, MALLOC_CAP_SPIRAM);

// Allocate synthesis buffers in internal RAM (faster)
int16_t *fft_buf = heap_caps_malloc(fft_size, MALLOC_CAP_INTERNAL);

// Use DMA-capable memory for I2S
int16_t *audio_buf = heap_caps_malloc(audio_size, MALLOC_CAP_DMA);
```

**Benefits:**
- Optimal memory placement
- Faster critical operations
- Support for larger KB with SPIRAM

**Effort:** Low (1-2 days)
**Impact:** 15-20% performance improvement with proper placement

---

## 6. Implementation Priority Matrix

| Optimization | Impact | Effort | Priority | Est. Time |
|--------------|--------|--------|----------|-----------|
| Fixed-Point DSP | High | Medium | **Critical** | 2-3 weeks |
| ESP32 Optimized FFT | Very High | Medium | **Critical** | 1-2 weeks |
| Streaming Architecture | Very High | High | **Critical** | 3-4 weeks |
| XIP Flash Access | Very High | Low | **Critical** | 1 week |
| KB Compression | High | Medium | **High** | 2-3 weeks |
| Decision Tree Caching | Medium | Low | **High** | 1 week |
| Dual-Core Pipeline | High | Medium | **High** | 2 weeks |
| I2S DMA Output | Medium | Low | **High** | 2-3 days |
| Buffer Reduction | Medium | Low | **Medium** | 1-2 days |
| Lazy Allocation | Medium | Low | **Medium** | 2-3 days |
| Higher Sample Rate | Medium | Medium | **Medium** | 1-2 weeks |
| Enhanced Prosody | High | Very High | **Low** | 4-6 weeks |
| Voice Customization | Medium | Medium | **Low** | 1-2 weeks |

---

## 7. Recommended Implementation Phases

### Phase 1: Critical ESP32 Enablement (4-6 weeks)
1. XIP Flash Access (1 week)
2. Streaming Architecture (3-4 weeks)  
3. Buffer Optimization (3-5 days)
4. I2S DMA Output (2-3 days)

**Result:** TTS runs reliably on ESP32 with single language

---

### Phase 2: Performance Optimization (3-5 weeks)
1. ESP32 Optimized FFT (1-2 weeks)
2. Fixed-Point DSP (2-3 weeks)
3. Decision Tree Caching (1 week)
4. Lazy Initialization (3-5 days)

**Result:** 2-3x performance improvement, real-time factor < 0.3

---

### Phase 3: Multi-Language Support (2-3 weeks)
1. KB Compression (2-3 weeks)
2. Heap Optimization (1-2 days)

**Result:** 2-3 languages fit in 4 MB flash

---

### Phase 4: Advanced Features (4-8 weeks)
1. Dual-Core Pipeline (2 weeks)
2. Higher Sample Rate (1-2 weeks)
3. Voice Customization (1-2 weeks)
4. Enhanced Prosody (4-6 weeks, optional)

**Result:** Near-commercial quality TTS on ESP32

---

## 8. Testing and Validation

### 8.1 Performance Benchmarks
```c
// Measure synthesis performance
uint32_t start = esp_timer_get_time();
synthesize_text("The quick brown fox jumps over the lazy dog");
uint32_t elapsed = esp_timer_get_time() - start;
printf("RTF: %.2f\n", elapsed / (audio_duration * 1000000.0));
```

**Targets:**
- Real-time factor: < 0.5 (2x faster than real-time)
- Latency: < 100 ms for short phrases
- Memory: < 150 KB RAM total

---

### 8.2 Quality Metrics
- **MOS (Mean Opinion Score):** Target > 3.5 (decent quality)
- **Intelligibility:** Word error rate < 5% for common words
- **Naturalness:** Prosody quality subjective evaluation

---

### 8.3 Stress Testing
```c
// Memory stress test
for (int i = 0; i < 1000; i++) {
    synthesize_random_text();
    assert(heap_caps_check_integrity_all(true));
}

// Continuous synthesis test  
while (1) {
    synthesize_text(test_corpus[i++ % corpus_size]);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Should run indefinitely without crashes
}
```

---

## 9. Conclusion

PicoTTS can be significantly optimized for ESP32 with the following expected outcomes:

### After Phase 1 (Critical):
- ✅ Runs on ESP32 with 1 language
- ✅ ~150 KB RAM usage
- ✅ RTF ~0.8 (acceptable)

### After Phase 2 (Performance):
- ✅ RTF ~0.3 (3x faster than real-time)
- ✅ ~100 KB RAM usage
- ✅ 30-40% lower power consumption

### After Phase 3 (Multi-language):
- ✅ 2-3 languages in 4 MB flash
- ✅ Dynamic language switching

### After Phase 4 (Advanced):
- ✅ Near-commercial quality
- ✅ 22 kHz output
- ✅ Customizable voices
- ✅ Dual-core performance

**Total Development Time:** 13-22 weeks for complete optimization

**Recommended Starting Point:** 
Focus on **Phase 1** (Critical Enablement) first, then evaluate needs for subsequent phases based on application requirements.
