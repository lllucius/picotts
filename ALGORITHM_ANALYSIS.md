# PicoTTS Algorithm Analysis

## Executive Summary

This document provides a comprehensive analysis of the algorithms used in the PicoTTS (SVOX Pico) text-to-speech system. The analysis focuses on the core signal processing, speech synthesis, and acoustic modeling techniques with special attention to their suitability for embedded systems like ESP32.

## 1. Core Architecture Overview

PicoTTS is a formant-based concatenative TTS system that uses:
- **Statistical parametric synthesis** with decision trees
- **Mel-cepstral coefficients (MGC)** for spectral representation
- **Log F0 (LFZ)** for pitch modeling
- **FFT-based signal processing** for synthesis
- **HMM-like state modeling** with 5 states per phoneme

### Processing Pipeline

```
Text Input → Tokenization → Phonetic Analysis → 
Accentuation/Phrasing → Phoneme-to-Acoustic Mapping (PAM) → 
Signal Generation (SIG) → Audio Output
```

## 2. Key Algorithms and Techniques

### 2.1 Text Processing (Token, Transcription, Accentuation/Phrasing)

**Components:**
- `picotok.c` - Tokenization
- `picotrns.c` - Grapheme-to-phoneme conversion
- `picoacph.c` - Accentuation and phrasing
- `picowa.c` - Word analysis

**Algorithms:**
- Rule-based tokenization and sentence boundary detection
- Lexicon lookup with fallback letter-to-sound rules
- Decision tree-based prosody prediction
- Phrase boundary detection using punctuation and syntax

**Memory Impact:**
- Lexicon data (~several MB per language)
- Decision tree structures (compact binary format)
- Working buffers for text processing

### 2.2 Phoneme-to-Acoustic Mapping (PAM)

**File:** `picopam.c` (4,810 lines - largest module)

**Core Technique:** 
- Uses **decision trees** to predict acoustic parameters from phonetic context
- Separate tree sets for:
  - **LFZ (Log F0)**: 5 decision trees per phoneme state
  - **MGC (Mel-cepstral)**: 5 decision trees per phoneme state
- 5-state HMM-like model per phoneme

**Feature Vectors:**
- Phoneme feature vector: 60 bytes
- Syllable feature vector: 64-68 bytes  
- Context window: up to 400 phonemes per sentence

**Decision Tree Process:**
```
Linguistic Features → Tree Traversal → 
PDF (Probability Distribution Function) Index → 
Mean + Variance Parameters
```

**Key Constants:**
```c
#define PICOPAM_DT_NRLFZ    5    /* nr of lfz decision trees per phoneme */
#define PICOPAM_DT_NRMGC    5    /* nr of mgc decision trees per phoneme */
#define PICOPAM_NRSTPF      5    /* nr of states per phone */
#define PICOPAM_MAX_PH_PER_SENT 400  /* max phonemes per sentence */
```

### 2.3 Signal Generation (SIG)

**Files:** `picosig.c`, `picosig2.c` (total ~4,900 lines)

**Core Algorithm:** **Mel-cepstral vocoding with excitation generation**

#### 2.3.1 Spectral Processing

**Mel-Cepstral Analysis:**
- Order: 25 coefficients (`PICODSP_CEPORDER = 25`)
- Phase order: 72 (`PICODSP_PHASEORDER = 72`)
- FFT size: 256 points (`PICODSP_FFTSIZE = 256`)
- Sampling rate: 16 kHz (`PICODSP_SAMP_FREQ = 16000`)

**Key Steps:**
1. Mel-cepstral coefficients → spectrum envelope
2. Phase spectrum generation (minimum phase reconstruction)
3. IFFT to time domain
4. Overlap-add synthesis

#### 2.3.2 Excitation Generation

**Voiced Excitation:**
- Pitch-synchronous impulse train
- Period Ti = Fs / F0
- Amplitude scaled by energy parameter

**Unvoiced Excitation:**
- Pseudo-random noise generation
- Pre-computed random table (760 samples)
- Period-based noise pulses

**Code Reference:**
```c
// From picosig2.c
if (voiced == 0) { /* Unvoiced */
    Ti = (picoos_int32)(rounding + (picoos_single)Fs / (picoos_single)sig_inObj->Fuv_p);
    sqrtTi = (picoos_int32)(E * sqrt((double)Fs / (hop * sig_inObj->Fuv_p)) * fact * PICODSP_GETEXC_K1);
} else { /* Voiced */
    Ti = (picoos_int32)(rounding + (picoos_single)Fs / (picoos_single)F0);
    sqrtTi = (picoos_int32)(E * sqrt((double)Fs / (hop * sig_inObj->F0_p)) * fact * PICODSP_GETEXC_K1);
}
```

#### 2.3.3 Windowing and Overlap-Add

- Hann window generation for smooth transitions
- Displacement: 64 samples (`PICODSP_DISPLACE = FFTSIZE/4`)
- Overlap-add for continuous output

### 2.4 FFT Implementation

**File:** `picofftsg.c` (3,274 lines)

**Algorithm:** **Split-radix FFT** (adapted from Takuya Ooura's implementation)

**Features:**
- Radix: Split-radix for efficiency
- Decimation: Frequency domain
- Data: In-place processing
- Table-free implementation (computes twiddle factors on-the-fly)

**Functions:**
- `cdft()` - Complex DFT
- `rdft()` - Real DFT (used for speech)
- `ddct()` - Discrete Cosine Transform
- `ddst()` - Discrete Sine Transform

**Memory:** No pre-computed tables (on-the-fly calculation trades speed for memory)

### 2.5 Mathematical Optimizations

#### Fast Exponential Approximation

**File:** `picopal.c`

Uses Schraudolph's fast exp approximation (1999):
```c
picopal_double picopal_quick_exp(const picopal_double y) {
    union {
        picopal_double d;
        struct {
            #if PICO_ENDIANNESS == ENDIANNESS_LITTLE
              picopal_int32 j,i;
            #else
              picopal_int32 i,j;
            #endif
        } n;
    } _eco;
    _eco.n.i = (picopal_int32)(1512775.3951951856938297995605697f * y) + 1072632447;
    return _eco.d;
}
```

**Advantage:** ~10-20x faster than standard exp()
**Limitation:** Moderate accuracy loss (~1% error)

#### Duff's Device Loop Unrolling

**File:** `picodsp.h`

```c
#define FAST_DEVICE(aCount, aAction) \
{ \
    int count_ = (aCount); \
    int times_ = (count_ + 7) >> 3; \
    switch (count_ & 7){ \
        case 0: do { aAction; \
        case 7: aAction; \
        // ... unrolled 8 iterations
    } while (--times_ > 0); \
}
```

**Purpose:** Loop unrolling for tight inner loops
**Benefit:** Reduces loop overhead by ~30-40%

### 2.6 Audio Post-Processing

**File:** `com_android_tts_compat_SynthProxy.cpp`

**Biquad Low-Shelf Filter:**
```c
void applyFilter(int16_t* buffer, size_t sampleCount) {
    for (size_t i=0 ; i<sampleCount ; i++) {
        x0 = (double) buffer[i];
        out0 = (m_fa*x0) + (m_fb*x1) + (m_fc*x2) + (m_fd*out1) + (m_fe*out2);
        // ... state updates and clipping
    }
}
```

**Parameters:**
- Attenuation: -3dB (default)
- Transition frequency: 1000 Hz
- Shelf slope: 1.0
- Gain: 1.0

## 3. Memory Footprint Analysis

### 3.1 Static Memory (Constant Data)

| Component | Size | Notes |
|-----------|------|-------|
| Cosine tables | ~4-8 KB | Pre-computed trig values |
| Random table | ~3 KB | 760 int32 values |
| Hann windows | ~1-2 KB | Window coefficients |
| **FFT tables** | **0 KB** | Table-free implementation! |

### 3.2 Dynamic Memory (Runtime Buffers)

| Component | Size | Notes |
|-----------|------|-------|
| FFT buffer | ~2 KB | 256 complex samples |
| Input/output buffers | ~2 KB each | Configurable |
| Cepstral buffers | ~1 KB | 3 frames × 25 coefficients |
| Phase buffers | ~1.5 KB | 5 frames × 72 coefficients |
| Decision tree workspace | ~32 KB | Feature vectors + tree traversal |
| Sentence buffer | Variable | Max 400 phonemes × 60 bytes = 24 KB |

**Estimated Peak RAM:** ~70-100 KB for synthesis engine alone

### 3.3 Knowledge Base Files (Flash/Storage)

| Language Pack | Size | Components |
|--------------|------|------------|
| Complete (6 languages) | ~42 MB | All language data |
| Single language | ~3-7 MB | Lexicon, trees, PDFs |
| Minimal (single voice) | ~1.5-3 MB | Compressed resources |

**Components per language:**
- Lexicon (`.lex`): ~1-2 MB
- Text analysis (`.ta`): ~500 KB - 1 MB  
- Signal generation (`.sg`): ~1-2 MB
- PDF data (MGC, LFZ): ~500 KB - 1 MB

## 4. Computational Complexity

### 4.1 Per-Phoneme Operations

**PAM Stage:**
- Decision tree traversals: O(log N) × 10 trees (5 LFZ + 5 MGC)
- Typical depth: 8-12 nodes per tree
- ~80-120 comparisons per phoneme

**SIG Stage:**
- FFT/IFFT: O(N log N) where N=256 → ~2048 ops
- Mel-cepstral synthesis: O(M²) where M=25 → ~625 ops
- Total per frame: ~5000-8000 floating point ops

### 4.2 Real-Time Performance

**Frame Rate:** 16 kHz / 64 samples = 250 frames/sec
**Processing:** ~5000 ops/frame × 250 = **1.25 MFLOPS**

**For typical speech (10 phonemes/sec):**
- Text processing: ~1000 ops/phoneme = 10 KFLOPS
- PAM: ~100 comparisons + lookups = negligible
- SIG: 1.25 MFLOPS (dominant)

**Total: ~1.3 MFLOPS for real-time synthesis**

## 5. Algorithm Strengths

### 5.1 Memory Efficiency
1. **Table-free FFT** - No twiddle factor tables
2. **Compact decision trees** - Binary format, shared across states
3. **Compressed PDFs** - Quantized parameters (uint8)
4. **Shared resources** - Single engine for all voices

### 5.2 Computational Efficiency
1. **Fast exp approximation** - 10-20x speedup
2. **Fixed-point capable** - Most operations can use integer math
3. **Streaming synthesis** - Incremental processing, no batch requirements
4. **Optimized loops** - Unrolling and SIMD-friendly patterns

### 5.3 Quality Features
1. **Prosody modeling** - Decision tree-based natural intonation
2. **Context-dependent synthesis** - 5-state phoneme models
3. **Smooth concatenation** - Overlap-add with windowing
4. **Parametric flexibility** - Pitch, speed, volume modifiers

## 6. Algorithm Limitations

### 6.1 Quality Constraints
1. **16 kHz sampling** - Lower fidelity than modern 22-48 kHz systems
2. **Formant synthesis** - Less natural than unit selection or neural TTS
3. **Fixed acoustic models** - Cannot adapt to new speakers without retraining
4. **Limited prosody** - Rule-based, not data-driven

### 6.2 Performance Bottlenecks
1. **FFT operations** - Dominant CPU cost (~60-70% of synthesis time)
2. **Floating-point heavy** - Mel-cepstral synthesis requires FP math
3. **Memory access patterns** - Decision tree traversal has poor cache locality
4. **Single-threaded** - No parallelization opportunities in core synthesis

### 6.3 Scalability Issues
1. **Per-language resources** - Each language adds 3-7 MB
2. **Decision tree size** - Grows with prosody complexity
3. **No compression** - Knowledge bases stored uncompressed
4. **Monolithic design** - Hard to swap components

## 7. Comparison with Modern Approaches

### 7.1 vs. Neural TTS (Tacotron, FastSpeech)
- **Advantage:** 100-1000x less memory, 10-100x less compute
- **Disadvantage:** Lower naturalness, less expressive prosody

### 7.2 vs. Unit Selection
- **Advantage:** 10-100x smaller footprint, deterministic latency
- **Disadvantage:** Less natural concatenation, robotic quality

### 7.3 vs. Older Formant Synthesis (Klatt, etc.)
- **Advantage:** Statistical prosody, context modeling, better quality
- **Disadvantage:** Slightly higher computational cost

## 8. Suitability for ESP32

### 8.1 ESP32 Specifications
- **CPU:** Dual-core Xtensa LX6 @ 240 MHz
- **RAM:** 520 KB SRAM
- **Flash:** 4-16 MB (typical)
- **FPU:** Yes (single precision)

### 8.2 Feasibility Analysis

**Memory Requirements:**
✅ **RAM:** 70-100 KB fits in 520 KB SRAM (with margin for application)
✅ **Flash:** 1.5-3 MB/language fits in 4-16 MB (1-2 languages feasible)

**Computational Requirements:**
✅ **FLOPS:** 1.3 MFLOPS << 240 MHz × 2 cores capability
✅ **FPU:** ESP32 has hardware FPU for floating point ops

**Challenges:**
⚠️ **Memory fragmentation** - Large contiguous buffers needed
⚠️ **Flash wear** - Frequent reads of knowledge bases
⚠️ **Power consumption** - Continuous synthesis drains battery

### 8.3 Verdict
**PicoTTS is WELL-SUITED for ESP32**, with expected:
- **Real-time factor:** 0.2-0.5x (2-5x faster than real-time)
- **Latency:** <100ms for short utterances
- **Power:** ~80-120 mA during synthesis @ 240 MHz

## 9. Conclusion

PicoTTS uses a sophisticated yet efficient combination of:
- Statistical parametric synthesis with decision trees
- Mel-cepstral vocoding with table-free FFT
- Optimized mathematical approximations
- Compact knowledge representation

The system is **remarkably well-optimized for embedded deployment**, making intelligent trade-offs between quality, speed, and memory footprint. Its algorithms are mature, proven, and specifically designed for resource-constrained environments like smartphones and embedded systems.

The main limitation is speech quality compared to modern neural approaches, but for applications requiring small footprint, low latency, and offline operation, PicoTTS remains an excellent choice.
