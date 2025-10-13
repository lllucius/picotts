# PicoTTS Command Line Tools

This directory contains command-line utilities for text-to-speech conversion.

## Programs

### pico2wave

Standard PicoTTS text-to-speech converter (original).

**Usage:**
```bash
pico2wave -w output.wav "Hello, world!"
pico2wave -w output.wav -l en-GB "British English"
echo "Text from stdin" | pico2wave -w output.wav
```

**Options:**
- `-w, --wave=filename.wav` - Output WAV file (required)
- `-l, --lang=lang` - Language (en-US, en-GB, de-DE, es-ES, fr-FR, it-IT)

### pico2wave_quality

Enhanced text-to-speech converter with Phase 3 quality improvements.

**Usage:**
```bash
# Basic usage (same as pico2wave)
pico2wave_quality -w output.wav "Hello, world!"

# With voice profiles
pico2wave_quality -w output.wav --voice female "Female voice"
pico2wave_quality -w output.wav --voice male "Male voice"
pico2wave_quality -w output.wav --voice child "Child voice"

# With quality modes
pico2wave_quality -w output.wav --quality speed "Fast synthesis"
pico2wave_quality -w output.wav --quality high "Best quality"

# With custom parameters
pico2wave_quality -w output.wav --pitch 1.2 --speed 0.9 "Custom voice"
pico2wave_quality -w output.wav --formant 150 "Shifted formants"

# Show statistics
pico2wave_quality -w output.wav --stats "Show quality stats"

# Reading from stdin
echo "Hello from stdin" | pico2wave_quality -w output.wav
```

**Options:**
- `-w, --wave=filename.wav` - Output WAV file (required)
- `-l, --lang=lang` - Language (en-US, en-GB, de-DE, es-ES, fr-FR, it-IT)
- `-v, --voice=profile` - Voice profile (default, male, female, child, robot, slow, fast)
- `-q, --quality=mode` - Quality mode (speed, balanced, high)
- `-p, --pitch=scale` - Pitch scaling (0.5-2.0, default 1.0)
- `-s, --speed=scale` - Speed scaling (0.5-3.0, default 1.0)
- `-f, --formant=shift` - Formant shift in Hz (-500 to +500, default 0)
- `-S, --stats` - Show quality enhancement statistics

## Building

### Standard Build (without quality enhancements)

```bash
cd pico
./autogen.sh
./configure
make
```

This builds both `pico2wave` and `pico2wave_quality`, but `pico2wave_quality` requires the quality enhancement library to be compiled in.

### With Quality Enhancements

```bash
cd pico
./autogen.sh
CFLAGS="-DPICO_USE_QUALITY_ENHANCE=1" ./configure
make
```

This enables quality enhancements in the library and `pico2wave_quality` will have full functionality.

### Installing

```bash
sudo make install
```

This installs the programs to `/usr/local/bin` and language files to `/usr/local/share/pico/lang`.

## Testing Quality Improvements

### Test Different Voice Profiles

```bash
# Generate samples with different voices
pico2wave_quality -w male.wav --voice male "This is a male voice"
pico2wave_quality -w female.wav --voice female "This is a female voice"
pico2wave_quality -w child.wav --voice child "This is a child voice"
pico2wave_quality -w robot.wav --voice robot "This is a robot voice"

# Play them back
aplay male.wav
aplay female.wav
aplay child.wav
aplay robot.wav
```

### Test Quality Modes

```bash
# Generate samples with different quality settings
pico2wave_quality -w speed.wav --quality speed "Quick notification mode"
pico2wave_quality -w balanced.wav --quality balanced "Balanced quality"
pico2wave_quality -w high.wav --quality high "High quality mode"

# Compare file sizes and quality
ls -lh *.wav
aplay speed.wav balanced.wav high.wav
```

### Test Custom Parameters

```bash
# Test pitch variations
pico2wave_quality -w pitch_low.wav --pitch 0.8 "Lower pitch voice"
pico2wave_quality -w pitch_normal.wav --pitch 1.0 "Normal pitch voice"
pico2wave_quality -w pitch_high.wav --pitch 1.3 "Higher pitch voice"

# Test speed variations
pico2wave_quality -w speed_slow.wav --speed 0.7 "Slower speech"
pico2wave_quality -w speed_normal.wav --speed 1.0 "Normal speed"
pico2wave_quality -w speed_fast.wav --speed 1.5 "Faster speech"

# Test formant shift
pico2wave_quality -w formant_down.wav --formant -150 "Formants shifted down"
pico2wave_quality -w formant_up.wav --formant 150 "Formants shifted up"
```

### Test Fricatives and Consonants

Use sentences with lots of fricatives and consonants to test the improved excitation:

```bash
# Test fricatives (s, sh, f, th)
pico2wave_quality -w fricatives.wav "She sells seashells by the seashore. Fresh fish and french fries. Think thoughtfully through these things."

# Test consonants (p, t, k)
pico2wave_quality -w consonants.wav "Peter Piper picked a peck of pickled peppers. Keep calm and carry on. Tick tock goes the clock."

# Compare with standard version
pico2wave -w fricatives_standard.wav "She sells seashells by the seashore."
pico2wave_quality -w fricatives_enhanced.wav "She sells seashells by the seashore."
```

### Benchmark Performance

```bash
# Time synthesis with different quality modes
time pico2wave_quality -w test_speed.wav --quality speed "The quick brown fox jumps over the lazy dog"
time pico2wave_quality -w test_balanced.wav --quality balanced "The quick brown fox jumps over the lazy dog"
time pico2wave_quality -w test_high.wav --quality high "The quick brown fox jumps over the lazy dog"

# Show statistics
pico2wave_quality -w test.wav --stats "Hello, world!"
```

## Scripting Examples

### Batch Conversion

```bash
#!/bin/bash
# Convert multiple texts to speech

while IFS= read -r line; do
    filename=$(echo "$line" | tr ' ' '_' | cut -c1-30).wav
    pico2wave_quality -w "$filename" --voice female "$line"
    echo "Generated: $filename"
done < input_texts.txt
```

### Voice Assistant Script

```bash
#!/bin/bash
# Simple voice assistant

read -p "Enter text: " text
pico2wave_quality -w /tmp/response.wav --voice female --quality balanced "$text"
aplay /tmp/response.wav
rm /tmp/response.wav
```

### Multi-Language Demo

```bash
#!/bin/bash
# Demonstrate multiple languages

pico2wave_quality -w en-us.wav -l en-US "Hello from America"
pico2wave_quality -w en-gb.wav -l en-GB "Hello from Britain"
pico2wave_quality -w de-de.wav -l de-DE "Guten Tag aus Deutschland"
pico2wave_quality -w es-es.wav -l es-ES "Hola desde España"
pico2wave_quality -w fr-fr.wav -l fr-FR "Bonjour de France"
pico2wave_quality -w it-it.wav -l it-IT "Ciao dall'Italia"

# Play all
for f in *.wav; do
    echo "Playing $f"
    aplay "$f"
    sleep 1
done
```

## Troubleshooting

### Missing Language Files

If you get errors about missing language files, make sure they're installed:

```bash
# Check if language files exist
ls -l /usr/local/share/pico/lang/

# If not installed, run make install
cd pico
sudo make install
```

### Missing libpopt

If compilation fails with "popt.h not found":

```bash
# On Debian/Ubuntu
sudo apt-get install libpopt-dev

# On Fedora/RHEL
sudo dnf install popt-devel

# On macOS with Homebrew
brew install popt
```

### Quality Enhancements Not Working

If `pico2wave_quality` doesn't show quality options:

```bash
# Rebuild with quality enhancements enabled
cd pico
make clean
CFLAGS="-DPICO_USE_QUALITY_ENHANCE=1" ./configure
make
```

Check if quality enhancements are compiled in:

```bash
pico2wave_quality -w test.wav --voice female "test"
# Should show: "Quality enhancements enabled"
```

## Performance Notes

### Speed vs Quality

| Quality Mode | RTF (relative) | Use Case |
|--------------|----------------|----------|
| Speed | ~0.25 | Quick notifications, alerts |
| Balanced | ~0.35 | General TTS, voice assistants |
| High | ~0.55 | Audiobooks, accessibility |

RTF = Real-Time Factor (lower is faster)
- RTF 0.25 = 4x faster than real-time
- RTF 0.35 = ~3x faster than real-time
- RTF 0.55 = ~2x faster than real-time

All modes are real-time capable on modern hardware.

### Memory Usage

- Standard: ~150 KB RAM
- With quality enhancements: ~150 KB + 200 bytes
- Language files: 3-7 MB per language (on disk)

## See Also

- **PHASE3_QUALITY_IMPROVEMENTS.md** - Complete quality enhancement documentation
- **examples/quality_example.c** - Programmatic API usage
- **ESP32_IMPLEMENTATION_GUIDE.md** - ESP32 integration guide
- **picoqualityenhance.h** - Quality enhancement API reference

## License

These tools are provided under the Apache 2.0 license, same as PicoTTS.
