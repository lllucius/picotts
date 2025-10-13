# Test2Wave Example

## Overview

`test2wave` is a simple example program demonstrating how to use the PicoTTS library to convert text to speech with voice quality improvements. This example showcases the voice quality filter that enhances audio output by improving tonal balance and clarity.

## Features

- Demonstrates basic PicoTTS API usage
- Implements voice quality enhancement filter
- Shows proper resource management and cleanup
- Includes detailed comments explaining each step
- Applies low-shelf EQ filter for improved audio quality

## Building

The example is built automatically when you compile the PicoTTS library:

```bash
cd pico
./autogen.sh
./configure
make
```

This will create the `test2wave` executable in the `pico` directory.

## Usage

```bash
test2wave <output.wav> [text]
```

### Arguments

- `<output.wav>`: Path to the output WAV file (required)
- `[text]`: Text to synthesize (optional, defaults to a sample sentence)

### Examples

```bash
# Synthesize default text
./test2wave output.wav

# Synthesize custom text
./test2wave hello.wav "Hello, this is a test."

# Longer text
./test2wave speech.wav "The quick brown fox jumps over the lazy dog. This is a demonstration of text to speech synthesis with voice quality improvements."
```

## Output Format

The generated WAV files have the following characteristics:

- Sample rate: 16,000 Hz
- Bit depth: 16-bit signed integer
- Channels: Mono (single channel)
- Encoding: Linear PCM, little-endian

## Voice Quality Filter

The example includes a voice quality enhancement filter with the following parameters:

- **Type**: Low-shelf biquad IIR filter
- **Low-frequency attenuation**: -18 dB
- **Transition frequency**: 1100 Hz
- **Shelf slope (Q)**: 1.0
- **Overall gain**: 5.5x

This filter improves voice quality by:
1. Reducing excessive low-frequency energy
2. Creating headroom for overall amplification
3. Improving speech clarity and intelligibility
4. Providing a more natural frequency balance

## Code Structure

The example demonstrates:

1. **Initialization**: Allocating memory and initializing the PicoTTS system
2. **Resource Loading**: Loading language-specific text analysis and signal generation resources
3. **Voice Creation**: Creating and configuring a voice definition
4. **Engine Setup**: Creating the synthesis engine
5. **Filter Initialization**: Setting up the voice quality filter
6. **Synthesis Loop**: Processing text and generating audio samples
7. **Filter Application**: Applying the quality filter to improve output
8. **Output Writing**: Writing filtered audio to a WAV file
9. **Cleanup**: Properly releasing all resources

## Voice Quality Filter Algorithm

The filter uses a second-order (biquad) IIR filter with the difference equation:

```
y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
```

Coefficients are calculated based on the desired frequency response and applied with overflow protection to prevent clipping.

## Customization

You can modify the filter parameters at the top of `test2wave.c`:

```c
#define FILTER_LOWSHELF_ATTENUATION -18.0f  // Attenuation in dB
#define FILTER_TRANSITION_FREQ 1100.0f      // Transition frequency in Hz
#define FILTER_SHELF_SLOPE 1.0f             // Filter Q factor
#define FILTER_GAIN 5.5f                     // Overall gain
```

Adjust these values to tune the audio output to your preferences or specific use case.

## Language Support

The example currently uses en-US (American English) by default. To use other languages, modify the resource file names in the code:

```c
// Available languages:
// en-US (American English): en-US_ta.bin, en-US_lh0_sg.bin
// en-GB (British English): en-GB_ta.bin, en-GB_kh0_sg.bin
// de-DE (German): de-DE_ta.bin, de-DE_gl0_sg.bin
// es-ES (Spanish): es-ES_ta.bin, es-ES_zl0_sg.bin
// fr-FR (French): fr-FR_ta.bin, fr-FR_nk0_sg.bin
// it-IT (Italian): it-IT_ta.bin, it-IT_cm0_sg.bin
```

## See Also

- `pico2wave`: Full-featured command-line TTS tool
- `VOICE_QUALITY.md`: Detailed documentation on voice quality improvements
- PicoTTS API documentation in `lib/picoapi.h`

## License

This example is licensed under the Apache License 2.0, same as the PicoTTS library.
