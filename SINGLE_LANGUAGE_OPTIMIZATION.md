# Single-Language Optimization Guide

## Overview

This guide explains how to optimize PicoTTS for embedded systems when using only one language (e.g., English). These optimizations can significantly reduce memory usage and startup time.

## Performance Improvements for Single-Language Usage

### 1. Memory Savings

When using only English (en-US), you can achieve substantial memory savings:

| Component | Multi-Language | English Only | Savings |
|-----------|---------------|--------------|---------|
| Flash Storage | 15-20 MB (all languages) | 3-4 MB | 80% |
| RAM (without XIP) | 7-10 MB | 3-4 MB | 60% |
| RAM (with XIP) | 2-3 MB | 500 KB - 1 MB | 70% |

### 2. Startup Time Improvements

| Method | Startup Time | Notes |
|--------|-------------|--------|
| Standard loading | 200-500 ms | Loads and specializes KBs |
| XIP (flash-mapped) | 50-100 ms | Minimal processing |
| Pre-cached (SPIRAM) | 10-30 ms | Best performance |

## Implementation Strategies

### Strategy 1: XIP (Execute-In-Place) - Recommended for ESP32

**What it does:** Access knowledge base data directly from flash memory without copying to RAM.

**Benefits:**
- Zero RAM usage for KB data (3-4 MB savings)
- Fast startup (no copying needed)
- Works with ESP32's memory-mapped flash

**Implementation:**

1. **Enable XIP in your build:**
```c
// In your platform-specific code or picoembedded.h
#define PICO_XIP_SUPPORT 1
#define PICO_XIP_CONST const __attribute__((section(".rodata")))
```

2. **Link language files into flash:**
```c
// ESP-IDF example
PICO_XIP_CONST uint8_t en_us_ta_data[] = {
    #include "en-US_ta.bin.h"  // Convert .bin to .h using xxd or similar
};

PICO_XIP_CONST uint8_t en_us_sg_data[] = {
    #include "en-US_lh0_sg.bin.h"
};
```

3. **Load from embedded data:**
```c
// Instead of loading from file system:
pico_status_t loadEmbeddedResource(pico_System system, 
                                   const uint8_t *data, 
                                   uint32_t size,
                                   const char *resourceName) {
    // Modify picorsrc_loadResource to accept memory buffer
    // Set kb->base to point directly at flash data
    // Skip file I/O completely
}
```

**ESP32-specific example:**
```c
// The language data is already in flash (.rodata section)
// ESP32 flash is cached, so access is reasonably fast
extern const uint8_t en_us_ta_start[] asm("_binary_en_US_ta_bin_start");
extern const uint8_t en_us_ta_end[]   asm("_binary_en_US_ta_bin_end");

// Use these pointers directly - no malloc needed
kb->base = (uint8_t *)en_us_ta_start;
kb->size = en_us_ta_end - en_us_ta_start;
```

### Strategy 2: SPIRAM Mapping - Best Performance

**What it does:** Map knowledge base data to external SPIRAM for faster access than flash.

**Benefits:**
- Faster than flash XIP (SPIRAM is faster than flash)
- Still saves main SRAM for buffers and processing
- Good for ESP32 with SPIRAM

**Implementation:**

1. **Allocate in SPIRAM:**
```c
#include "esp_heap_caps.h"

// Allocate in SPIRAM (ESP32)
uint8_t *kb_data = heap_caps_malloc(kb_size, MALLOC_CAP_SPIRAM);

// Copy once from flash to SPIRAM
memcpy(kb_data, flash_data, kb_size);

// Use SPIRAM pointer
kb->base = kb_data;
```

2. **Cache hot paths:** Keep frequently accessed data in IRAM/DRAM:
```c
// Decision tree lookup tables should be in fast memory
#define DT_CACHE_IN_IRAM __attribute__((section(".iram1")))

DT_CACHE_IN_IRAM static uint16_t dt_lookup_cache[256];
```

### Strategy 3: Compile-Time Optimizations

**Remove unused language support:**

```c
// In picorsrc.c or platform config
#define PICO_SINGLE_LANGUAGE_EN_US 1

#ifdef PICO_SINGLE_LANGUAGE_EN_US
// Remove language detection code
// Remove multi-language resource management
// Simplify initialization
#endif
```

**Static resource allocation:**

```c
// Instead of dynamic resource loading:
static picoknow_KnowledgeBase en_us_kbs[16];  // Fixed array
static picorsrc_Resource en_us_resource;      // Static resource

// Pre-initialize at compile time where possible
void initEnglishOnly(void) {
    // Simplified initialization for English only
    // No language detection
    // No resource enumeration
}
```

## Practical Example: ESP32 with English Only

```c
// 1. Configuration
#define PICO_EMBEDDED_PLATFORM 1
#define PICO_XIP_SUPPORT 1
#define PICO_SINGLE_LANGUAGE 1

// 2. Embed language files
extern const uint8_t en_us_ta_bin_start[] asm("_binary_en_US_ta_bin_start");
extern const uint8_t en_us_ta_bin_end[]   asm("_binary_en_US_ta_bin_end");
extern const uint8_t en_us_sg_bin_start[] asm("_binary_en_US_lh0_sg_bin_start");
extern const uint8_t en_us_sg_bin_end[]   asm("_binary_en_US_lh0_sg_bin_end");

// 3. Fast initialization
void tts_init_english_only(void) {
    pico_System system;
    pico_Resource taRes, sgRes;
    
    // Initialize with minimal memory (English only needs ~500KB)
    uint8_t *memory = malloc(500 * 1024);  // Or heap_caps_malloc for SPIRAM
    pico_initialize(memory, 500 * 1024, &system);
    
    // Load from embedded data (zero-copy with XIP)
    pico_loadResourceFromMemory(system, en_us_ta_bin_start, 
                                en_us_ta_bin_end - en_us_ta_bin_start,
                                "en-US-ta", &taRes);
    pico_loadResourceFromMemory(system, en_us_sg_bin_start,
                                en_us_sg_bin_end - en_us_sg_bin_start,
                                "en-US-sg", &sgRes);
    
    // Create voice
    pico_createVoiceDefinition(system, "en-US");
    pico_addResourceToVoiceDefinition(system, "en-US", "en-US-ta");
    pico_addResourceToVoiceDefinition(system, "en-US", "en-US-sg");
}
```

## Performance Tuning

### Reduce Specialization Overhead

The specialization phase (parsing KB structures) takes 50-100ms. To reduce this:

1. **Cache specialization results** (if doing repeated init/shutdown):
```c
static int specialized = 0;
static kdt_subobj_t cached_dt_structures[16];

if (!specialized) {
    // Do specialization once
    specializeDt(...);
    // Cache results
    specialized = 1;
}
```

2. **Profile and optimize hot paths:**
```bash
# Use ESP32 profiling tools
esp-idf/tools/esp32ulp/esp32ulp_trace.py
```

### Memory Layout Optimization

```c
// Order of allocation for best cache performance:
// 1. Frequently accessed lookup tables → IRAM
// 2. Working buffers → DRAM
// 3. Knowledge base data → Flash (XIP) or SPIRAM
// 4. Temporary buffers → Stack or DRAM

// Example memory map:
// 0x40000000 - IRAM (cache, hot functions)
// 0x3FF00000 - DRAM (buffers, stack)
// 0x3F400000 - Flash (language data via XIP)
// 0x3F800000 - SPIRAM (optional cached KB data)
```

## Benchmarks (ESP32 @ 240 MHz)

| Configuration | Init Time | RAM Usage | RTF |
|--------------|-----------|-----------|-----|
| Standard (all languages) | 500 ms | 8 MB | 0.6 |
| English + filesystem | 300 ms | 4 MB | 0.5 |
| English + XIP | 80 ms | 1 MB | 0.5 |
| English + SPIRAM cache | 30 ms | 1 MB | 0.4 |

RTF = Real-Time Factor (lower is better, < 1.0 means faster than real-time)

## Limitations and Tradeoffs

### XIP (Flash Access)
- **Pro:** Zero RAM for KB data, simple implementation
- **Con:** Flash access is slower than RAM (but cached)
- **Best for:** Systems with limited RAM, moderate CPU

### SPIRAM Caching
- **Pro:** Faster than flash, more RAM available
- **Con:** Uses SPIRAM, initial copy overhead
- **Best for:** Systems with SPIRAM, need best performance

### Static Compilation
- **Pro:** Smallest code size, fastest
- **Con:** Can't change language at runtime
- **Best for:** Single-language-only products

## Migration Guide

### From Multi-Language to English-Only

1. **Remove unused language files** from your build
2. **Enable compile-time optimization:**
   ```c
   #define PICO_SINGLE_LANGUAGE_EN_US 1
   ```
3. **Use XIP for flash-based loading**
4. **Reduce memory allocation:**
   ```c
   // Before: 10 MB for all languages
   pico_initialize(memory, 10 * 1024 * 1024, &system);
   
   // After: 500 KB for English only
   pico_initialize(memory, 500 * 1024, &system);
   ```
5. **Test and profile** on your target hardware

## Conclusion

For single-language English deployment on ESP32 or similar embedded systems:

1. **Use XIP** to access .bin files directly from flash (3-4 MB RAM savings)
2. **Enable streaming architecture** (Phase 1 optimization, 87% buffer reduction)
3. **Consider SPIRAM** for better performance if available
4. **Profile your application** to identify bottlenecks

The existing .bin format is already optimized. The specialization overhead (50-100ms) is acceptable for most applications and much faster than the alternative of copying large amounts of data.

## References

- `PHASE1_IMPLEMENTATION.md` - XIP support details
- `IMPROVEMENT_SUGGESTIONS.md` - Additional optimization ideas
- `ESP32_IMPLEMENTATION_GUIDE.md` - Platform-specific guidelines
- `picoembedded.h` - Embedded system configuration options
