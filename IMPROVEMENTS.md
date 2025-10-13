# TTS and Speech Synthesis Improvements for Embedded and Desktop Systems

## Executive Summary

This document provides recommendations for improving TTS and speech synthesis in PicoTTS that are suitable for both embedded devices and desktop systems. The suggestions focus on performance optimization, memory efficiency, and enhanced audio quality while maintaining compatibility across platforms.

## Current Implementation Analysis

The current voice quality filter implementation uses:
- Double-precision floating-point arithmetic (64-bit)
- IIR biquad filter structure
- Sample-by-sample processing

While effective, this can be optimized for embedded systems.

## Recommended Improvements

### 1. Fixed-Point Arithmetic Implementation (Embedded-Friendly)

**Problem**: Double-precision floating-point operations are expensive on many embedded processors without FPU.

**Solution**: Implement a fixed-point version of the filter alongside the floating-point version.

**Benefits**:
- 3-10x faster on ARM Cortex-M and similar embedded processors
- Deterministic performance and power consumption
- No FPU required

**Implementation**:

```c
/* Fixed-point filter configuration */
#define FIXEDPOINT_FRACBITS 15  // Q15 format (1 sign bit, 15 fraction bits)
#define FIXEDPOINT_SCALE (1 << FIXEDPOINT_FRACBITS)

typedef struct {
    int32_t m_fa, m_fb, m_fc, m_fd, m_fe;  // Q15 coefficients
    int32_t x1, x2;                         // Q15 input history
    int64_t out1, out2;                     // Q30 output history (for precision)
} VoiceQualityFilterFixed;

void initVoiceQualityFilterFixed(VoiceQualityFilterFixed *filter) {
    // Calculate coefficients in floating point, then convert to Q15
    double amp = pow(10.0, FILTER_LOWSHELF_ATTENUATION / 40.0);
    double w = 2.0 * M_PI * (FILTER_TRANSITION_FREQ / SAMPLE_RATE);
    double sinw = sin(w);
    double cosw = cos(w);
    double beta = sqrt(amp) / FILTER_SHELF_SLOPE;

    double b0 = amp * ((amp + 1.0) - ((amp - 1.0) * cosw) + (beta * sinw));
    double b1 = 2.0 * amp * ((amp - 1.0) - ((amp + 1.0) * cosw));
    double b2 = amp * ((amp + 1.0) - ((amp - 1.0) * cosw) - (beta * sinw));
    double a0 = (amp + 1.0) + ((amp - 1.0) * cosw) + (beta * sinw);
    double a1 = 2.0 * ((amp - 1.0) + ((amp + 1.0) * cosw));
    double a2 = -((amp + 1.0) + ((amp - 1.0) * cosw) - (beta * sinw));

    // Convert to fixed-point Q15
    filter->m_fa = (int32_t)((FILTER_GAIN * b0 / a0) * FIXEDPOINT_SCALE);
    filter->m_fb = (int32_t)((FILTER_GAIN * b1 / a0) * FIXEDPOINT_SCALE);
    filter->m_fc = (int32_t)((FILTER_GAIN * b2 / a0) * FIXEDPOINT_SCALE);
    filter->m_fd = (int32_t)((a1 / a0) * FIXEDPOINT_SCALE);
    filter->m_fe = (int32_t)((a2 / a0) * FIXEDPOINT_SCALE);

    filter->x1 = filter->x2 = 0;
    filter->out1 = filter->out2 = 0;
}

void applyVoiceQualityFilterFixed(VoiceQualityFilterFixed *filter, int16_t* buffer, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; i++) {
        int32_t x0 = (int32_t)buffer[i] << FIXEDPOINT_FRACBITS;  // Convert to Q15
        
        // Perform multiplication and accumulation (Q15 * Q15 = Q30)
        int64_t out0 = ((int64_t)filter->m_fa * x0) +
                      ((int64_t)filter->m_fb * filter->x1) +
                      ((int64_t)filter->m_fc * filter->x2) +
                      ((int64_t)filter->m_fd * (filter->out1 >> FIXEDPOINT_FRACBITS)) +
                      ((int64_t)filter->m_fe * (filter->out2 >> FIXEDPOINT_FRACBITS));

        // Update state
        filter->x2 = filter->x1;
        filter->x1 = x0;
        filter->out2 = filter->out1;
        filter->out1 = out0;

        // Convert back to int16 with saturation
        int32_t result = (int32_t)(out0 >> (2 * FIXEDPOINT_FRACBITS));
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        buffer[i] = (int16_t)result;
    }
}
```

### 2. SIMD Optimization (Desktop Performance)

**Problem**: Processing samples one-by-one doesn't utilize modern CPU vector capabilities.

**Solution**: Implement SIMD versions using SSE2 (x86/x64) and NEON (ARM).

**Benefits**:
- 4-8x speedup on desktop CPUs
- Lower CPU usage enables higher synthesis quality or parallel processing
- Better battery life on laptops

**Implementation Outline**:

```c
#ifdef __SSE2__
#include <emmintrin.h>

void applyVoiceQualityFilterSSE2(VoiceQualityFilter *filter, int16_t* buffer, size_t sampleCount) {
    // Process 4 samples at a time using SSE2 intrinsics
    // Fall back to scalar processing for remaining samples
    size_t simd_count = (sampleCount / 4) * 4;
    
    // SSE2 processing here...
    
    // Handle remaining samples with scalar code
    for (size_t i = simd_count; i < sampleCount; i++) {
        // Scalar processing
    }
}
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>

void applyVoiceQualityFilterNEON(VoiceQualityFilter *filter, int16_t* buffer, size_t sampleCount) {
    // Process 4-8 samples at a time using NEON intrinsics
}
#endif
```

### 3. Runtime Filter Selection

**Problem**: Need to choose optimal implementation at runtime based on platform.

**Solution**: Add capability detection and automatic selection.

```c
typedef enum {
    FILTER_IMPL_FLOAT,      // Standard floating-point
    FILTER_IMPL_FIXED,      // Fixed-point for embedded
    FILTER_IMPL_SSE2,       // SSE2 SIMD (x86/x64)
    FILTER_IMPL_NEON        // ARM NEON SIMD
} FilterImplementation;

FilterImplementation selectBestFilterImpl(void) {
#ifdef __ARM_NEON__
    return FILTER_IMPL_NEON;
#elif defined(__SSE2__)
    return FILTER_IMPL_SSE2;
#elif defined(EMBEDDED_DEVICE) || !defined(__FPU_PRESENT)
    return FILTER_IMPL_FIXED;
#else
    return FILTER_IMPL_FLOAT;
#endif
}
```

### 4. Memory Optimization for Embedded Systems

**Problem**: 2.5 MB memory allocation may be excessive for some embedded systems.

**Solution**: Add configurable memory tiers.

```c
/* Memory configuration profiles */
#ifdef EMBEDDED_TINY
#define PICO_MEM_SIZE       1000000   // 1 MB for microcontrollers
#define MAX_OUTBUF_SIZE     64
#elif defined(EMBEDDED_SMALL)
#define PICO_MEM_SIZE       1500000   // 1.5 MB for small embedded
#define MAX_OUTBUF_SIZE     96
#else
#define PICO_MEM_SIZE       2500000   // 2.5 MB for desktop/larger embedded
#define MAX_OUTBUF_SIZE     128
#endif
```

### 5. Adaptive Quality Settings

**Problem**: Fixed filter parameters may not be optimal for all use cases.

**Solution**: Add quality presets and adaptive filtering.

```c
typedef enum {
    QUALITY_MINIMAL,        // Minimal processing for embedded
    QUALITY_STANDARD,       // Current implementation
    QUALITY_ENHANCED,       // Additional post-processing
    QUALITY_MAXIMUM         // All enhancements enabled
} QualityLevel;

typedef struct {
    float lowshelfAttenuation;
    float transitionFreq;
    float gain;
    int enableDynamicRange;     // Enable dynamic range compression
    int enableNoiseGate;        // Enable noise gating
} QualityProfile;

const QualityProfile profiles[] = {
    // MINIMAL: Fast, low overhead for embedded
    {-12.0f, 1100.0f, 3.5f, 0, 0},
    
    // STANDARD: Current implementation
    {-18.0f, 1100.0f, 5.5f, 0, 0},
    
    // ENHANCED: Better quality with moderate overhead
    {-18.0f, 1100.0f, 5.5f, 1, 0},
    
    // MAXIMUM: Best quality, desktop-oriented
    {-18.0f, 1100.0f, 5.5f, 1, 1}
};
```

### 6. Dynamic Range Compression (Optional Enhancement)

**Problem**: Inconsistent volume levels in synthesized speech.

**Solution**: Add simple look-ahead compressor for more consistent levels.

```c
typedef struct {
    float threshold;        // dB threshold for compression
    float ratio;           // Compression ratio
    float attack;          // Attack time in ms
    float release;         // Release time in ms
    float makeup_gain;     // Post-compression gain
    float env;             // Current envelope level
} SimpleCompressor;

void applyCompression(SimpleCompressor *comp, int16_t* buffer, size_t sampleCount, float sampleRate) {
    float attack_coef = exp(-1.0f / (sampleRate * comp->attack / 1000.0f));
    float release_coef = exp(-1.0f / (sampleRate * comp->release / 1000.0f));
    
    for (size_t i = 0; i < sampleCount; i++) {
        float sample = buffer[i] / 32768.0f;
        float abs_sample = fabs(sample);
        
        // Envelope follower
        if (abs_sample > comp->env) {
            comp->env = attack_coef * comp->env + (1.0f - attack_coef) * abs_sample;
        } else {
            comp->env = release_coef * comp->env + (1.0f - release_coef) * abs_sample;
        }
        
        // Calculate gain reduction
        float env_db = 20.0f * log10f(comp->env + 1e-10f);
        float gain = 1.0f;
        if (env_db > comp->threshold) {
            float excess = env_db - comp->threshold;
            gain = powf(10.0f, -(excess * (1.0f - 1.0f / comp->ratio)) / 20.0f);
        }
        
        // Apply gain with makeup
        buffer[i] = (int16_t)(sample * gain * comp->makeup_gain * 32768.0f);
    }
}
```

### 7. Noise Gate (Optional Enhancement)

**Problem**: Synthesis artifacts during silence.

**Solution**: Add simple noise gate to clean up silent passages.

```c
typedef struct {
    float threshold_db;     // Threshold in dB
    float hysteresis_db;    // Hysteresis to prevent chattering
    int is_open;           // Current gate state
    int hold_samples;      // Samples to hold gate open
    int hold_counter;      // Current hold counter
} NoiseGate;

void applyNoiseGate(NoiseGate *gate, int16_t* buffer, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; i++) {
        float level_db = 20.0f * log10f(fabs(buffer[i]) / 32768.0f + 1e-10f);
        
        if (gate->is_open) {
            if (level_db < gate->threshold_db - gate->hysteresis_db) {
                if (gate->hold_counter > 0) {
                    gate->hold_counter--;
                } else {
                    gate->is_open = 0;
                }
            } else {
                gate->hold_counter = gate->hold_samples;
            }
        } else {
            if (level_db > gate->threshold_db) {
                gate->is_open = 1;
                gate->hold_counter = gate->hold_samples;
            }
        }
        
        if (!gate->is_open) {
            buffer[i] = 0;
        }
    }
}
```

## Implementation Roadmap

### Phase 1: Core Optimizations (Embedded Focus)
1. Fixed-point arithmetic implementation
2. Memory optimization profiles
3. Runtime selection framework

**Target**: 50-70% performance improvement on embedded devices without FPU

### Phase 2: SIMD Optimization (Desktop Focus)
1. SSE2 implementation for x86/x64
2. NEON implementation for ARM
3. Automatic fallback to scalar code

**Target**: 4-6x speedup on desktop systems

### Phase 3: Quality Enhancements (Optional)
1. Dynamic range compression
2. Noise gate
3. Quality presets system
4. Adaptive filtering based on content

**Target**: Measurable improvement in perceived quality

## Performance Expectations

| Platform | Current | Fixed-Point | SIMD | Combined |
|----------|---------|-------------|------|----------|
| ARM Cortex-M4 (no FPU) | 100% | 30% | N/A | 30% |
| ARM Cortex-A7 (with FPU) | 100% | 85% | 20% | 17% |
| x86-64 Desktop | 100% | 95% | 15% | 14% |
| ARM64 (NEON) | 100% | 90% | 18% | 16% |

*Percentages show relative CPU time (lower is better)*

## Compatibility Matrix

| Feature | Embedded Tiny | Embedded Small | Desktop | Notes |
|---------|--------------|----------------|---------|-------|
| Fixed-point filter | ✅ Recommended | ✅ Optional | ⚠️ Fallback | Best for no-FPU systems |
| Floating-point filter | ⚠️ If FPU | ✅ Recommended | ✅ Default | Requires FPU |
| SIMD optimization | ❌ | ⚠️ ARM NEON | ✅ Recommended | Desktop/high-end ARM |
| Dynamic compression | ❌ | ⚠️ Optional | ✅ Optional | CPU intensive |
| Noise gate | ✅ Minimal | ✅ Optional | ✅ Optional | Very low overhead |
| Memory < 1 MB | ✅ | ❌ | ❌ | Tiny profile |
| Memory 1-2 MB | ✅ | ✅ | ❌ | Small profile |
| Memory > 2 MB | ✅ | ✅ | ✅ | Standard profile |

## Testing Recommendations

### Embedded Device Testing
1. Test on actual hardware (not just simulators)
2. Measure real-time factor (RTF < 0.3 for embedded)
3. Monitor power consumption
4. Verify in resource-constrained scenarios

### Desktop Testing
1. Benchmark with different CPU architectures
2. Test with/without SIMD optimizations
3. Verify audio quality with objective metrics (PESQ, POLQA)
4. Profile for optimization opportunities

### Cross-Platform Testing
1. Ensure bit-exact output across implementations (where possible)
2. Test fixed-point vs floating-point quality differences
3. Verify graceful fallback when optimizations unavailable

## Conclusion

These improvements provide a path to better performance and quality across both embedded devices and desktop systems:

- **Embedded focus**: Fixed-point arithmetic, memory optimization, minimal overhead
- **Desktop focus**: SIMD acceleration, quality enhancements, optional features
- **Universal**: Smart runtime selection, quality presets, modular design

All improvements can be implemented incrementally, allowing gradual adoption and testing.
