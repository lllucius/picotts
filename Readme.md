# Pico TTS

Text to speech voice sinthesizer from SVox, included in Android AOSP.

## Build and install in Linux

The source code is inside folder 'pico'

```
cd pico
```

Create autotools files:

```
./autogen.sh
```

Configure & build:

```
./configure
make
```

Install (this install files to /usr/bin, /usr/lib and /usr/share/pico):

```
make install
```

## Usage

```
pico2wave -l LANG -w OUT_WAV_FILE "text you want to sinthesize"
aplay OUT_WAV_FILE
rm OUT_WAV_FILE
```

Languages can be: en-EN, en-GB, es-ES, de-DE, fr-FR, it-IT

Output file must be .wav

## Voice Quality Improvements

PicoTTS includes voice quality enhancement features that improve audio output through low-shelf equalization filtering. This results in:

- Enhanced speech clarity and intelligibility
- Increased perceived loudness without clipping
- More natural-sounding frequency balance
- Better overall audio quality

### Available Implementations

**Standard (Floating-Point)** - `test2wave`
- Uses double-precision floating-point arithmetic
- Best quality on systems with FPU
- Suitable for desktop and modern embedded systems

**Embedded (Fixed-Point)** - `test2wave_embedded`
- Uses Q15 fixed-point arithmetic (no FPU required)
- Optimized for resource-constrained embedded devices
- 50-70% faster on ARM Cortex-M without FPU
- Same audio quality as floating-point version

For detailed information about the TTS synthesis algorithm and voice quality improvements, see:
- [IMPROVEMENTS.md](IMPROVEMENTS.md) - Comprehensive improvement suggestions for embedded and desktop
- [VOICE_QUALITY.md](VOICE_QUALITY.md) - Technical documentation on synthesis and filtering
- [SYNTHESIS_SUMMARY.md](SYNTHESIS_SUMMARY.md) - Executive summary for developers

## Examples and Documentation

### test2wave Example

A complete example program demonstrating PicoTTS library usage with voice quality enhancements:

```bash
cd pico
./test2wave output.wav "Hello world, this is a test."
```

See [pico/bin/TEST2WAVE_README.md](pico/bin/TEST2WAVE_README.md) for details.

### test2wave_embedded - Embedded Systems Version

An optimized version using fixed-point arithmetic for embedded devices without FPU:

```bash
cd pico
./test2wave_embedded output.wav "Embedded optimized test."
```

**Benefits:**
- No floating-point unit (FPU) required
- 50-70% faster on ARM Cortex-M processors
- Lower power consumption
- Same audio quality as floating-point version

Run the demo script to generate multiple example audio files:

```bash
cd pico
./bin/demo_test2wave.sh
```

### pico2wave Tool

The standard command-line tool for text-to-speech conversion:

```bash
pico2wave -l en-US -w output.wav "Your text here"
```

## License

License Apache-2.0 (see pico_resources/NOTICE)

