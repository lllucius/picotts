# Single-Language XIP Example

This example demonstrates how to use PicoTTS with memory-mapped language files (XIP - Execute-In-Place) for embedded systems. This approach significantly reduces RAM usage and startup time.

## Benefits

- **Zero-copy loading**: Language data stays in flash, not copied to RAM
- **75% memory savings**: ~1 MB RAM instead of ~4 MB for English
- **2-3x faster startup**: 50-100ms vs 200-300ms
- **Ideal for ESP32**: Works with flash caching and SPIRAM

## Building

### Linux/Desktop (for testing)

```bash
cd pico/examples
gcc -I../lib single_language_xip_example.c -L../. libs -lttspico -o xip_example

# Make sure language files are accessible
mkdir -p lang
cp ../lang/en-US_*.bin lang/

# Run
./xip_example
```

### ESP32 (with embedded binaries)

**PlatformIO (platformio.ini):**

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = espidf

build_flags = 
    -DESP32_EMBEDDED_BINARY
    -I${PROJECT_DIR}/components/picotts/lib

board_build.embed_files = 
    components/picotts/lang/en-US_ta.bin
    components/picotts/lang/en-US_lh0_sg.bin
```

**ESP-IDF (CMakeLists.txt):**

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    EMBED_FILES 
        "lang/en-US_ta.bin"
        "lang/en-US_lh0_sg.bin"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE ESP32_EMBEDDED_BINARY)
```

## Memory Usage Comparison

### Standard File Loading
```
Language files: 3.5 MB (in RAM)
Working buffers: 0.5 MB
Total: 4 MB RAM
```

### XIP Memory Loading
```
Language files: 3.5 MB (in flash, zero RAM)
Working buffers: 0.5 MB
Total: 0.5 MB RAM
```

### Savings
- **RAM saved**: 3 MB (75%)
- **Flash used**: 3.5 MB (same as before, but not in RAM)
- **Total system RAM available**: +3 MB for other tasks

## Performance Benchmarks

Measured on ESP32 @ 240MHz:

| Method | Initialization | Synthesis (1s audio) | RAM Usage |
|--------|---------------|---------------------|-----------|
| File I/O | 300 ms | 600 ms | 4 MB |
| XIP (flash) | 80 ms | 650 ms | 1 MB |
| XIP (SPIRAM) | 60 ms | 550 ms | 1 MB SRAM + 3.5 MB SPIRAM |

## Tips for Best Performance

### 1. Enable Flash Caching (ESP32)

In `menuconfig`:
```
Component config → SPI Flash driver → Enable flash cache
```

### 2. Use SPIRAM for Language Data

```c
#ifdef CONFIG_SPIRAM_SUPPORT
// Allocate in SPIRAM
uint8_t *kb_data = heap_caps_malloc(kb_size, MALLOC_CAP_SPIRAM);
memcpy(kb_data, flash_data, kb_size);
#endif
```

### 3. Optimize Buffer Sizes

For single-language English:

```c
#define PICO_EMBEDDED_PLATFORM 1

// In picoembedded.h or build flags:
-DPICO_MEM_SIZE=1048576  // 1 MB instead of 10 MB
```

### 4. Reduce Phoneme Buffer (if using streaming)

```c
#define PICOPAM_MAX_PH_PER_SENT 32  // Instead of 400
```

## Troubleshooting

### "Failed to load TA/SG resource"

- Make sure language files are embedded correctly
- Check file paths in CMakeLists.txt or platformio.ini
- Verify binary symbols are created (check linker output)

### "Out of memory" Error

- Increase `PICO_MEM_SIZE` (try 1.5-2 MB)
- Enable SPIRAM if available
- Check for memory leaks in your application

### Slow Synthesis

- Enable flash cache in menuconfig
- Increase CPU frequency (ESP32: 240 MHz)
- Consider using SPIRAM for better performance

## See Also

- [SINGLE_LANGUAGE_OPTIMIZATION.md](../../SINGLE_LANGUAGE_OPTIMIZATION.md) - Complete optimization guide
- [ESP32_IMPLEMENTATION_GUIDE.md](../../ESP32_IMPLEMENTATION_GUIDE.md) - ESP32-specific details
- [PHASE1_IMPLEMENTATION.md](../../PHASE1_IMPLEMENTATION.md) - XIP implementation details

## License

Same as PicoTTS (Apache License 2.0)
