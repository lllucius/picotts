# PicoTTS Documentation Index

This index helps you find the right documentation for your needs.

## Quick Links

- **Just want to try it?** → [QUICKSTART.md](QUICKSTART.md)
- **Want example code?** → [pico/bin/test2wave.c](pico/bin/test2wave.c) and [pico/bin/TEST2WAVE_README.md](pico/bin/TEST2WAVE_README.md)
- **Need technical details?** → [VOICE_QUALITY.md](VOICE_QUALITY.md)
- **Want an overview?** → [SYNTHESIS_SUMMARY.md](SYNTHESIS_SUMMARY.md)
- **Visual learner?** → [FILTER_VISUALIZATION.md](FILTER_VISUALIZATION.md)

## Documentation Structure

### For Users

1. **[Readme.md](Readme.md)** - Main README
   - Build and installation instructions
   - Basic usage
   - Overview of features

2. **[QUICKSTART.md](QUICKSTART.md)** - Quick Start Guide
   - How to get started quickly
   - Simple examples
   - Troubleshooting common issues
   - Performance notes

3. **[pico/bin/TEST2WAVE_README.md](pico/bin/TEST2WAVE_README.md)** - test2wave User Guide
   - How to use the test2wave example
   - Command-line arguments
   - Usage examples
   - Language support

### For Developers

4. **[SYNTHESIS_SUMMARY.md](SYNTHESIS_SUMMARY.md)** - Executive Summary
   - Synthesis algorithm overview
   - Voice quality improvements delivered
   - Technical architecture
   - Implementation details
   - Build and test instructions

5. **[VOICE_QUALITY.md](VOICE_QUALITY.md)** - Technical Documentation
   - Detailed TTS synthesis algorithm
   - Voice quality analysis
   - Filter design and mathematics
   - Performance characteristics
   - Implementation guidelines
   - Parameter tuning recommendations

6. **[FILTER_VISUALIZATION.md](FILTER_VISUALIZATION.md)** - Visual Guide
   - Frequency response graphs
   - Signal flow diagrams
   - Waveform comparisons
   - Spectral analysis
   - Audio quality metrics
   - Filter implementation diagrams

### Source Code

7. **[pico/bin/test2wave.c](pico/bin/test2wave.c)** - Example Program
   - Complete working implementation
   - Voice quality filter code
   - Heavily commented
   - Best practices demonstrated
   - Error handling examples

8. **[pico/bin/demo_test2wave.sh](pico/bin/demo_test2wave.sh)** - Demo Script
   - Automated demonstration
   - Multiple examples
   - Easy to run

9. **[pico/bin/pico2wave.c](pico/bin/pico2wave.c)** - Command-line Tool
   - Full-featured TTS tool
   - Reference implementation

## Documentation by Topic

### TTS Algorithm and Synthesis

- **Overview**: [SYNTHESIS_SUMMARY.md](SYNTHESIS_SUMMARY.md) - "Synthesis Algorithm Overview"
- **Details**: [VOICE_QUALITY.md](VOICE_QUALITY.md) - "TTS Synthesis Pipeline"
- **Visual**: [FILTER_VISUALIZATION.md](FILTER_VISUALIZATION.md) - "Signal Flow Diagram"

### Voice Quality Improvements

- **Overview**: [Readme.md](Readme.md) - "Voice Quality Improvements"
- **Quick Start**: [QUICKSTART.md](QUICKSTART.md) - "What Are Voice Quality Improvements?"
- **Analysis**: [VOICE_QUALITY.md](VOICE_QUALITY.md) - "Voice Quality Analysis"
- **Visual**: [FILTER_VISUALIZATION.md](FILTER_VISUALIZATION.md) - "Filter Frequency Response"

### Filter Implementation

- **Code**: [pico/bin/test2wave.c](pico/bin/test2wave.c) - `VoiceQualityFilter` structure
- **Theory**: [VOICE_QUALITY.md](VOICE_QUALITY.md) - "Low-Shelf Equalization Filter"
- **Math**: [VOICE_QUALITY.md](VOICE_QUALITY.md) - "Mathematical Implementation"
- **Visual**: [FILTER_VISUALIZATION.md](FILTER_VISUALIZATION.md) - "Biquad Structure"

### Usage and Examples

- **Quick Start**: [QUICKSTART.md](QUICKSTART.md) - All sections
- **test2wave**: [pico/bin/TEST2WAVE_README.md](pico/bin/TEST2WAVE_README.md)
- **Demo**: Run `pico/bin/demo_test2wave.sh`
- **Integration**: [QUICKSTART.md](QUICKSTART.md) - "Integrate into Your Application"

### Performance and Quality

- **Metrics**: [FILTER_VISUALIZATION.md](FILTER_VISUALIZATION.md) - "Audio Quality Metrics"
- **Analysis**: [VOICE_QUALITY.md](VOICE_QUALITY.md) - "Performance Characteristics"
- **Benchmarks**: [SYNTHESIS_SUMMARY.md](SYNTHESIS_SUMMARY.md) - "Performance Characteristics"

### Customization and Tuning

- **Quick Guide**: [QUICKSTART.md](QUICKSTART.md) - "Tuning the Filter"
- **Detailed**: [VOICE_QUALITY.md](VOICE_QUALITY.md) - "Parameter Tuning"
- **Code**: [pico/bin/test2wave.c](pico/bin/test2wave.c) - Filter constants at top of file

## Recommended Reading Paths

### Path 1: "I want to use it now"
1. [QUICKSTART.md](QUICKSTART.md)
2. [pico/bin/TEST2WAVE_README.md](pico/bin/TEST2WAVE_README.md)
3. Run `demo_test2wave.sh`

### Path 2: "I want to understand it"
1. [SYNTHESIS_SUMMARY.md](SYNTHESIS_SUMMARY.md)
2. [VOICE_QUALITY.md](VOICE_QUALITY.md)
3. [FILTER_VISUALIZATION.md](FILTER_VISUALIZATION.md)

### Path 3: "I want to integrate it"
1. [QUICKSTART.md](QUICKSTART.md) - "Integrate into Your Application"
2. Study [pico/bin/test2wave.c](pico/bin/test2wave.c)
3. [VOICE_QUALITY.md](VOICE_QUALITY.md) - "Implementation Details"

### Path 4: "I want to customize it"
1. [QUICKSTART.md](QUICKSTART.md) - "Tuning the Filter"
2. [VOICE_QUALITY.md](VOICE_QUALITY.md) - "Parameter Tuning"
3. Experiment with [pico/bin/test2wave.c](pico/bin/test2wave.c)

## File Summary

| File | Purpose | Length | Audience |
|------|---------|--------|----------|
| Readme.md | Main README | Medium | Everyone |
| QUICKSTART.md | Quick start guide | Long | Users |
| SYNTHESIS_SUMMARY.md | Executive summary | Long | Developers |
| VOICE_QUALITY.md | Technical docs | Long | Developers |
| FILTER_VISUALIZATION.md | Visual guide | Long | Visual learners |
| pico/bin/TEST2WAVE_README.md | Example guide | Medium | Users |
| pico/bin/test2wave.c | Example code | Long | Developers |
| pico/bin/demo_test2wave.sh | Demo script | Short | Everyone |

## Getting Help

1. Check the [QUICKSTART.md](QUICKSTART.md) troubleshooting section
2. Review the relevant documentation above
3. Study the example code in [pico/bin/test2wave.c](pico/bin/test2wave.c)
4. Consult the technical documentation in [VOICE_QUALITY.md](VOICE_QUALITY.md)

## Contributing

All documentation and code follows these principles:

- **Clarity**: Easy to understand
- **Completeness**: Covers all aspects
- **Correctness**: Technically accurate
- **Consistency**: Unified style and terminology
- **Code Quality**: Well-commented, follows best practices

## Version Information

This documentation corresponds to the voice quality improvements implementation for PicoTTS, including:

- test2wave example program
- Voice quality filter (low-shelf EQ)
- Comprehensive documentation suite
- Demo scripts and examples

All implementations are based on the proven filter design from the Android AOSP PicoTTS implementation.
