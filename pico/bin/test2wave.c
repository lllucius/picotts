/* test2wave.c
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
 *   Simple example demonstrating how to use the PicoTTS library
 *   to convert text to speech with voice quality improvements.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <picoapi.h>
#include <picoapid.h>
#include <picoos.h>

/* Memory and buffer configuration */
#define PICO_MEM_SIZE       2500000
#define MAX_OUTBUF_SIZE     128

/* Voice quality filter parameters - based on analysis of synthesis output */
#define FILTER_LOWSHELF_ATTENUATION -18.0f  // Attenuate low frequencies by 18dB
#define FILTER_TRANSITION_FREQ 1100.0f      // Transition frequency at 1100Hz
#define FILTER_SHELF_SLOPE 1.0f             // Filter Q factor
#define FILTER_GAIN 5.5f                     // Overall gain to compensate
#define SAMPLE_RATE 16000.0f                 // PicoTTS sample rate

/* Language configuration */
#ifdef picolangdir
const char * PICO_LINGWARE_PATH = picolangdir "/";
#else
const char * PICO_LINGWARE_PATH = "./lang/";
#endif

const char * PICO_VOICE_NAME = "PicoVoice";

/* Filter state variables for biquad IIR filter */
typedef struct {
    double m_fa, m_fb, m_fc, m_fd, m_fe;  // Filter coefficients
    double x1, x2;                         // Input history
    double out1, out2;                     // Output history
} VoiceQualityFilter;

/**
 * Initialize the voice quality filter coefficients
 * This implements a low-shelf filter that attenuates low frequencies
 * which are often over-emphasized in TTS output, making room for
 * overall amplification without clipping.
 */
void initVoiceQualityFilter(VoiceQualityFilter *filter) {
    // Calculate filter coefficients using low-shelf filter design
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

    // Normalize and apply gain
    filter->m_fa = FILTER_GAIN * b0 / a0;
    filter->m_fb = FILTER_GAIN * b1 / a0;
    filter->m_fc = FILTER_GAIN * b2 / a0;
    filter->m_fd = a1 / a0;
    filter->m_fe = a2 / a0;

    // Initialize state
    filter->x1 = filter->x2 = 0.0;
    filter->out1 = filter->out2 = 0.0;
}

/**
 * Apply the voice quality filter to audio samples
 * Uses a biquad IIR filter to improve the tonal balance
 */
void applyVoiceQualityFilter(VoiceQualityFilter *filter, int16_t* buffer, size_t sampleCount) {
    for (size_t i = 0; i < sampleCount; i++) {
        double x0 = (double)buffer[i];
        
        // Biquad difference equation
        double out0 = (filter->m_fa * x0) + 
                     (filter->m_fb * filter->x1) + 
                     (filter->m_fc * filter->x2) + 
                     (filter->m_fd * filter->out1) + 
                     (filter->m_fe * filter->out2);

        // Update state
        filter->x2 = filter->x1;
        filter->x1 = x0;
        filter->out2 = filter->out1;
        filter->out1 = out0;

        // Clip to prevent overflow
        if (out0 > 32767.0) {
            buffer[i] = 32767;
        } else if (out0 < -32768.0) {
            buffer[i] = -32768;
        } else {
            buffer[i] = (int16_t)out0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output.wav> [text]\n", argv[0]);
        fprintf(stderr, "Example: %s output.wav \"Hello, this is a test.\"\n", argv[0]);
        return 1;
    }

    char *wavefile = argv[1];
    char *text = (argc >= 3) ? argv[2] : "Hello world. This is a test of the Pico text to speech system.";
    
    printf("PicoTTS Test2Wave Example\n");
    printf("=========================\n");
    printf("Output file: %s\n", wavefile);
    printf("Text: %s\n", text);
    printf("Voice quality filter: ENABLED\n");
    printf("  - Low-shelf attenuation: %.1f dB\n", FILTER_LOWSHELF_ATTENUATION);
    printf("  - Transition frequency: %.1f Hz\n", FILTER_TRANSITION_FREQ);
    printf("  - Overall gain: %.1f\n\n", FILTER_GAIN);

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

    /* Load text analysis resource (using en-US as example) */
    char taFileName[256];
    snprintf(taFileName, sizeof(taFileName), "%sen-US_ta.bin", PICO_LINGWARE_PATH);
    ret = pico_loadResource(picoSystem, (pico_Char *)taFileName, &picoTaResource);
    if (ret) {
        /* Try local lang directory if installed path doesn't work */
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
        /* Try local lang directory if installed path doesn't work */
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

    /* Add resources to voice */
    pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *)PICO_VOICE_NAME, taResourceName);
    pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *)PICO_VOICE_NAME, sgResourceName);

    /* Create engine */
    ret = pico_newEngine(picoSystem, (const pico_Char *)PICO_VOICE_NAME, &picoEngine);
    if (ret) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot create engine (%i): %s\n", ret, outMessage);
        goto cleanup;
    }

    /* Initialize voice quality filter */
    VoiceQualityFilter filter;
    initVoiceQualityFilter(&filter);

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

    printf("Synthesizing...\n");
    
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
                /* Apply voice quality filter to improve audio */
                applyVoiceQualityFilter(&filter, outbuf, bytes_recv / 2);
                
                if ((bufused + bytes_recv) <= sizeof(buffer)) {
                    memcpy(buffer + bufused, (int8_t *)outbuf, bytes_recv);
                    bufused += bytes_recv;
                } else {
                    /* Write accumulated samples */
                    picoos_sdfPutSamples(sdOutFile, bufused / 2, (picoos_int16 *)buffer);
                    bufused = 0;
                    memcpy(buffer, (int8_t *)outbuf, bytes_recv);
                    bufused += bytes_recv;
                }
            }
        } while (ret == PICO_STEP_BUSY);

        /* Write remaining samples for this chunk */
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
