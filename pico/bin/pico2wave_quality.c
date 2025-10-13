/* pico2wave_quality.c
 *
 * Enhanced version of pico2wave with Phase 3 quality improvements
 * for testing on Linux before ESP32 deployment
 *
 * Copyright (C) 2009 Mathieu Parent <math.parent@gmail.com>
 * Copyright (C) 2024 PicoTTS Quality Enhancement Contributors
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
 *   Convert text to .wav using svox text-to-speech with quality enhancements
 *
 */


#include <popt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <picoapi.h>
#include <picoapid.h>
#include <picoos.h>

/* Phase 3: Quality Enhancement */
#if PICO_USE_QUALITY_ENHANCE
#include <picoqualityenhance.h>
#endif

/* adaptation layer defines */
#define PICO_MEM_SIZE       2500000
#define DummyLen 100000000

/* string constants */
#define MAX_OUTBUF_SIZE     128
#ifdef picolangdir
const char * PICO_LINGWARE_PATH             = picolangdir "/";
#else
const char * PICO_LINGWARE_PATH             = "./lang/";
#endif
const char * PICO_VOICE_NAME                = "PicoVoice";

/* supported voices */
const char * picoSupportedLangIso3[]        = { "eng",              "eng",              "deu",              "spa",              "fra",              "ita" };
const char * picoSupportedCountryIso3[]     = { "USA",              "GBR",              "DEU",              "ESP",              "FRA",              "ITA" };
const char * picoSupportedLang[]            = { "en-US",            "en-GB",            "de-DE",            "es-ES",            "fr-FR",            "it-IT" };
const char * picoInternalLang[]             = { "en-US",            "en-GB",            "de-DE",            "es-ES",            "fr-FR",            "it-IT" };
const char * picoInternalTaLingware[]       = { "en-US_ta.bin",     "en-GB_ta.bin",     "de-DE_ta.bin",     "es-ES_ta.bin",     "fr-FR_ta.bin",     "it-IT_ta.bin" };
const char * picoInternalSgLingware[]       = { "en-US_lh0_sg.bin", "en-GB_kh0_sg.bin", "de-DE_gl0_sg.bin", "es-ES_zl0_sg.bin", "fr-FR_nk0_sg.bin", "it-IT_cm0_sg.bin" };
const char * picoInternalUtppLingware[]     = { "en-US_utpp.bin",   "en-GB_utpp.bin",   "de-DE_utpp.bin",   "es-ES_utpp.bin",   "fr-FR_utpp.bin",   "it-IT_utpp.bin" };
const int picoNumSupportedVocs              = 6;

/* adapation layer global variables */
void *          picoMemArea         = NULL;
pico_System     picoSystem          = NULL;
pico_Resource   picoTaResource      = NULL;
pico_Resource   picoSgResource      = NULL;
pico_Resource   picoUtppResource    = NULL;
pico_Engine     picoEngine          = NULL;
pico_Char *     picoTaFileName      = NULL;
pico_Char *     picoSgFileName      = NULL;
pico_Char *     picoUtppFileName    = NULL;
pico_Char *     picoTaResourceName  = NULL;
pico_Char *     picoSgResourceName  = NULL;
pico_Char *     picoUtppResourceName = NULL;
int     picoSynthAbort = 0;

#define CHUNK_SIZE 16384UL
/* buffered read from source until EOF; user should free the buffer */
size_t myread(FILE *source, char **buffer) {
    size_t count;
    size_t size = 0;
    size_t offset = 0;
    *buffer = NULL;
    do {
        *buffer = (char *)realloc(*buffer, size + CHUNK_SIZE);
        assert(*buffer);
        size += CHUNK_SIZE;
        count = fread(&((*buffer)[offset]), 1, CHUNK_SIZE, source);
        assert(!ferror(source));
        if (feof(source)) {
            (*buffer)[offset + count] = 0;
            return offset + count;
        }
        offset += CHUNK_SIZE;
    } while (1);
}

void print_usage_examples() {
    printf("\nUsage Examples:\n");
    printf("  Basic usage:\n");
    printf("    pico2wave_quality -w output.wav \"Hello, world!\"\n\n");
    
    printf("  With language selection:\n");
    printf("    pico2wave_quality -w output.wav -l en-GB \"Hello from Britain\"\n\n");
    
#if PICO_USE_QUALITY_ENHANCE
    printf("  With voice customization:\n");
    printf("    pico2wave_quality -w output.wav --voice female \"Hello, I'm a female voice\"\n");
    printf("    pico2wave_quality -w output.wav --voice male \"Hello, I'm a male voice\"\n");
    printf("    pico2wave_quality -w output.wav --voice child \"Hi! Let's play!\"\n\n");
    
    printf("  With quality mode:\n");
    printf("    pico2wave_quality -w output.wav --quality speed \"Quick notification\"\n");
    printf("    pico2wave_quality -w output.wav --quality high \"High quality speech\"\n\n");
    
    printf("  With custom parameters:\n");
    printf("    pico2wave_quality -w output.wav --pitch 1.2 --speed 0.9 \"Custom voice\"\n\n");
    
    printf("  Reading from stdin:\n");
    printf("    echo \"Hello from stdin\" | pico2wave_quality -w output.wav\n\n");
#endif
    
    printf("Voice Profiles:\n");
    printf("  default, male, female, child, robot, slow, fast\n\n");
    
    printf("Quality Modes:\n");
    printf("  speed (fast), balanced (default), high (best quality)\n\n");
}

int main(int argc, const char *argv[]) {
    char * wavefile = NULL;
    char * lang = "en-US";
    int langIndex = -1, langIndexTmp = -1;
    char * text = NULL;
    int8_t * buffer;
    size_t bufferSize = 256;

#if PICO_USE_QUALITY_ENHANCE
    /* Quality enhancement options */
    char * voice_profile = NULL;
    char * quality_mode = NULL;
    float pitch_scale = 1.0f;
    float speed_scale = 1.0f;
    float formant_shift = 0.0f;
    int show_stats = 0;
#endif

    /* Parsing options */
    poptContext optCon;
    int opt;

    struct poptOption optionsTable[] = {
        { "wave", 'w', POPT_ARG_STRING, &wavefile, 0,
          "Write output to this WAV file (extension SHOULD be .wav)", "filename.wav" },
        { "lang", 'l', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &lang, 0,
          "Language (en-US, en-GB, de-DE, es-ES, fr-FR, it-IT)", "lang" },
#if PICO_USE_QUALITY_ENHANCE
        { "voice", 'v', POPT_ARG_STRING, &voice_profile, 0,
          "Voice profile (default, male, female, child, robot, slow, fast)", "profile" },
        { "quality", 'q', POPT_ARG_STRING, &quality_mode, 0,
          "Quality mode (speed, balanced, high)", "mode" },
        { "pitch", 'p', POPT_ARG_FLOAT, &pitch_scale, 0,
          "Pitch scaling (0.5-2.0, default 1.0)", "scale" },
        { "speed", 's', POPT_ARG_FLOAT, &speed_scale, 0,
          "Speed scaling (0.5-3.0, default 1.0)", "scale" },
        { "formant", 'f', POPT_ARG_FLOAT, &formant_shift, 0,
          "Formant shift in Hz (-500 to +500, default 0)", "shift" },
        { "stats", 'S', POPT_ARG_NONE, &show_stats, 0,
          "Show quality enhancement statistics", NULL },
#endif
        POPT_AUTOHELP
        POPT_TABLEEND
    };
    
    optCon = poptGetContext(NULL, argc, argv, optionsTable, POPT_CONTEXT_POSIXMEHARDER);
    poptSetOtherOptionHelp(optCon, "<words>");

    /* Reporting about invalid extra options */
    while ((opt = poptGetNextOpt(optCon)) != -1) {
        switch (opt) {
        default:
            fprintf(stderr, "Invalid option %s: %s\n",
                poptBadOption(optCon, 0), poptStrerror(opt));
            poptPrintHelp(optCon, stderr, 0);
            print_usage_examples();
            exit(1);
        }
    }

    /* Mandatory option: --wave */
    if(!wavefile) {
        fprintf(stderr, "Mandatory option: %s\n\n",
            "--wave=filename.wav");
        poptPrintHelp(optCon, stderr, 0);
        print_usage_examples();
        exit(1);
    }
    
    /* option: --lang */
    for(langIndexTmp = 0; langIndexTmp < picoNumSupportedVocs; langIndexTmp++) {
        if(!strcmp(picoSupportedLang[langIndexTmp], lang)) {
            langIndex = langIndexTmp;
            break;
        }
    }
    if(langIndex == -1) {
        fprintf(stderr, "Unknown language: %s\nValid languages:\n", lang);
        for(langIndexTmp = 0; langIndexTmp < picoNumSupportedVocs; langIndexTmp++) {
            fprintf(stderr, "%s\n", picoSupportedLang[langIndexTmp]);
        }
        lang = "en-US";
        fprintf(stderr, "\n");
        poptPrintHelp(optCon, stderr, 0);
        exit(1);
    }

    /* Remaining argument is <words> */
    const char **extra_argv;
    extra_argv = poptGetArgs(optCon);
    if(extra_argv) {
        text = (char *) &(*extra_argv)[0];
    } else {
        /* Read from stdin */
        size_t len = myread(stdin, &text);
        if (len == 0 || text == NULL) {
            fprintf(stderr, "Error: No text provided (either as argument or stdin)\n");
            poptPrintHelp(optCon, stderr, 0);
            print_usage_examples();
            exit(1);
        }
    }

    poptFreeContext(optCon);

#if PICO_USE_QUALITY_ENHANCE
    /* Initialize quality enhancements */
    printf("Initializing PicoTTS with quality enhancements...\n");
    if (pico_quality_init() != PICO_OK) {
        fprintf(stderr, "Warning: Failed to initialize quality enhancements\n");
    } else {
        printf("Quality enhancements enabled\n");
        
        /* Apply voice profile */
        if (voice_profile) {
            printf("Applying voice profile: %s\n", voice_profile);
            if (!strcmp(voice_profile, "male")) {
                pico_apply_voice_profile(PICO_VOICE_PROFILE_MALE);
            } else if (!strcmp(voice_profile, "female")) {
                pico_apply_voice_profile(PICO_VOICE_PROFILE_FEMALE);
            } else if (!strcmp(voice_profile, "child")) {
                pico_apply_voice_profile(PICO_VOICE_PROFILE_CHILD);
            } else if (!strcmp(voice_profile, "robot")) {
                pico_apply_voice_profile(PICO_VOICE_PROFILE_ROBOT);
            } else if (!strcmp(voice_profile, "slow")) {
                pico_apply_voice_profile(PICO_VOICE_PROFILE_SLOW);
            } else if (!strcmp(voice_profile, "fast")) {
                pico_apply_voice_profile(PICO_VOICE_PROFILE_FAST);
            } else if (!strcmp(voice_profile, "default")) {
                pico_apply_voice_profile(PICO_VOICE_PROFILE_DEFAULT);
            } else {
                fprintf(stderr, "Warning: Unknown voice profile '%s', using default\n", voice_profile);
            }
        }
        
        /* Apply quality mode */
        if (quality_mode) {
            printf("Setting quality mode: %s\n", quality_mode);
            if (!strcmp(quality_mode, "speed")) {
                pico_set_quality_mode(PICO_QUALITY_MODE_SPEED);
            } else if (!strcmp(quality_mode, "balanced")) {
                pico_set_quality_mode(PICO_QUALITY_MODE_BALANCED);
            } else if (!strcmp(quality_mode, "high")) {
                pico_set_quality_mode(PICO_QUALITY_MODE_QUALITY);
            } else {
                fprintf(stderr, "Warning: Unknown quality mode '%s', using balanced\n", quality_mode);
            }
        }
        
        /* Apply custom parameters */
        if (pitch_scale != 1.0f || speed_scale != 1.0f || formant_shift != 0.0f) {
            pico_voice_params_t params;
            pico_get_voice_params(&params);
            params.pitch_scale = pitch_scale;
            params.speed_scale = speed_scale;
            params.formant_shift = formant_shift;
            
            if (pico_set_voice_params(&params) == PICO_OK) {
                printf("Custom parameters: pitch=%.2f, speed=%.2f, formant=%.0fHz\n",
                       pitch_scale, speed_scale, formant_shift);
            } else {
                fprintf(stderr, "Warning: Invalid custom parameters\n");
            }
        }
    }
#else
    printf("Initializing PicoTTS (quality enhancements not compiled in)...\n");
    printf("To enable quality enhancements, rebuild with -DPICO_USE_QUALITY_ENHANCE=1\n");
#endif

    buffer = malloc(bufferSize);

    int ret, getstatus;
    pico_Char * inp = NULL;
    pico_Char * local_text = NULL;
    short       outbuf[MAX_OUTBUF_SIZE/2];
    pico_Int16  bytes_sent, bytes_recv, text_remaining, out_data_type;
    pico_Retstring outMessage;

    picoSynthAbort = 0;

    picoMemArea = malloc(PICO_MEM_SIZE);
    if((ret = pico_initialize(picoMemArea, PICO_MEM_SIZE, &picoSystem))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot initialize pico (%i): %s\n", ret, outMessage);
        goto terminate;
    }

    /* Load the text analysis Lingware resource file */
    picoTaFileName = (pico_Char *) malloc(PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE);
    strcpy((char *) picoTaFileName, PICO_LINGWARE_PATH);
    strcat((char *) picoTaFileName, (const char *) picoInternalTaLingware[langIndex]);
    if((ret = pico_loadResource(picoSystem, picoTaFileName, &picoTaResource))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot load text analysis resource file (%i): %s\n", ret, outMessage);
        goto unloadTaResource;
    }

    /* Load the signal generation Lingware resource file */
    picoSgFileName = (pico_Char *) malloc(PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE);
    strcpy((char *) picoSgFileName, PICO_LINGWARE_PATH);
    strcat((char *) picoSgFileName, (const char *) picoInternalSgLingware[langIndex]);
    if((ret = pico_loadResource(picoSystem, picoSgFileName, &picoSgResource))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot load signal generation Lingware resource file (%i): %s\n", ret, outMessage);
        goto unloadSgResource;
    }

    /* Get the text analysis resource name */
    picoTaResourceName = (pico_Char *) malloc(PICO_MAX_RESOURCE_NAME_SIZE);
    if((ret = pico_getResourceName(picoSystem, picoTaResource, (char *) picoTaResourceName))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot get the text analysis resource name (%i): %s\n", ret, outMessage);
        goto unloadUtppResource;
    }

    /* Get the signal generation resource name */
    picoSgResourceName = (pico_Char *) malloc(PICO_MAX_RESOURCE_NAME_SIZE);
    if((ret = pico_getResourceName(picoSystem, picoSgResource, (char *) picoSgResourceName))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot get the signal generation resource name (%i): %s\n", ret, outMessage);
        goto unloadUtppResource;
    }

    /* Create a voice definition */
    if((ret = pico_createVoiceDefinition(picoSystem, (const pico_Char *) PICO_VOICE_NAME))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot create voice definition (%i): %s\n", ret, outMessage);
        goto unloadUtppResource;
    }

    /* Add the text analysis resource to the voice */
    if((ret = pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *) PICO_VOICE_NAME, picoTaResourceName))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot add the text analysis resource to the voice (%i): %s\n", ret, outMessage);
        goto unloadUtppResource;
    }

    /* Add the signal generation resource to the voice */
    if((ret = pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *) PICO_VOICE_NAME, picoSgResourceName))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot add the signal generation resource to the voice (%i): %s\n", ret, outMessage);
        goto unloadUtppResource;
    }

    /* Create a new Pico engine */
    if((ret = pico_newEngine(picoSystem, (const pico_Char *) PICO_VOICE_NAME, &picoEngine))) {
        pico_getSystemStatusMessage(picoSystem, ret, outMessage);
        fprintf(stderr, "Cannot create a new pico engine (%i): %s\n", ret, outMessage);
        goto disposeEngine;
    }

    local_text = (pico_Char *) text;
    text_remaining = strlen((const char *) local_text) + 1;
    inp = (pico_Char *) local_text;

    size_t bufused = 0;
    picoos_Common common = (picoos_Common) pico_sysGetCommon(picoSystem);
    picoos_SDFile sdOutFile = NULL;

    picoos_bool done = TRUE;
    if(TRUE != (done = picoos_sdfOpenOut(common, &sdOutFile,
        (picoos_char *) wavefile, SAMPLE_FREQ_16KHZ, PICOOS_ENC_LIN)))
    {
        fprintf(stderr, "Cannot open output wave file\n");
        ret = 1;
        goto disposeEngine;
    }

    printf("Synthesizing text (%zu bytes)...\n", strlen(text));

    /* Synthesis loop */
    while (text_remaining) {
        /* Feed the text into the engine */
        if((ret = pico_putTextUtf8(picoEngine, inp, text_remaining, &bytes_sent))) {
            pico_getSystemStatusMessage(picoSystem, ret, outMessage);
            fprintf(stderr, "Cannot put Text (%i): %s\n", ret, outMessage);
            goto disposeEngine;
        }

        text_remaining -= bytes_sent;
        inp += bytes_sent;

        do {
            if (picoSynthAbort) {
                goto disposeEngine;
            }
            /* Retrieve the samples and add them to the buffer */
            getstatus = pico_getData(picoEngine, (void *) outbuf,
                      MAX_OUTBUF_SIZE, &bytes_recv, &out_data_type);
            if((getstatus != PICO_STEP_BUSY) && (getstatus != PICO_STEP_IDLE)) {
                pico_getSystemStatusMessage(picoSystem, getstatus, outMessage);
                fprintf(stderr, "Cannot get Data (%i): %s\n", getstatus, outMessage);
                goto disposeEngine;
            }
            if (bytes_recv) {
                if ((bufused + bytes_recv) <= bufferSize) {
                    memcpy(buffer+bufused, (int8_t *) outbuf, bytes_recv);
                    bufused += bytes_recv;
                } else {
                    done = picoos_sdfPutSamples(sdOutFile, bufused / 2,
                                                (picoos_int16*) (buffer));
                    bufused = 0;
                    memcpy(buffer, (int8_t *) outbuf, bytes_recv);
                    bufused += bytes_recv;
                }
            }
        } while (PICO_STEP_BUSY == getstatus);
        
        /* This chunk of synthesis is finished; pass the remaining samples */
        if (!picoSynthAbort) {
            done = picoos_sdfPutSamples(sdOutFile, bufused / 2,
                                        (picoos_int16*) (buffer));
        }
        picoSynthAbort = 0;
    }

    if(TRUE != (done = picoos_sdfCloseOut(common, &sdOutFile))) {
        fprintf(stderr, "Cannot close output wave file\n");
        ret = 1;
        goto disposeEngine;
    }

    printf("Synthesis complete! Output written to: %s\n", wavefile);

#if PICO_USE_QUALITY_ENHANCE
    /* Show statistics if requested */
    if (show_stats) {
        pico_quality_stats_t stats;
        if (pico_get_quality_stats(&stats) == PICO_OK) {
            printf("\nQuality Enhancement Statistics:\n");
            printf("  Noise samples generated: %u\n", stats.noise_samples_generated);
            printf("  Filter updates: %u\n", stats.filter_updates);
            printf("  Pitch adjustments: %u\n", stats.pitch_adjustments);
            printf("  Formant shifts: %u\n", stats.formant_shifts);
        }
    }
    
    /* Cleanup quality enhancements */
    pico_quality_cleanup();
#endif

disposeEngine:
    if (picoEngine) {
        pico_disposeEngine(picoSystem, &picoEngine);
        pico_releaseVoiceDefinition(picoSystem, (pico_Char *) PICO_VOICE_NAME);
        picoEngine = NULL;
    }
unloadUtppResource:
    if (picoUtppResource) {
        pico_unloadResource(picoSystem, &picoUtppResource);
        picoUtppResource = NULL;
    }
unloadSgResource:
    if (picoSgResource) {
        pico_unloadResource(picoSystem, &picoSgResource);
        picoSgResource = NULL;
    }
unloadTaResource:
    if (picoTaResource) {
        pico_unloadResource(picoSystem, &picoTaResource);
        picoTaResource = NULL;
    }
terminate:
    if (picoSystem) {
        pico_terminate(&picoSystem);
        picoSystem = NULL;
    }
    exit(ret);
}
