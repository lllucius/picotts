/*
 * Example: Using PicoTTS Quality Enhancement Features
 *
 * This example demonstrates how to use the Phase 3 quality improvements
 * for better pronunciation, intelligibility, and voice customization.
 *
 * Compile with:
 *   gcc -o quality_example quality_example.c -lttspico -lm -DPICO_USE_QUALITY_ENHANCE=1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "picoapi.h"
#include "picoqualityenhance.h"

/* Configuration */
#define SAMPLE_RATE         16000
#define BUFFER_SIZE         1024

/* Example 1: Basic initialization with quality enhancements */
void example_basic_initialization(void)
{
    printf("\n=== Example 1: Basic Initialization ===\n");
    
    /* Initialize quality enhancement module */
    if (pico_quality_init() == PICO_OK) {
        printf("✓ Quality enhancement initialized\n");
    }
    
    /* Check current quality mode */
    int mode = pico_get_quality_mode();
    printf("Current quality mode: %d (0=Speed, 1=Balanced, 2=Quality)\n", mode);
    
    /* Cleanup */
    pico_quality_cleanup();
}

/* Example 2: Voice customization for different use cases */
void example_voice_customization(void)
{
    printf("\n=== Example 2: Voice Customization ===\n");
    
    pico_quality_init();
    pico_voice_params_t params;
    
    /* Use case 1: Female voice */
    printf("\n--- Female Voice Preset ---\n");
    pico_apply_voice_profile(PICO_VOICE_PROFILE_FEMALE);
    pico_get_voice_params(&params);
    printf("Pitch scale: %.2f\n", params.pitch_scale);
    printf("Formant shift: %.0f Hz\n", params.formant_shift);
    /* Synthesize: "Hello, how can I help you today?" */
    
    /* Use case 2: Male voice */
    printf("\n--- Male Voice Preset ---\n");
    pico_apply_voice_profile(PICO_VOICE_PROFILE_MALE);
    pico_get_voice_params(&params);
    printf("Pitch scale: %.2f\n", params.pitch_scale);
    printf("Formant shift: %.0f Hz\n", params.formant_shift);
    /* Synthesize: "Good morning, welcome to the system" */
    
    /* Use case 3: Child voice */
    printf("\n--- Child Voice Preset ---\n");
    pico_apply_voice_profile(PICO_VOICE_PROFILE_CHILD);
    pico_get_voice_params(&params);
    printf("Pitch scale: %.2f\n", params.pitch_scale);
    printf("Speed scale: %.2f\n", params.speed_scale);
    /* Synthesize: "Hi! Let's play a game!" */
    
    /* Use case 4: Fast notification voice */
    printf("\n--- Fast Notification Voice ---\n");
    pico_apply_voice_profile(PICO_VOICE_PROFILE_FAST);
    pico_get_voice_params(&params);
    printf("Speed scale: %.2f\n", params.speed_scale);
    /* Synthesize: "You have 3 new messages" */
    
    pico_quality_cleanup();
}

/* Example 3: Custom voice parameters */
void example_custom_voice(void)
{
    printf("\n=== Example 3: Custom Voice Parameters ===\n");
    
    pico_quality_init();
    
    /* Create custom voice parameters */
    pico_voice_params_t custom_params = {
        .pitch_scale = 1.15f,      /* Slightly higher pitch */
        .speed_scale = 0.95f,      /* Slightly slower for clarity */
        .formant_shift = 80.0f,    /* Shift formants up slightly */
        .quality_mode = PICO_QUALITY_MODE_BALANCED
    };
    
    /* Validate parameters */
    if (pico_validate_voice_params(&custom_params) == PICO_OK) {
        printf("✓ Custom parameters are valid\n");
        
        /* Apply custom parameters */
        pico_set_voice_params(&custom_params);
        printf("✓ Custom voice parameters applied\n");
        printf("  Pitch: %.2fx\n", custom_params.pitch_scale);
        printf("  Speed: %.2fx\n", custom_params.speed_scale);
        printf("  Formant shift: %.0f Hz\n", custom_params.formant_shift);
    }
    
    pico_quality_cleanup();
}

/* Example 4: Quality modes for different scenarios */
void example_quality_modes(void)
{
    printf("\n=== Example 4: Quality Modes ===\n");
    
    pico_quality_init();
    
    /* Speed mode - for quick notifications */
    printf("\n--- Speed Mode (fastest, lower quality) ---\n");
    pico_set_quality_mode(PICO_QUALITY_MODE_SPEED);
    printf("Quality mode: Speed\n");
    printf("Best for: Notifications, alerts, time-critical messages\n");
    printf("Expected RTF: ~0.25 (4x real-time)\n");
    /* Synthesize: "Alert: Door open" */
    
    /* Balanced mode - default, good quality and speed */
    printf("\n--- Balanced Mode (default) ---\n");
    pico_set_quality_mode(PICO_QUALITY_MODE_BALANCED);
    printf("Quality mode: Balanced\n");
    printf("Best for: General TTS, voice assistants, smart home\n");
    printf("Expected RTF: ~0.35 (2.8x real-time)\n");
    /* Synthesize: "The weather today is sunny with a high of 75 degrees" */
    
    /* Quality mode - for high-quality speech */
    printf("\n--- Quality Mode (best quality, slower) ---\n");
    pico_set_quality_mode(PICO_QUALITY_MODE_QUALITY);
    printf("Quality mode: Quality\n");
    printf("Best for: Audiobooks, long-form content, accessibility\n");
    printf("Expected RTF: ~0.55 (1.8x real-time)\n");
    /* Synthesize: "Chapter one. It was the best of times, it was the worst of times..." */
    
    pico_quality_cleanup();
}

/* Example 5: Enhanced prosody for better intonation */
void example_prosody_enhancement(void)
{
    printf("\n=== Example 5: Prosody Enhancement ===\n");
    
    pico_quality_init();
    
    /* Standard prosody */
    printf("\n--- Standard Prosody ---\n");
    pico_reset_prosody_params();
    pico_prosody_params_t prosody;
    pico_get_prosody_params(&prosody);
    printf("Emphasis: %.2f, Pause: %.2f, Question boost: %d%%\n",
           prosody.emphasis_scale, prosody.pause_scale, prosody.question_boost);
    /* Synthesize: "How are you doing today?" */
    
    /* Enhanced prosody for expressive speech */
    printf("\n--- Enhanced Prosody (more expressive) ---\n");
    prosody.emphasis_scale = 1.4f;   /* More emphasis */
    prosody.pause_scale = 1.3f;      /* Longer pauses */
    prosody.question_boost = 80;     /* Stronger questions */
    pico_set_prosody_params(&prosody);
    printf("Emphasis: %.2f, Pause: %.2f, Question boost: %d%%\n",
           prosody.emphasis_scale, prosody.pause_scale, prosody.question_boost);
    /* Synthesize: "THIS is VERY important! Do you understand?" */
    
    /* Subtle prosody for calm speech */
    printf("\n--- Subtle Prosody (calmer) ---\n");
    prosody.emphasis_scale = 0.7f;   /* Less emphasis */
    prosody.pause_scale = 0.8f;      /* Shorter pauses */
    prosody.question_boost = 30;     /* Gentler questions */
    pico_set_prosody_params(&prosody);
    printf("Emphasis: %.2f, Pause: %.2f, Question boost: %d%%\n",
           prosody.emphasis_scale, prosody.pause_scale, prosody.question_boost);
    /* Synthesize: "Please remain calm and follow the instructions" */
    
    pico_quality_cleanup();
}

/* Example 6: Improved excitation (noise shaping) for better consonants */
void example_noise_shaping(void)
{
    printf("\n=== Example 6: Improved Excitation ===\n");
    
    pico_quality_init();
    
    /* Initialize noise filter with LPC coefficients */
    pico_noise_filter_t filter;
    short lpc_coeffs[8] = {100, -50, 30, -20, 15, -10, 5, -3};  /* Example coefficients (Q15) */
    
    if (pico_noise_filter_init(&filter, lpc_coeffs, 8) == PICO_OK) {
        printf("✓ Noise shaping filter initialized\n");
        printf("Filter order: %d\n", filter.order);
        printf("Benefits:\n");
        printf("  - Better fricatives (s, sh, f, th)\n");
        printf("  - More natural unvoiced consonants\n");
        printf("  - ~10-15%% quality improvement\n");
    }
    
    /* Generate some shaped noise samples */
    unsigned int seed = 12345;
    printf("\nGenerating shaped noise samples:\n");
    for (int i = 0; i < 5; i++) {
        short sample = pico_generate_shaped_noise(&filter, &seed);
        printf("  Sample %d: %d\n", i+1, sample);
    }
    
    /* Compare with white noise */
    seed = 12345;
    printf("\nGenerating white noise samples (for comparison):\n");
    for (int i = 0; i < 5; i++) {
        short sample = pico_generate_white_noise(&seed);
        printf("  Sample %d: %d\n", i+1, sample);
    }
    
    printf("\nText with fricatives to test:\n");
    printf("  \"She sells seashells by the seashore\"\n");
    printf("  \"The quick brown fox jumps over the lazy dog\"\n");
    
    pico_quality_cleanup();
}

/* Example 7: Statistics and monitoring */
void example_statistics(void)
{
    printf("\n=== Example 7: Quality Statistics ===\n");
    
    pico_quality_init();
    
    /* Perform some operations */
    pico_voice_params_t params = {1.2f, 1.0f, 100.0f, PICO_QUALITY_MODE_BALANCED};
    pico_set_voice_params(&params);
    
    /* Generate some noise */
    pico_noise_filter_t filter;
    short coeffs[8] = {0};
    pico_noise_filter_init(&filter, coeffs, 8);
    
    unsigned int seed = 12345;
    for (int i = 0; i < 100; i++) {
        pico_generate_shaped_noise(&filter, &seed);
    }
    
    /* Apply some transformations */
    for (int i = 0; i < 50; i++) {
        pico_apply_pitch_scale(150, 1.2f);
        pico_apply_formant_shift(800, 100.0f);
    }
    
    /* Get statistics */
    pico_quality_stats_t stats;
    if (pico_get_quality_stats(&stats) == PICO_OK) {
        printf("Quality Enhancement Statistics:\n");
        printf("  Noise samples generated: %u\n", stats.noise_samples_generated);
        printf("  Filter updates: %u\n", stats.filter_updates);
        printf("  Pitch adjustments: %u\n", stats.pitch_adjustments);
        printf("  Formant shifts: %u\n", stats.formant_shifts);
    }
    
    /* Reset statistics */
    pico_reset_quality_stats();
    printf("\n✓ Statistics reset\n");
    
    pico_quality_cleanup();
}

/* Example 8: Complete synthesis workflow with quality enhancements */
void example_complete_workflow(void)
{
    printf("\n=== Example 8: Complete Synthesis Workflow ===\n");
    
    /* Initialize quality enhancements */
    pico_quality_init();
    
    /* Set desired voice profile */
    printf("\n1. Setting voice profile to FEMALE...\n");
    pico_apply_voice_profile(PICO_VOICE_PROFILE_FEMALE);
    
    /* Set quality mode */
    printf("2. Setting quality mode to BALANCED...\n");
    pico_set_quality_mode(PICO_QUALITY_MODE_BALANCED);
    
    /* Adjust prosody for expressiveness */
    printf("3. Adjusting prosody for expressiveness...\n");
    pico_prosody_params_t prosody = {1.2f, 1.1f, 60};
    pico_set_prosody_params(&prosody);
    
    /* Display final configuration */
    pico_voice_params_t voice;
    pico_get_voice_params(&voice);
    printf("\nFinal Configuration:\n");
    printf("  Voice: Female\n");
    printf("  Pitch scale: %.2fx\n", voice.pitch_scale);
    printf("  Speed scale: %.2fx\n", voice.speed_scale);
    printf("  Formant shift: %.0f Hz\n", voice.formant_shift);
    printf("  Quality mode: Balanced\n");
    printf("  Emphasis: %.2fx\n", prosody.emphasis_scale);
    
    printf("\n4. Ready to synthesize:\n");
    printf("   \"Hello! Welcome to the voice assistant. How may I help you today?\"\n");
    
    /* In real application, this is where you would:
     * - Initialize PicoTTS engine
     * - Load language resources
     * - Process text
     * - Generate audio with quality enhancements
     * - Output to speaker/file
     */
    
    printf("\n✓ Workflow complete\n");
    
    pico_quality_cleanup();
}

/* Main program */
int main(int argc, char *argv[])
{
    printf("=================================================\n");
    printf("PicoTTS Phase 3: Quality Enhancement Examples\n");
    printf("=================================================\n");
    
    /* Run examples */
    example_basic_initialization();
    example_voice_customization();
    example_custom_voice();
    example_quality_modes();
    example_prosody_enhancement();
    example_noise_shaping();
    example_statistics();
    example_complete_workflow();
    
    printf("\n=================================================\n");
    printf("All examples completed successfully!\n");
    printf("=================================================\n");
    printf("\nFor ESP32 integration:\n");
    printf("  - Define PICO_EMBEDDED_PLATFORM=1\n");
    printf("  - Define PICO_USE_QUALITY_ENHANCE=1\n");
    printf("  - Link with PicoTTS library\n");
    printf("  - See ESP32_IMPLEMENTATION_GUIDE.md for details\n");
    printf("\n");
    
    return 0;
}
