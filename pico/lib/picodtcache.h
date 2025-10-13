/*
 * Copyright (C) 2025 - Phase 2 Performance Optimizations
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
 * @file picodtcache.h
 *
 * Decision Tree Cache for PAM optimization (Phase 2)
 * 
 * Caches frequent decision tree traversal results to avoid
 * repeated tree walks for common phoneme contexts.
 * 
 * Expected performance:
 * - 50-70% cache hit rate for common contexts
 * - 20-30% PAM stage speedup
 * - Only 1-2 KB memory overhead
 * 
 * Usage:
 *   Enable with: -DPICO_USE_DT_CACHE=1
 */

#ifndef PICODTCACHE_H_
#define PICODTCACHE_H_

#include "picodefs.h"
#include "picoos.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Configuration
 * ============================================================================ */

/* Enable decision tree caching (Phase 2 optimization) */
#if defined(PICO_USE_DT_CACHE) || defined(PICO_EMBEDDED_PLATFORM)
    #define PICO_DT_CACHE_ENABLED 1
#endif

/* Cache size (power of 2 for fast modulo) */
#ifndef PICO_DT_CACHE_SIZE
    #ifdef PICO_EMBEDDED_PLATFORM
        #define PICO_DT_CACHE_SIZE 128  /* Smaller for embedded */
    #else
        #define PICO_DT_CACHE_SIZE 256  /* Larger for desktop */
    #endif
#endif

/* Cache entry structure */
typedef struct {
    picoos_uint32 context_hash;  /* Hash of linguistic context */
    picoos_uint16 pdf_index;     /* Cached PDF result */
    picoos_uint16 tree_id;       /* Which tree this applies to */
    picoos_uint8  valid;         /* Is this entry valid? */
    picoos_uint8  access_count;  /* LRU counter */
} picodt_cache_entry_t;

/* Cache statistics (for profiling) */
typedef struct {
    picoos_uint32 hits;          /* Number of cache hits */
    picoos_uint32 misses;        /* Number of cache misses */
    picoos_uint32 collisions;    /* Number of hash collisions */
    picoos_uint32 evictions;     /* Number of LRU evictions */
} picodt_cache_stats_t;

/* Cache object */
typedef struct {
    picodt_cache_entry_t entries[PICO_DT_CACHE_SIZE];
    picodt_cache_stats_t stats;
    picoos_uint8 enabled;
    picoos_uint8 clock;  /* Global clock for LRU */
} picodt_cache_t;

/* ============================================================================
 * Public API
 * ============================================================================ */

/**
 * Initialize decision tree cache
 * @param mm Memory manager
 * @param cache Pointer to cache structure
 * @return PICO_OK on success
 */
picoos_uint8 picodt_cache_initialize(picoos_MemoryManager mm, 
                                     picodt_cache_t **cache);

/**
 * Deallocate decision tree cache
 * @param mm Memory manager
 * @param cache Pointer to cache structure
 */
void picodt_cache_deallocate(picoos_MemoryManager mm, 
                             picodt_cache_t **cache);

/**
 * Clear all cache entries
 * @param cache Cache structure
 */
void picodt_cache_clear(picodt_cache_t *cache);

/**
 * Lookup PDF index in cache
 * @param cache Cache structure
 * @param context_hash Hash of linguistic context
 * @param tree_id Decision tree identifier
 * @param pdf_index Output: PDF index if found
 * @return 1 if found (cache hit), 0 if not found (cache miss)
 */
picoos_uint8 picodt_cache_lookup(picodt_cache_t *cache,
                                 picoos_uint32 context_hash,
                                 picoos_uint16 tree_id,
                                 picoos_uint16 *pdf_index);

/**
 * Insert PDF index into cache
 * @param cache Cache structure
 * @param context_hash Hash of linguistic context
 * @param tree_id Decision tree identifier
 * @param pdf_index PDF index to cache
 */
void picodt_cache_insert(picodt_cache_t *cache,
                         picoos_uint32 context_hash,
                         picoos_uint16 tree_id,
                         picoos_uint16 pdf_index);

/**
 * Get cache statistics
 * @param cache Cache structure
 * @param stats Output: statistics structure
 */
void picodt_cache_get_stats(picodt_cache_t *cache,
                            picodt_cache_stats_t *stats);

/**
 * Calculate hit rate percentage
 * @param cache Cache structure
 * @return Hit rate as percentage (0-100)
 */
picoos_uint8 picodt_cache_hit_rate(picodt_cache_t *cache);

/* ============================================================================
 * Context Hashing
 * ============================================================================ */

/**
 * Compute hash of phoneme context for caching
 * Uses FNV-1a hash algorithm (fast and good distribution)
 * 
 * @param phoneme Current phoneme
 * @param prev_phoneme Previous phoneme
 * @param next_phoneme Next phoneme
 * @param stress Stress level
 * @param position Position in word
 * @return 32-bit hash value
 */
static inline picoos_uint32 picodt_compute_context_hash(
    picoos_uint8 phoneme,
    picoos_uint8 prev_phoneme,
    picoos_uint8 next_phoneme,
    picoos_uint8 stress,
    picoos_uint8 position)
{
    /* FNV-1a hash constants */
    picoos_uint32 hash = 2166136261U;
    const picoos_uint32 FNV_prime = 16777619U;
    
    /* Hash each component */
    hash ^= phoneme;
    hash *= FNV_prime;
    
    hash ^= prev_phoneme;
    hash *= FNV_prime;
    
    hash ^= next_phoneme;
    hash *= FNV_prime;
    
    hash ^= stress;
    hash *= FNV_prime;
    
    hash ^= position;
    hash *= FNV_prime;
    
    return hash;
}

/**
 * Extended context hash including more features
 * @param features Array of feature values
 * @param num_features Number of features to hash
 * @return 32-bit hash value
 */
picoos_uint32 picodt_compute_extended_hash(const picoos_uint8 *features,
                                           picoos_uint16 num_features);

/* ============================================================================
 * Inline Helpers
 * ============================================================================ */

/**
 * Check if cache is enabled
 */
static inline picoos_uint8 picodt_cache_is_enabled(picodt_cache_t *cache) {
    return (cache != NULL && cache->enabled);
}

/**
 * Enable/disable cache
 */
static inline void picodt_cache_set_enabled(picodt_cache_t *cache, 
                                            picoos_uint8 enabled) {
    if (cache) {
        cache->enabled = enabled;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* PICODTCACHE_H_ */
