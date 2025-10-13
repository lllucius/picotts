# PicoTTS Algorithm Visual Overview

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        TEXT INPUT                               │
│                  "Hello, how are you?"                          │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TEXT PROCESSING                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │Tokenize  │→ │Normalize │→ │  G2P     │→ │ Prosody  │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
│   Split text    $5→5 dollars   text→/tɛkst/  Pitch/Duration    │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│          PHONEME-TO-ACOUSTIC MAPPING (PAM)                      │
│                                                                 │
│   Linguistic Features → Decision Trees → Acoustic Parameters   │
│                                                                 │
│   ┌─────────────┐      ┌─────────────┐     ┌──────────────┐   │
│   │  Phoneme    │      │   5 Trees   │     │ Mel-cepstral │   │
│   │  Context    │  →   │   for LFZ   │  →  │ Coefficients │   │
│   │  Features   │      │  (Pitch)    │     │   (25 dim)   │   │
│   └─────────────┘      └─────────────┘     └──────────────┘   │
│                                                                 │
│   ┌─────────────┐      ┌─────────────┐     ┌──────────────┐   │
│   │  Syllable   │      │   5 Trees   │     │   Log F0     │   │
│   │  Features   │  →   │   for MGC   │  →  │   Energy     │   │
│   │  (60 bytes) │      │ (Spectrum)  │     │   Duration   │   │
│   └─────────────┘      └─────────────┘     └──────────────┘   │
│                                                                 │
│   5-state HMM-like model per phoneme                           │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│              SIGNAL GENERATION (SIG)                            │
│                                                                 │
│  ┌─────────────────────────────────────────────────────┐       │
│  │         EXCITATION GENERATION                       │       │
│  │                                                      │       │
│  │  Voiced:         Unvoiced:                          │       │
│  │  ───┬───┬───     ▓░▒░▓░▒░▓                          │       │
│  │     │   │   │    Random noise                        │       │
│  │  Pitch pulses   at effective period                  │       │
│  └──────────────────────────┬───────────────────────────┘       │
│                             │                                   │
│                             ▼                                   │
│  ┌─────────────────────────────────────────────────────┐       │
│  │      MEL-CEPSTRAL VOCODING                          │       │
│  │                                                      │       │
│  │  Cepstrum → Spectrum → Phase → FFT → Time Domain    │       │
│  │                                                      │       │
│  │  [c₀,c₁...c₂₄] → |S(ω)| → ∠S(ω) → IFFT → s(t)      │       │
│  │   25 coeffs      256 bins  Min-phase  256pt  audio  │       │
│  └──────────────────────────┬───────────────────────────┘       │
│                             │                                   │
│                             ▼                                   │
│  ┌─────────────────────────────────────────────────────┐       │
│  │         OVERLAP-ADD SYNTHESIS                       │       │
│  │                                                      │       │
│  │  Frame N-1:  [════════════]                         │       │
│  │  Frame N  :          [════════════]                 │       │
│  │  Output   :  [──────────────────]                   │       │
│  │                 ↑         ↑                          │       │
│  │              75% overlap  Hann window                │       │
│  └──────────────────────────┬───────────────────────────┘       │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                  AUDIO OUTPUT                                   │
│            16-bit PCM @ 16 kHz                                  │
│   ▁▂▃▅▆▇█▇▆▅▃▂▁ ▁▂▃▅▆▇█▇▆▅▃▂▁ ▁▂▃▅▆▇█▇▆▅▃▂▁                    │
└─────────────────────────────────────────────────────────────────┘
```

## Memory Layout Diagram

```
ESP32 Memory Map (4 MB Flash, 520 KB SRAM)
═══════════════════════════════════════════════════════════════

FLASH (4 MB):                         SRAM (520 KB):
┌─────────────────┐ 0x00000          ┌─────────────────┐ 0x00000
│   Bootloader    │                  │   Stack         │ 
│     (64 KB)     │                  │    (32 KB)      │
├─────────────────┤ 0x10000          ├─────────────────┤
│                 │                  │   Heap          │
│   Application   │                  │  (dynamic)      │
│    (1.5 MB)     │                  │                 │
│                 │                  ├─────────────────┤
├─────────────────┤ 0x190000         │  TTS Runtime    │
│                 │                  │   Buffers       │
│  TTS Knowledge  │ ← XIP (no copy)  │  (70-100 KB)    │
│     Base        │                  │                 │
│  (1.5-3 MB)     │   Direct access  ├─────────────────┤
│                 │   from flash     │  Application    │
│  - Lexicon      │                  │     Data        │
│  - Decision     │                  │  (remaining)    │
│    Trees        │                  │                 │
│  - PDFs         │                  └─────────────────┘
│                 │
└─────────────────┘ 0x3FFFFF

Optimization: Use XIP (Execute-In-Place) to access KB directly
from flash → Saves 1.5-3 MB of RAM!
```

## Processing Pipeline Timeline

```
Time →
═══════════════════════════════════════════════════════════════

Text Input: "Hello"
│
├─ [Tokenization]                    ← 1 ms
│  Output: ["Hello"]
│
├─ [G2P Conversion]                  ← 5 ms
│  Output: [h ə l oʊ]
│  Lookup in lexicon (fast hash table)
│
├─ [Prosody Prediction]              ← 10 ms
│  Decision tree traversal (4 phonemes × 10 trees)
│  Output: Pitch contour, durations, energy
│
├─ [PAM Processing]                  ← 20 ms
│  5 states/phoneme × 4 phonemes = 20 frames
│  Each frame: 10 tree traversals → PDF lookup
│  Output: Mel-cepstral + F0 trajectories
│
├─ [Signal Generation]               ← 50 ms
│  20 frames × 2.5 ms/frame
│  For each frame:
│  ├─ Excitation generation          ← 0.1 ms
│  ├─ Mel-cepstral synthesis         ← 0.5 ms
│  ├─ FFT operations                 ← 1.5 ms (dominant!)
│  └─ Overlap-add                    ← 0.4 ms
│
└─ [Audio Output]
   Output: 320 ms of audio (0.32 sec)

Total Processing Time: ~86 ms
Real-Time Factor: 86/320 = 0.27 (3.7x faster than real-time)

Note: With optimizations (ESP-DSP FFT, fixed-point, dual-core)
```

## Decision Tree Example

```
Predict F0 (pitch) for phoneme 't':

                    ┌─────────────────────┐
                    │  Phoneme = vowel?   │
                    └──────────┬──────────┘
                              / \
                          No /   \ Yes
                            /     \
              ┌────────────┐       ┌──────────────┐
              │Position =  │       │ Stress =     │
              │ final?     │       │ primary?     │
              └─────┬──────┘       └──────┬───────┘
                   / \                    / \
               No /   \ Yes           No /   \ Yes
                 /     \                /     \
         ┌──────┐   ┌──────┐    ┌──────┐   ┌──────┐
         │PDF   │   │PDF   │    │PDF   │   │PDF   │
         │ 045  │   │ 046  │    │ 123  │   │ 124  │
         └──────┘   └──────┘    └──────┘   └──────┘
         ↓          ↓           ↓          ↓
      F0=85Hz    F0=95Hz     F0=110Hz   F0=140Hz
                                         (stressed)

Features examined: 2-3 per tree
Tree depth: ~8-12 levels
Total trees: 10 per phoneme state (5 LFZ + 5 MGC)
```

## FFT Processing Visualization

```
Mel-Cepstral to Audio Synthesis:

Input: Mel-cepstral coefficients [c₀, c₁, c₂, ... c₂₄]
        ↓
Step 1: Cepstrum → Log-Magnitude Spectrum
        │
        │  log|H(ω)| = Σ cₖ·cos(kω)    (Cosine transform)
        │              k=0
        ↓
        [log-magnitude spectrum, 128 bins]
        │
Step 2: Exponential → Magnitude Spectrum
        │
        │  |H(ω)| = exp(log|H(ω)|)     (Fast exp approx)
        │
        ↓
        [magnitude spectrum]
        │
Step 3: Minimum Phase Reconstruction
        │
        │  ∠H(ω) = -Im{Hilbert(log|H(ω)|)}
        │
        ↓
        [complex spectrum: magnitude + phase]
        │
Step 4: Inverse FFT (256-point)
        │
        │  h(t) = IFFT{|H(ω)|·e^(j∠H(ω))}
        │
        ↓
        [impulse response, 256 samples]
        │
Step 5: Convolution with Excitation
        │
        │  s(t) = h(t) * e(t)
        │         ↑       ↑
        │      filter  excitation
        ↓
        [audio frame, 64 new samples]

Total operations per frame: ~5000 FLOPS
At 250 frames/sec: 1.25 MFLOPS
```

## Optimization Impact Chart

```
Performance Improvement by Optimization:

                              Baseline (no optimization)
                              │
ESP-DSP FFT ─────────────────►├──────────────────┐ +50%
                              │                  │
Fixed-point DSP ─────────────►├──────────────┐   │ +40%
                              │              │   │
Decision tree cache ─────────►├──────────┐   │   │ +25%
                              │          │   │   │
Dual-core pipeline ──────────►├────────┐ │   │   │ +50%
                              │        │ │   │   │
Lazy init + buffers ─────────►├─────┐  │ │   │   │ +15%
                              │     │  │ │   │   │
                              0    100 200 300 400 500%

Combined effect (not additive): ~3-4x overall speedup
Real-time factor: 0.8 → 0.25 (baseline → optimized)
```

## Memory Optimization Flow

```
Memory Usage Reduction:

BASELINE (Unoptimized):
┌──────────────────────────────────────────────────┐
│ Knowledge Base in RAM          │ 3 MB            │
├────────────────────────────────┼─────────────────┤
│ Sentence buffer (400 phonemes) │ 24 KB           │
├────────────────────────────────┼─────────────────┤
│ FFT/DSP buffers                │ 10 KB           │
├────────────────────────────────┼─────────────────┤
│ Engine state                   │ 80 KB           │
├────────────────────────────────┼─────────────────┤
│ TOTAL RAM                      │ ~3.1 MB         │ ❌ TOO LARGE!
└──────────────────────────────────────────────────┘

            ↓ Apply XIP (Execute-In-Place)
            
STEP 1 (XIP enabled):
┌──────────────────────────────────────────────────┐
│ Knowledge Base (in flash, XIP) │ 0 KB ✓         │
├────────────────────────────────┼─────────────────┤
│ Sentence buffer (400 phonemes) │ 24 KB           │
├────────────────────────────────┼─────────────────┤
│ FFT/DSP buffers                │ 10 KB           │
├────────────────────────────────┼─────────────────┤
│ Engine state                   │ 80 KB           │
├────────────────────────────────┼─────────────────┤
│ TOTAL RAM                      │ ~114 KB         │ ⚠️ Borderline
└──────────────────────────────────────────────────┘

            ↓ Apply Streaming + Buffer Reduction
            
STEP 2 (Optimized):
┌──────────────────────────────────────────────────┐
│ Knowledge Base (in flash, XIP) │ 0 KB ✓         │
├────────────────────────────────┼─────────────────┤
│ Streaming buffer (32 phonemes) │ 2 KB ✓         │
├────────────────────────────────┼─────────────────┤
│ FFT/DSP buffers (reduced)      │ 4 KB ✓         │
├────────────────────────────────┼─────────────────┤
│ Engine state (optimized)       │ 60 KB ✓        │
├────────────────────────────────┼─────────────────┤
│ TOTAL RAM                      │ ~66 KB          │ ✅ PERFECT!
└──────────────────────────────────────────────────┘

Result: 47x memory reduction (3.1 MB → 66 KB)
Fits comfortably in ESP32's 520 KB SRAM
```

## Quality vs Performance Trade-off Space

```
                Quality (MOS)
                    ↑
                4.5 │                    ● Neural TTS
                    │                   (Tacotron, FastSpeech)
                4.0 │              ● Unit Selection
                    │             (Flite, Festival)
                3.5 │          ●                    ↗ Optimized PicoTTS
                    │         ╱                    ╱  (22 kHz, enhanced)
                3.0 │    ● ──┘                    ╱
                    │    │  PicoTTS Baseline     ╱
                2.5 │    └─ Formant Synthesis   ╱
                    │       (eSpeak)           ╱
                2.0 │                        ╱
                    │                      ╱
                1.5 │                    ╱
                    └────┴────┴────┴────┴────┴────┴────┴──→
                    0.1  0.5  1    5   10   50  100  500  Compute (MFLOPS)

Memory Footprint (bubble size):
● Small:   < 1 MB    (formant, PicoTTS)
● Medium:  1-10 MB   (unit selection)
● Large:   > 10 MB   (neural TTS)

Sweet Spot for ESP32: 
PicoTTS optimized → 3.5 MOS, 0.5 MFLOPS, < 1 MB
```

## Dual-Core Pipeline Architecture

```
ESP32 Dual-Core Utilization:

CORE 0 (Text Processing):              CORE 1 (Audio Synthesis):
┌─────────────────────────┐            ┌─────────────────────────┐
│                         │            │                         │
│  ┌────────────────┐    │            │    ┌────────────────┐  │
│  │  Text Input    │    │            │    │   Acoustic     │  │
│  └───────┬────────┘    │            │    │   Parameters   │  │
│          │             │            │    └────────┬───────┘  │
│          ▼             │            │             │          │
│  ┌────────────────┐    │            │             ▼          │
│  │  Tokenization  │    │            │    ┌────────────────┐  │
│  └───────┬────────┘    │            │    │  Excitation    │  │
│          │             │            │    │  Generation    │  │
│          ▼             │            │    └────────┬───────┘  │
│  ┌────────────────┐    │            │             │          │
│  │  G2P + Prosody │    │            │             ▼          │
│  └───────┬────────┘    │            │    ┌────────────────┐  │
│          │             │            │    │  Mel-Cepstral  │  │
│          ▼             │            │    │  Synthesis     │  │
│  ┌────────────────┐    │   Queue    │    └────────┬───────┘  │
│  │      PAM       │────┼────────►───┼────────────►│          │
│  │  (Acoustic     │    │  (FIFO)    │             ▼          │
│  │   Prediction)  │    │            │    ┌────────────────┐  │
│  └────────────────┘    │            │    │  FFT/IFFT      │  │
│          │             │            │    └────────┬───────┘  │
│          │ (loop)      │            │             │          │
│          └─────────────┼────┐       │             ▼          │
│                        │    │       │    ┌────────────────┐  │
└────────────────────────┘    │       │    │  Overlap-Add   │  │
                              │       │    └────────┬───────┘  │
                              │       │             │          │
                              │       │             ▼          │
                              │       │    ┌────────────────┐  │
                              │       │    │  I2S Output    │  │
                              │       │    │  (DMA)         │  │
                              │       │    └────────────────┘  │
                              │       │             │          │
                              └───────┼─────────────┘          │
                                      │      (feedback)        │
                                      └────────────────────────┘

Benefits:
- 40-60% higher throughput (parallel processing)
- Lower latency (pipelined)
- Better CPU utilization (both cores active)
- Continuous output (no gaps)
```

## Algorithm Comparison Matrix

```
┌─────────────┬───────────┬───────────┬───────────┬─────────────┐
│             │  Formant  │  PicoTTS  │   Unit    │   Neural    │
│             │ Synthesis │(Statistical│ Selection │     TTS     │
│             │           │Parametric)│           │             │
├─────────────┼───────────┼───────────┼───────────┼─────────────┤
│ Memory      │   ★★★★★   │   ★★★★☆   │   ★★☆☆☆   │    ★☆☆☆☆    │
│ (smaller)   │   10 KB   │   100 KB  │   10 MB   │    50 MB    │
├─────────────┼───────────┼───────────┼───────────┼─────────────┤
│ Compute     │   ★★★★★   │   ★★★★☆   │   ★★★☆☆   │    ★☆☆☆☆    │
│ (faster)    │ 0.1 MFLOPS│ 1.3 MFLOPS│  5 MFLOPS │ 500 MFLOPS  │
├─────────────┼───────────┼───────────┼───────────┼─────────────┤
│ Quality     │   ★★☆☆☆   │   ★★★☆☆   │   ★★★★☆   │   ★★★★★     │
│             │ 2.5 MOS   │ 3.2 MOS   │ 3.8 MOS   │  4.3 MOS    │
├─────────────┼───────────┼───────────┼───────────┼─────────────┤
│ Latency     │   ★★★★★   │   ★★★★☆   │   ★★★☆☆   │    ★★☆☆☆    │
│             │  10 ms    │  50 ms    │  200 ms   │   1000 ms   │
├─────────────┼───────────┼───────────┼───────────┼─────────────┤
│ Flexibility │   ★★☆☆☆   │   ★★★☆☆   │   ★★★★☆   │   ★★★★★     │
│             │Rule-based │Tree-based │Multi-voice│ Expressive  │
├─────────────┼───────────┼───────────┼───────────┼─────────────┤
│ ESP32 Fit   │   ★★★★★   │   ★★★★★   │   ★★☆☆☆   │    ★☆☆☆☆    │
│             │  Perfect  │  Perfect  │  Difficult│ Impossible  │
└─────────────┴───────────┴───────────┴───────────┴─────────────┘

Legend: ★★★★★ Excellent  ★★★★☆ Good  ★★★☆☆ Fair  ★★☆☆☆ Poor  ★☆☆☆☆ Very Poor

Conclusion: PicoTTS offers best balance for ESP32
```

## Summary

These visualizations demonstrate:

1. **Architecture**: PicoTTS uses a sophisticated multi-stage pipeline
2. **Memory**: XIP and streaming reduce RAM usage by 47x
3. **Performance**: FFT dominates compute; ESP-DSP optimization critical
4. **Quality**: Sweet spot between efficiency and naturalness
5. **ESP32 Fit**: Excellent match for capabilities and constraints

The key to successful ESP32 deployment is implementing the critical optimizations (XIP, ESP-DSP FFT, streaming buffers, dual-core) which together enable real-time synthesis with minimal resources.
