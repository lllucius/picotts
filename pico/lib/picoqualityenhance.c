/*
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
 */

/**
 * @file picoqualityenhance.c
 *
 * Quality Enhancement Module Implementation
 *
 * Implements speech quality improvements for PicoTTS:
 * - Improved excitation generation with LPC noise shaping
 * - Voice customization (pitch, speed, formant control)
 * - Quality mode presets
 * - Enhanced prosody controls
 */

#include "picoqualityenhance.h"
#include "picoos.h"
#include "picodefs.h"

#if PICO_USE_QUALITY_ENHANCE

/*******************************************************************************
 * Global State
 ******************************************************************************/

/* Global quality context */
static pico_quality_context_t g_quality_ctx = {
    .initialized = 0
};

/* Global statistics */
static pico_quality_stats_t g_quality_stats = {0};

/*******************************************************************************
 * Initialization and Configuration
 ******************************************************************************/

picoos_int32 pico_quality_init(void)
{
    if (g_quality_ctx.initialized) {
        return PICO_OK;  /* Already initialized */
    }

    /* Initialize default voice parameters */
    g_quality_ctx.voice_params.pitch_scale = PICO_DEFAULT_PITCH_SCALE;
    g_quality_ctx.voice_params.speed_scale = PICO_DEFAULT_SPEED_SCALE;
    g_quality_ctx.voice_params.formant_shift = PICO_DEFAULT_FORMANT_SHIFT;
    g_quality_ctx.voice_params.quality_mode = PICO_DEFAULT_QUALITY_MODE;

    /* Initialize default prosody parameters */
    g_quality_ctx.prosody_params.emphasis_scale = PICO_DEFAULT_EMPHASIS_SCALE;
    g_quality_ctx.prosody_params.pause_scale = PICO_DEFAULT_PAUSE_SCALE;
    g_quality_ctx.prosody_params.question_boost = PICO_DEFAULT_QUESTION_BOOST;

    /* Initialize noise filter */
    g_quality_ctx.noise_filter.order = PICO_NOISE_FILTER_ORDER;
    {
        int i;
        for (i = 0; i < PICO_NOISE_FILTER_ORDER; i++) {
            g_quality_ctx.noise_filter.state[i] = 0;
            /* Default coefficients (simple all-pass initially) */
            g_quality_ctx.noise_filter.coeffs[i] = 0;
        }
    }

    /* Initialize random seed */
    g_quality_ctx.random_seed = 12345;

    /* Reset statistics */
    pico_reset_quality_stats();

    g_quality_ctx.initialized = 1;

    return PICO_OK;
}

picoos_int32 pico_quality_cleanup(void)
{
    if (!g_quality_ctx.initialized) {
        return PICO_OK;
    }

    /* Reset all state */
    g_quality_ctx.initialized = 0;

    return PICO_OK;
}

picoos_int32 pico_set_quality_mode(picoos_int8 mode)
{
    if (mode < PICO_QUALITY_MODE_SPEED || mode > PICO_QUALITY_MODE_QUALITY) {
        return PICO_ERR_OTHER;
    }

    g_quality_ctx.voice_params.quality_mode = mode;

    return PICO_OK;
}

picoos_int8 pico_get_quality_mode(void)
{
    return g_quality_ctx.voice_params.quality_mode;
}

/*******************************************************************************
 * Voice Parameter Control
 ******************************************************************************/

picoos_int32 pico_validate_voice_params(pico_voice_params_t *params)
{
    if (!params) {
        return PICO_ERR_NULLPTR_ACCESS;
    }

    /* Validate pitch scale */
    if (params->pitch_scale < PICO_PITCH_SCALE_MIN ||
        params->pitch_scale > PICO_PITCH_SCALE_MAX) {
        return PICO_ERR_OTHER;
    }

    /* Validate speed scale */
    if (params->speed_scale < PICO_SPEED_SCALE_MIN ||
        params->speed_scale > PICO_SPEED_SCALE_MAX) {
        return PICO_ERR_OTHER;
    }

    /* Validate formant shift */
    if (params->formant_shift < PICO_FORMANT_SHIFT_MIN ||
        params->formant_shift > PICO_FORMANT_SHIFT_MAX) {
        return PICO_ERR_OTHER;
    }

    /* Validate quality mode */
    if (params->quality_mode < PICO_QUALITY_MODE_SPEED ||
        params->quality_mode > PICO_QUALITY_MODE_QUALITY) {
        return PICO_ERR_OTHER;
    }

    return PICO_OK;
}

picoos_int32 pico_set_voice_params(pico_voice_params_t *params)
{
    picoos_int32 result;

    if (!params) {
        return PICO_ERR_NULLPTR_ACCESS;
    }

    /* Validate parameters */
    result = pico_validate_voice_params(params);
    if (result != PICO_OK) {
        return result;
    }

    /* Apply parameters */
    g_quality_ctx.voice_params = *params;

    return PICO_OK;
}

picoos_int32 pico_get_voice_params(pico_voice_params_t *params)
{
    if (!params) {
        return PICO_ERR_NULLPTR_ACCESS;
    }

    *params = g_quality_ctx.voice_params;

    return PICO_OK;
}

picoos_int32 pico_reset_voice_params(void)
{
    g_quality_ctx.voice_params.pitch_scale = PICO_DEFAULT_PITCH_SCALE;
    g_quality_ctx.voice_params.speed_scale = PICO_DEFAULT_SPEED_SCALE;
    g_quality_ctx.voice_params.formant_shift = PICO_DEFAULT_FORMANT_SHIFT;
    g_quality_ctx.voice_params.quality_mode = PICO_DEFAULT_QUALITY_MODE;

    return PICO_OK;
}

/*******************************************************************************
 * Prosody Control
 ******************************************************************************/

picoos_int32 pico_set_prosody_params(pico_prosody_params_t *params)
{
    if (!params) {
        return PICO_ERR_NULLPTR_ACCESS;
    }

    /* Validate and clamp parameters */
    params->emphasis_scale = pico_clamp_float(params->emphasis_scale, 0.5f, 2.0f);
    params->pause_scale = pico_clamp_float(params->pause_scale, 0.5f, 2.0f);
    
    if (params->question_boost < 0) params->question_boost = 0;
    if (params->question_boost > 100) params->question_boost = 100;

    /* Apply parameters */
    g_quality_ctx.prosody_params = *params;

    return PICO_OK;
}

picoos_int32 pico_get_prosody_params(pico_prosody_params_t *params)
{
    if (!params) {
        return PICO_ERR_NULLPTR_ACCESS;
    }

    *params = g_quality_ctx.prosody_params;

    return PICO_OK;
}

picoos_int32 pico_reset_prosody_params(void)
{
    g_quality_ctx.prosody_params.emphasis_scale = PICO_DEFAULT_EMPHASIS_SCALE;
    g_quality_ctx.prosody_params.pause_scale = PICO_DEFAULT_PAUSE_SCALE;
    g_quality_ctx.prosody_params.question_boost = PICO_DEFAULT_QUESTION_BOOST;

    return PICO_OK;
}

/*******************************************************************************
 * Excitation Generation (Improved Noise Shaping)
 ******************************************************************************/

picoos_int32 pico_noise_filter_init(
    pico_noise_filter_t *filter,
    picoos_int16 *coeffs,
    picoos_int8 order)
{
    int i;

    if (!filter || !coeffs) {
        return PICO_ERR_NULLPTR_ACCESS;
    }

    if (order <= 0 || order > PICO_NOISE_FILTER_ORDER) {
        return PICO_ERR_OTHER;
    }

    filter->order = order;

    /* Initialize state to zero */
    for (i = 0; i < PICO_NOISE_FILTER_ORDER; i++) {
        filter->state[i] = 0;
    }

    /* Copy coefficients */
    for (i = 0; i < order; i++) {
        filter->coeffs[i] = coeffs[i];
    }

    /* Zero remaining coefficients */
    for (i = order; i < PICO_NOISE_FILTER_ORDER; i++) {
        filter->coeffs[i] = 0;
    }

    return PICO_OK;
}

picoos_int16 pico_generate_white_noise(picoos_uint32 *seed)
{
    /* Linear congruential generator */
    /* Constants from Numerical Recipes */
    *seed = (*seed * 1664525U + 1013904223U);
    
    /* Convert to signed 16-bit range */
    return (picoos_int16)((*seed >> 16) & 0xFFFF) - 16384;
}

picoos_int16 pico_generate_shaped_noise(
    pico_noise_filter_t *filter,
    picoos_uint32 *seed)
{
    picoos_int32 output;
    picoos_int16 white_noise;
    int i;

    if (!filter || !seed) {
        return 0;
    }

    /* Generate white noise input */
    white_noise = pico_generate_white_noise(seed);

    /* Apply LPC filter (all-pole filter) */
    /* y[n] = x[n] - sum(a[i] * y[n-i]) */
    output = (picoos_int32)white_noise << 8;  /* Scale up for precision */

    for (i = 0; i < filter->order; i++) {
        output -= ((picoos_int32)filter->coeffs[i] * 
                   (picoos_int32)filter->state[i]) >> 7;
    }

    /* Shift state buffer */
    for (i = filter->order - 1; i > 0; i--) {
        filter->state[i] = filter->state[i-1];
    }

    /* Store new output */
    filter->state[0] = (picoos_int16)(output >> 8);

    /* Update statistics */
    g_quality_stats.noise_samples_generated++;

    return filter->state[0];
}

picoos_int32 pico_noise_filter_update(
    pico_noise_filter_t *filter,
    picoos_int16 *coeffs)
{
    int i;

    if (!filter || !coeffs) {
        return PICO_ERR_NULLPTR_ACCESS;
    }

    /* Update coefficients */
    for (i = 0; i < filter->order; i++) {
        filter->coeffs[i] = coeffs[i];
    }

    /* Update statistics */
    g_quality_stats.filter_updates++;

    return PICO_OK;
}

/*******************************************************************************
 * Utility Functions
 ******************************************************************************/

picoos_int16 pico_apply_pitch_scale(picoos_int16 f0, picoos_single scale)
{
    picoos_single scaled_f0;

    if (f0 <= 0) {
        return f0;  /* Unvoiced or invalid */
    }

    /* Apply scaling */
    scaled_f0 = (picoos_single)f0 * scale;

    /* Clamp to reasonable range (50-500 Hz) */
    if (scaled_f0 < 50.0f) scaled_f0 = 50.0f;
    if (scaled_f0 > 500.0f) scaled_f0 = 500.0f;

    /* Update statistics */
    g_quality_stats.pitch_adjustments++;

    return (picoos_int16)scaled_f0;
}

picoos_int16 pico_apply_formant_shift(picoos_int16 formant, picoos_single shift)
{
    picoos_single shifted_formant;

    if (formant <= 0) {
        return formant;
    }

    /* Apply shift */
    shifted_formant = (picoos_single)formant + shift;

    /* Clamp to reasonable range (200-5000 Hz) */
    if (shifted_formant < 200.0f) shifted_formant = 200.0f;
    if (shifted_formant > 5000.0f) shifted_formant = 5000.0f;

    /* Update statistics */
    g_quality_stats.formant_shifts++;

    return (picoos_int16)shifted_formant;
}

picoos_single pico_clamp_float(picoos_single value, picoos_single min, picoos_single max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/*******************************************************************************
 * Preset Voice Profiles
 ******************************************************************************/

picoos_int32 pico_apply_voice_profile(pico_voice_profile_t profile)
{
    pico_voice_params_t params = g_quality_ctx.voice_params;
    pico_prosody_params_t prosody = g_quality_ctx.prosody_params;

    /* Reset to defaults first */
    params.pitch_scale = PICO_DEFAULT_PITCH_SCALE;
    params.speed_scale = PICO_DEFAULT_SPEED_SCALE;
    params.formant_shift = PICO_DEFAULT_FORMANT_SHIFT;
    prosody.emphasis_scale = PICO_DEFAULT_EMPHASIS_SCALE;
    prosody.pause_scale = PICO_DEFAULT_PAUSE_SCALE;

    /* Apply profile-specific settings */
    switch (profile) {
        case PICO_VOICE_PROFILE_MALE:
            params.pitch_scale = PICO_MALE_PITCH_SCALE;
            params.formant_shift = PICO_MALE_FORMANT_SHIFT;
            break;

        case PICO_VOICE_PROFILE_FEMALE:
            params.pitch_scale = PICO_FEMALE_PITCH_SCALE;
            params.formant_shift = PICO_FEMALE_FORMANT_SHIFT;
            break;

        case PICO_VOICE_PROFILE_CHILD:
            params.pitch_scale = PICO_CHILD_PITCH_SCALE;
            params.speed_scale = PICO_CHILD_SPEED_SCALE;
            break;

        case PICO_VOICE_PROFILE_ROBOT:
            params.pitch_scale = PICO_ROBOT_PITCH_SCALE;
            prosody.emphasis_scale = PICO_ROBOT_EMPHASIS_SCALE;
            break;

        case PICO_VOICE_PROFILE_SLOW:
            params.speed_scale = PICO_SLOW_SPEED_SCALE;
            prosody.pause_scale = PICO_SLOW_PAUSE_SCALE;
            break;

        case PICO_VOICE_PROFILE_FAST:
            params.speed_scale = PICO_FAST_SPEED_SCALE;
            prosody.pause_scale = PICO_FAST_PAUSE_SCALE;
            break;

        case PICO_VOICE_PROFILE_DEFAULT:
        default:
            /* Already set to defaults */
            break;
    }

    /* Apply the parameters */
    pico_set_voice_params(&params);
    pico_set_prosody_params(&prosody);

    return PICO_OK;
}

/*******************************************************************************
 * Statistics and Debugging
 ******************************************************************************/

picoos_int32 pico_get_quality_stats(pico_quality_stats_t *stats)
{
    if (!stats) {
        return PICO_ERR_NULLPTR_ACCESS;
    }

    *stats = g_quality_stats;

    return PICO_OK;
}

picoos_int32 pico_reset_quality_stats(void)
{
    g_quality_stats.noise_samples_generated = 0;
    g_quality_stats.filter_updates = 0;
    g_quality_stats.pitch_adjustments = 0;
    g_quality_stats.formant_shifts = 0;

    return PICO_OK;
}

#else /* !PICO_USE_QUALITY_ENHANCE */

/*******************************************************************************
 * Stub implementations when quality enhancement is disabled
 ******************************************************************************/

picoos_int32 pico_quality_init(void) { return PICO_OK; }
picoos_int32 pico_quality_cleanup(void) { return PICO_OK; }
picoos_int32 pico_set_quality_mode(picoos_int8 mode) { (void)mode; return PICO_OK; }
picoos_int8 pico_get_quality_mode(void) { return PICO_QUALITY_MODE_BALANCED; }

picoos_int32 pico_set_voice_params(pico_voice_params_t *params) { 
    (void)params; 
    return PICO_OK; 
}

picoos_int32 pico_get_voice_params(pico_voice_params_t *params) { 
    if (params) {
        params->pitch_scale = 1.0f;
        params->speed_scale = 1.0f;
        params->formant_shift = 0.0f;
        params->quality_mode = PICO_QUALITY_MODE_BALANCED;
    }
    return PICO_OK; 
}

picoos_int32 pico_reset_voice_params(void) { return PICO_OK; }
picoos_int32 pico_validate_voice_params(pico_voice_params_t *params) { 
    (void)params; 
    return PICO_OK; 
}

picoos_int32 pico_set_prosody_params(pico_prosody_params_t *params) { 
    (void)params; 
    return PICO_OK; 
}

picoos_int32 pico_get_prosody_params(pico_prosody_params_t *params) { 
    if (params) {
        params->emphasis_scale = 1.0f;
        params->pause_scale = 1.0f;
        params->question_boost = 50;
    }
    return PICO_OK; 
}

picoos_int32 pico_reset_prosody_params(void) { return PICO_OK; }

picoos_int32 pico_noise_filter_init(pico_noise_filter_t *f, picoos_int16 *c, picoos_int8 o) {
    (void)f; (void)c; (void)o;
    return PICO_OK;
}

picoos_int16 pico_generate_shaped_noise(pico_noise_filter_t *f, picoos_uint32 *s) {
    (void)f;
    return pico_generate_white_noise(s);
}

picoos_int16 pico_generate_white_noise(picoos_uint32 *seed) {
    *seed = (*seed * 1664525U + 1013904223U);
    return (picoos_int16)((*seed >> 16) & 0xFFFF) - 16384;
}

picoos_int32 pico_noise_filter_update(pico_noise_filter_t *f, picoos_int16 *c) {
    (void)f; (void)c;
    return PICO_OK;
}

picoos_int16 pico_apply_pitch_scale(picoos_int16 f0, picoos_single scale) {
    (void)scale;
    return f0;
}

picoos_int16 pico_apply_formant_shift(picoos_int16 formant, picoos_single shift) {
    (void)shift;
    return formant;
}

picoos_single pico_clamp_float(picoos_single value, picoos_single min, picoos_single max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

picoos_int32 pico_apply_voice_profile(pico_voice_profile_t profile) {
    (void)profile;
    return PICO_OK;
}

picoos_int32 pico_get_quality_stats(pico_quality_stats_t *stats) {
    if (stats) {
        stats->noise_samples_generated = 0;
        stats->filter_updates = 0;
        stats->pitch_adjustments = 0;
        stats->formant_shifts = 0;
    }
    return PICO_OK;
}

picoos_int32 pico_reset_quality_stats(void) { return PICO_OK; }

#endif /* PICO_USE_QUALITY_ENHANCE */
