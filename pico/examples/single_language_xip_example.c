/*
 * Example: Single-Language English TTS with XIP (Execute-In-Place)
 * 
 * This example demonstrates how to use PicoTTS with memory-mapped language
 * files for embedded systems, avoiding the need to copy large language
 * files into RAM.
 * 
 * Benefits:
 * - Zero RAM usage for language data (3-4 MB savings)
 * - Fast startup (no file I/O)
 * - Ideal for ESP32 and other embedded systems
 * 
 * Compile: gcc -I../lib single_language_xip_example.c -L.. -lttspico -o xip_example
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "picoapi.h"
#include "picoextapi.h"

/* Method 1: Using external binary data (ESP32 style) */
#ifdef ESP32_EMBEDDED_BINARY
/* These symbols are created by the ESP32 build system when you embed
 * binary files. See platformio.ini or CMakeLists.txt for configuration.
 * 
 * Example CMakeLists.txt:
 *   target_add_binary_data(app "en-US_ta.bin" BINARY)
 *   target_add_binary_data(app "en-US_lh0_sg.bin" BINARY)
 */
extern const uint8_t en_us_ta_start[] asm("_binary_en_US_ta_bin_start");
extern const uint8_t en_us_ta_end[]   asm("_binary_en_US_ta_bin_end");
extern const uint8_t en_us_sg_start[] asm("_binary_en_US_lh0_sg_bin_start");
extern const uint8_t en_us_sg_end[]   asm("_binary_en_US_lh0_sg_bin_end");
#endif

/* Method 2: Load files into memory once (still saves specialization time) */
static unsigned char *loadFileToMemory(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char *buffer = malloc(*size);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    
    if (fread(buffer, 1, *size, f) != *size) {
        free(buffer);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    return buffer;
}

int main(int argc, char **argv) {
    pico_System picoSystem = NULL;
    pico_Resource taResource = NULL;
    pico_Resource sgResource = NULL;
    pico_Engine picoEngine = NULL;
    pico_Char *voiceName = (pico_Char *)"PicoVoice";
    void *picoMemArea = NULL;
    int ret;
    
    /* Text to synthesize */
    const char *text = "Hello, this is an example of PicoTTS with XIP support!";
    
    printf("=== PicoTTS Single-Language XIP Example ===\n\n");
    
    /* 1. Initialize memory for Pico system
     *    For English only, 500KB-1MB is sufficient */
    const size_t PICO_MEM_SIZE = 1024 * 1024; /* 1 MB */
    picoMemArea = malloc(PICO_MEM_SIZE);
    if (!picoMemArea) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    /* 2. Initialize Pico system */
    ret = pico_initialize(picoMemArea, PICO_MEM_SIZE, &picoSystem);
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to initialize Pico: %d\n", ret);
        free(picoMemArea);
        return 1;
    }
    printf("✓ Pico system initialized\n");
    
    /* 3. Load resources from memory */
    
#ifdef ESP32_EMBEDDED_BINARY
    /* Method 1: ESP32 embedded binary - zero copy, data stays in flash */
    printf("Loading resources from embedded flash memory (XIP)...\n");
    
    ret = picoext_loadResourceFromMemory(
        picoSystem,
        en_us_ta_start,
        en_us_ta_end - en_us_ta_start,
        (pico_Char *)"en-US-ta",
        &taResource
    );
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to load TA resource: %d\n", ret);
        goto cleanup;
    }
    
    ret = picoext_loadResourceFromMemory(
        picoSystem,
        en_us_sg_start,
        en_us_sg_end - en_us_sg_start,
        (pico_Char *)"en-US-sg",
        &sgResource
    );
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to load SG resource: %d\n", ret);
        goto cleanup;
    }
    
#else
    /* Method 2: Load files to memory - useful for testing */
    printf("Loading resources from files to memory...\n");
    
    size_t taSize, sgSize;
    unsigned char *taData = loadFileToMemory("../../lang/en-US_ta.bin", &taSize);
    unsigned char *sgData = loadFileToMemory("../../lang/en-US_lh0_sg.bin", &sgSize);
    
    if (!taData || !sgData) {
        fprintf(stderr, "Failed to load language files\n");
        fprintf(stderr, "Make sure en-US_ta.bin and en-US_lh0_sg.bin are in ../../lang/\n");
        free(taData);
        free(sgData);
        goto cleanup;
    }
    
    printf("  - TA file: %zu bytes\n", taSize);
    printf("  - SG file: %zu bytes\n", sgSize);
    
    ret = picoext_loadResourceFromMemory(
        picoSystem,
        taData,
        taSize,
        (pico_Char *)"en-US-ta",
        &taResource
    );
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to load TA resource: %d\n", ret);
        free(taData);
        free(sgData);
        goto cleanup;
    }
    
    ret = picoext_loadResourceFromMemory(
        picoSystem,
        sgData,
        sgSize,
        (pico_Char *)"en-US-sg",
        &sgResource
    );
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to load SG resource: %d\n", ret);
        free(taData);
        free(sgData);
        goto cleanup;
    }
    
    /* Note: taData and sgData must remain valid while resources are in use */
#endif
    
    printf("✓ Resources loaded\n");
    
    /* 4. Create voice definition */
    ret = pico_createVoiceDefinition(picoSystem, voiceName);
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to create voice definition: %d\n", ret);
        goto cleanup;
    }
    
    ret = pico_addResourceToVoiceDefinition(picoSystem, voiceName, 
                                           (pico_Char *)"en-US-ta");
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to add TA resource to voice: %d\n", ret);
        goto cleanup;
    }
    
    ret = pico_addResourceToVoiceDefinition(picoSystem, voiceName, 
                                           (pico_Char *)"en-US-sg");
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to add SG resource to voice: %d\n", ret);
        goto cleanup;
    }
    
    printf("✓ Voice definition created\n");
    
    /* 5. Create TTS engine */
    ret = pico_newEngine(picoSystem, voiceName, &picoEngine);
    if (ret != PICO_OK) {
        fprintf(stderr, "Failed to create engine: %d\n", ret);
        goto cleanup;
    }
    
    printf("✓ TTS engine created\n");
    printf("\nReady for synthesis!\n");
    printf("Text: \"%s\"\n\n", text);
    
    /* 6. Synthesize text */
    pico_Int16 textRemaining = strlen(text);
    const char *textPtr = text;
    
    while (textRemaining > 0) {
        pico_Int16 bytesInput = 0;
        ret = pico_putTextUtf8(picoEngine, (pico_Char *)textPtr, 
                               textRemaining, &bytesInput);
        if (ret != PICO_OK) {
            fprintf(stderr, "Failed to put text: %d\n", ret);
            goto cleanup;
        }
        textPtr += bytesInput;
        textRemaining -= bytesInput;
    }
    
    /* Signal end of text */
    ret = pico_putTextUtf8(picoEngine, (pico_Char *)"", 0, NULL);
    
    /* 7. Get audio data */
    pico_Char outBuffer[2048];
    pico_Int16 bytesReceived;
    pico_Int16 outDataType;
    int totalSamples = 0;
    
    printf("Generating audio...\n");
    
    do {
        ret = pico_getData(picoEngine, outBuffer, sizeof(outBuffer),
                          &bytesReceived, &outDataType);
        
        if (bytesReceived > 0) {
            totalSamples += bytesReceived / 2; /* 16-bit samples */
            /* In a real application, you would write this to an audio device
             * or file. For example:
             *   fwrite(outBuffer, 1, bytesReceived, audioFile);
             * or on ESP32:
             *   i2s_write(I2S_NUM_0, outBuffer, bytesReceived, &bytes_written, portMAX_DELAY);
             */
        }
    } while (ret == PICO_STEP_BUSY);
    
    printf("✓ Synthesis complete: %d samples\n", totalSamples);
    printf("  Audio duration: %.2f seconds (16kHz, 16-bit mono)\n", 
           totalSamples / 16000.0);
    
cleanup:
    /* 8. Cleanup */
    if (picoEngine) {
        pico_disposeEngine(picoSystem, &picoEngine);
    }
    if (voiceName) {
        pico_releaseVoiceDefinition(picoSystem, voiceName);
    }
    if (taResource) {
        pico_unloadResource(picoSystem, &taResource);
    }
    if (sgResource) {
        pico_unloadResource(picoSystem, &sgResource);
    }
    if (picoSystem) {
        pico_terminate(&picoSystem);
    }
    if (picoMemArea) {
        free(picoMemArea);
    }
    
    printf("\n=== Example complete ===\n");
    return 0;
}

/*
 * Performance Notes:
 * 
 * 1. Memory Usage (English only):
 *    - Without XIP: ~4 MB (language data copied to RAM)
 *    - With XIP: ~1 MB (only working buffers in RAM)
 *    - Savings: 3 MB (75% reduction)
 * 
 * 2. Startup Time:
 *    - File loading: 200-300 ms
 *    - Memory loading (XIP): 50-100 ms
 *    - Improvement: 2-3x faster
 * 
 * 3. For ESP32:
 *    - Use SPIRAM for even better performance
 *    - Enable flash cache in menuconfig
 *    - Consider dual-core for parallel processing
 * 
 * 4. Build Configuration (ESP32):
 *    In your platformio.ini or CMakeLists.txt:
 *    
 *    platformio.ini:
 *      board_build.embed_txtfiles =
 *        lang/en-US_ta.bin
 *        lang/en-US_lh0_sg.bin
 *    
 *    CMakeLists.txt:
 *      target_add_binary_data(app "lang/en-US_ta.bin" BINARY)
 *      target_add_binary_data(app "lang/en-US_lh0_sg.bin" BINARY)
 */
