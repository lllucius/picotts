/* test2wave_embedded.c
 *
 * Copyright (C) 2024 PicoTTS Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *   Embedded-optimized example demonstrating PicoTTS with fixed-point
 *   voice quality filtering for resource-constrained devices.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <picoapi.h>
#include <picoapid.h>
#include <picoos.h>

/* Memory and buffer configuration for embedded systems */
#ifdef EMBEDDED_TINY
#define PICO_MEM_SIZE       1000000
#define MAX_OUTBUF_SIZE     64
#elif defined(EMBEDDED_SMALL)
#define PICO_MEM_SIZE       1500000
#define MAX_OUTBUF_SIZE     96
#else
#define PICO_MEM_SIZE       2500000
#define MAX_OUTBUF_SIZE     128
#endif

/* Voice quality filter parameters */
#define FILTER_LOWSHELF_ATTENUATION -18.0f
#define FILTER_TRANSITION_FREQ 1100.0f
#define FILTER_SHELF_SLOPE 1.0f
#define FILTER_GAIN 5.5f
#define SAMPLE_RATE 16000.0f

/* Fixed-point configuration (Q15 format) */
#define FIXEDPOINT_FRACBITS 15
#define FIXEDPOINT_SCALE (1 << FIXEDPOINT_FRACBITS)

/* Language configuration */
#ifdef picolangdir
const char * PICO_LINGWARE_PATH = picolangdir "/";
#else
const char * PICO_LINGWARE_PATH = "./lang/";
#endif

const char * PICO_VOICE_NAME = "PicoVoice";

/* Fixed-point filter for embedded systems (no FPU required) */
typedef struct {
    int32_t m_fa, m_fb, m_fc, m_fd, m_fe;  // Q15 coefficients
    int32_t x1, x2;                         // Q15 input history
    int64_t out1, out2;                     // Q30 output history
} VoiceQualityFilterFixed;

/**
 * Initialize fixed-point voice quality filter
 * Coefficients are calculated once using floating-point math,
 * then converted to fixed-point for runtime efficiency.
 */
void initVoiceQualityFilterFixed(VoiceQualityFilterFixed *filter) {
    // Calculate coefficients using floating-point
    double amp = pow(10.0, FILTER_LOWSHELF_ATTENUATION / 40.0);
    double w = 2.0 * M_PI * (FILTER_TRANSITION_FREQ / SAMPLE_RATE);
    double sinw = sin(w);
    double cosw = cos(w);
    double beta = sqrt(amp) / FILTER_SHELF_SLOPE;

    // Low-shelf biquad filter coefficients
    double b0 = amp * ((amp + 1.0) - ((amp - 1.0) * cosw) + (beta * sinw));
    double b1 = 2.0 * amp * ((amp - 1.0) - ((amp + 1.0) * cosw));
    double b2 = amp * ((amp + 1.0) - ((amp - 1.0) * cosw) - (beta * sinw));
    double a0 = (amp + 1.0) + ((amp - 1.0) * cosw) + (beta * sinw);
    double a1 = 2.0 * ((amp - 1.0) + ((amp + 1.0) * cosw));
    double a2 = -((amp + 1.0) + ((amp - 1.0) * cosw) - (beta * sinw));

    // Convert to fixed-point Q15 format
    filter->m_fa = (int32_t)((FILTER_GAIN * b0 / a0) * FIXEDPOINT_SCALE);
    filter->m_fb = (int32_t)((FILTER_GAIN * b1 / a0) * FIXEDPOINT_SCALE);
    filter->m_fc = (int32_t)((FILTER_GAIN * b2 / a0) * FIXEDPOINT_SCALE);
    filter->m_fd = (int32_t)((a1 / a0) * FIXEDPOINT_SCALE);
    filter->m_fe = (int32_t)((a2 / a0) * FIXEDPOINT_SCALE);

    // Initialize state
    filter->x1 = filter->x2 = 0;
    filter->out1 = filter->out2 = 0;
    
    printf("Fixed-point filter initialized:\n");
    printf("  Coefficients (Q15): fa=%d, fb=%d, fc=%d, fd=%d, fe=%d\n",
           filter->m_fa, filter->m_fb, filter->m_fc, filter->m_fd, filter->m_fe);
}

/**
 * Apply fixed-point voice quality filter
 * Uses only integer arithmetic - no FPU required
 * Optimized for embedded processors
 */
void applyVoiceQualityFilterFixed(VoiceQualityFilterFixed *filter, int16_t* buffer, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; i++) {
        // Convert input to Q15 format
        int32_t x0 = (int32_t)buffer[i] << FIXEDPOINT_FRACBITS;
        
        // Biquad difference equation using fixed-point arithmetic
        // Q15 * Q15 = Q30, then scaled back to Q15 for feedback terms
        int64_t out0 = ((int64_t)filter->m_fa * x0) +
                      ((int64_t)filter->m_fb * filter->x1) +
                      ((int64_t)filter->m_fc * filter->x2) +
                      ((int64_t)filter->m_fd * (int32_t)(filter->out1 >> FIXEDPOINT_FRACBITS)) +
                      ((int64_t)filter->m_fe * (int32_t)(filter->out2 >> FIXEDPOINT_FRACBITS));

        // Update state
        filter->x2 = filter->x1;
        filter->x1 = x0;
        filter->out2 = filter->out1;
        filter->out1 = out0;

        // Convert back to int16 with saturation
        int32_t result = (int32_t)(out0 >> (2 * FIXEDPOINT_FRACBITS));
        if (result > 32767) {
            result = 32767;
        } else if (result < -32768) {
            result = -32768;
        }
        buffer[i] = (int16_t)result;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output.wav> [text]\n", argv[0]);
        fprintf(stderr, "Embedded-optimized version with fixed-point filter\n");
        return 1;
    }

    char *wavefile = argv[1];
    char *text = (argc >= 3) ? argv[2] : "Hello world. This is an embedded optimized test.";
    
    printf("PicoTTS Embedded Test2Wave Example\n");
    printf("===================================\n");
    printf("Output file: %s\n", wavefile);
    printf("Text: %s\n", text);
    printf("Memory budget: %d bytes\n", PICO_MEM_SIZE);
    printf("Voice quality filter: FIXED-POINT (embedded optimized)\n");
    printf("  - No FPU required\n");
    printf("  - Q15 fixed-point arithmetic\n");
    printf("  - Low-shelf attenuation: %.1f dB\n", FILTER_LOWSHELF_ATTENUATION);
    printf("  - Transition frequency: %.1f Hz\n\n", FILTER_TRANSITION_FREQ);

    /* Allocate memory for Pico */
    void *picoMemArea = malloc(PICO_MEM_SIZE);
    if (!picoMemArea) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }

    pico_System picoSystem = NULL;
    pico_Resource picoTaResource = NULL;
    pico_Resource picoSgResource = NULL;
    pico_Engine picoEngine = NULL;
    pico_Retstring outMessage;
    int ret;

    /* Initialize Pico system */
    ret = pico_initialize(picoMemArea, PICO_MEM_SIZE, &picoSystem);
    if (ret) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot initialize pico (%i): %s\n", ret, outMessage);
        free(picoMemArea);
        return 1;
    }

    /* Load text analysis resource */
    char taFileName[256];
    snprintf(taFileName, sizeof(taFileName), "%sen-US_ta.bin", PICO_LINGWARE_PATH);
    ret = pico_loadResource(picoSystem, (pico_Char *)taFileName, &picoTaResource);
    if (ret) {
        snprintf(taFileName, sizeof(taFileName), "./lang/en-US_ta.bin");
        ret = pico_loadResource(picoSystem, (pico_Char *)taFileName, &picoTaResource);
        if (ret) {
            pico_getSystemStatusMessage(picoSystem, ret, outMessage);
            fprintf(stderr, "Cannot load text analysis resource (%i): %s\n", ret, outMessage);
            pico_terminate(&picoSystem);
            free(picoMemArea);
            return 1;
        }
    }

    /* Load signal generation resource */
    char sgFileName[256];
    snprintf(sgFileName, sizeof(sgFileName), "%sen-US_lh0_sg.bin", PICO_LINGWARE_PATH);
    ret = pico_loadResource(picoSystem, (pico_Char *)sgFileName, &picoSgResource);
    if (ret) {
        snprintf(sgFileName, sizeof(sgFileName), "./lang/en-US_lh0_sg.bin");
        ret = pico_loadResource(picoSystem, (pico_Char *)sgFileName, &picoSgResource);
        if (ret) {
            pico_getSystemStatusMessage(picoSystem, ret, outMessage);
            fprintf(stderr, "Cannot load signal generation resource (%i): %s\n", ret, outMessage);
            pico_unloadResource(picoSystem, &picoTaResource);
            pico_terminate(&picoSystem);
            free(picoMemArea);
            return 1;
        }
    }

    /* Get resource names */
    pico_Char *taResourceName = (pico_Char *)malloc(PICO_MAX_RESOURCE_NAME_SIZE);
    pico_Char *sgResourceName = (pico_Char *)malloc(PICO_MAX_RESOURCE_NAME_SIZE);
    if (!taResourceName || !sgResourceName) {
        fprintf(stderr, "Failed to allocate memory for resource names\n");
        goto cleanup;
    }
    pico_getResourceName(picoSystem, picoTaResource, (char *)taResourceName);
    pico_getResourceName(picoSystem, picoSgResource, (char *)sgResourceName);

    /* Create voice definition */
    ret = pico_createVoiceDefinition(picoSystem, (const pico_Char *)PICO_VOICE_NAME);
    if (ret) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot create voice definition (%i): %s\n", ret, outMessage);
        goto cleanup;
    }

    pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *)PICO_VOICE_NAME, taResourceName);
    pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *)PICO_VOICE_NAME, sgResourceName);

    /* Create engine */
    ret = pico_newEngine(picoSystem, (const pico_Char *)PICO_VOICE_NAME, &picoEngine);
    if (ret) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot create engine (%i): %s\n", ret, outMessage);
        goto cleanup;
    }

    /* Initialize fixed-point voice quality filter */
    VoiceQualityFilterFixed filter;
    initVoiceQualityFilterFixed(&filter);

    /* Open output WAV file */
    picoos_Common common = (picoos_Common)pico_sysGetCommon(picoSystem);
    picoos_SDFile sdOutFile = NULL;
    if (!picoos_sdfOpenOut(common, &sdOutFile, (picoos_char *)wavefile, 
                          SAMPLE_FREQ_16KHZ, PICOOS_ENC_LIN)) {
        fprintf(stderr, "Cannot open output wave file\n");
        goto cleanup;
    }

    /* Synthesis loop */
    pico_Char *inp = (pico_Char *)text;
    pico_Int16 text_remaining = strlen(text) + 1;
    short outbuf[MAX_OUTBUF_SIZE / 2];
    int8_t buffer[256];
    size_t bufused = 0;

    printf("Synthesizing with fixed-point filter...\n");
    
    while (text_remaining) {
        pico_Int16 bytes_sent, bytes_recv, out_data_type;
        
        /* Feed text to engine */
        ret = pico_putTextUtf8(picoEngine, inp, text_remaining, &bytes_sent);
        if (ret) {
            pico_getSystemStatusMessage(picoSystem, ret, outMessage);
            fprintf(stderr, "Cannot put text (%i): %s\n", ret, outMessage);
            break;
        }

        text_remaining -= bytes_sent;
        inp += bytes_sent;

        /* Retrieve audio data */
        do {
            ret = pico_getData(picoEngine, (void *)outbuf, MAX_OUTBUF_SIZE, 
                             &bytes_recv, &out_data_type);
            
            if (ret != PICO_STEP_BUSY && ret != PICO_STEP_IDLE) {
                pico_getSystemStatusMessage(picoSystem, ret, outMessage);
                fprintf(stderr, "Cannot get data (%i): %s\n", ret, outMessage);
                break;
            }

            if (bytes_recv) {
                /* Apply fixed-point voice quality filter */
                applyVoiceQualityFilterFixed(&filter, outbuf, bytes_recv / 2);
                
                if ((bufused + bytes_recv) <= sizeof(buffer)) {
                    memcpy(buffer + bufused, (int8_t *)outbuf, bytes_recv);
                    bufused += bytes_recv;
                } else {
                    picoos_sdfPutSamples(sdOutFile, bufused / 2, (picoos_int16 *)buffer);
                    bufused = 0;
                    memcpy(buffer, (int8_t *)outbuf, bytes_recv);
                    bufused += bytes_recv;
                }
            }
        } while (ret == PICO_STEP_BUSY);

        if (bufused > 0) {
            picoos_sdfPutSamples(sdOutFile, bufused / 2, (picoos_int16 *)buffer);
            bufused = 0;
        }
    }

    printf("Synthesis complete!\n");
    picoos_sdfCloseOut(common, &sdOutFile);

cleanup:
    if (picoEngine) {
        pico_disposeEngine(picoSystem, &picoEngine);
        pico_releaseVoiceDefinition(picoSystem, (pico_Char *)PICO_VOICE_NAME);
    }
    if (sgResourceName) free((void *)sgResourceName);
    if (taResourceName) free((void *)taResourceName);
    if (picoSgResource) {
        pico_unloadResource(picoSystem, &picoSgResource);
    }
    if (picoTaResource) {
        pico_unloadResource(picoSystem, &picoTaResource);
    }
    if (picoSystem) {
        pico_terminate(&picoSystem);
    }
    free(picoMemArea);

    return 0;
}
