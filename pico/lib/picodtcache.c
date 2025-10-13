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
 * @file picodtcache.c
 *
 * Decision Tree Cache implementation
 */

#include "picodtcache.h"
#include "picoos.h"

/* ============================================================================
 * Private Helpers
 * ============================================================================ */

/**
 * Get cache index from hash
 */
static inline picoos_uint16 get_cache_index(picoos_uint32 hash) {
    return (picoos_uint16)(hash & (PICO_DT_CACHE_SIZE - 1));
}

/**
 * Find least recently used entry in case of collision
 */
static inline picoos_uint16 find_lru_entry(picodt_cache_t *cache, 
                                           picoos_uint16 start_index) {
    picoos_uint16 lru_index = start_index;
    picoos_uint8 min_access = 255;
    
    /* Search a small window around the hash index */
    for (picoos_uint16 i = 0; i < 4; i++) {
        picoos_uint16 idx = (start_index + i) & (PICO_DT_CACHE_SIZE - 1);
        if (!cache->entries[idx].valid) {
            return idx;  /* Found empty slot */
        }
        if (cache->entries[idx].access_count < min_access) {
            min_access = cache->entries[idx].access_count;
            lru_index = idx;
        }
    }
    
    return lru_index;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================ */

/**
 * Initialize decision tree cache
 */
picoos_uint8 picodt_cache_initialize(picoos_MemoryManager mm, 
                                     picodt_cache_t **cache) {
    picodt_cache_t *new_cache;
    
    if (cache == NULL) {
        return PICO_ERR_NULLPTR_ACCESS;
    }
    
    /* Allocate cache structure */
    new_cache = (picodt_cache_t *)picoos_allocate(mm, sizeof(picodt_cache_t));
    if (new_cache == NULL) {
        return PICO_EXC_OUT_OF_MEM;
    }
    
    /* Initialize all entries */
    for (int i = 0; i < PICO_DT_CACHE_SIZE; i++) {
        new_cache->entries[i].valid = 0;
        new_cache->entries[i].access_count = 0;
    }
    
    /* Initialize statistics */
    new_cache->stats.hits = 0;
    new_cache->stats.misses = 0;
    new_cache->stats.collisions = 0;
    new_cache->stats.evictions = 0;
    
    /* Enable cache */
    new_cache->enabled = 1;
    new_cache->clock = 0;
    
    *cache = new_cache;
    return PICO_OK;
}

/**
 * Deallocate decision tree cache
 */
void picodt_cache_deallocate(picoos_MemoryManager mm, 
                             picodt_cache_t **cache) {
    if (cache && *cache) {
        picoos_deallocate(mm, (void **)cache);
    }
}

/**
 * Clear all cache entries
 */
void picodt_cache_clear(picodt_cache_t *cache) {
    if (cache == NULL) {
        return;
    }
    
    for (int i = 0; i < PICO_DT_CACHE_SIZE; i++) {
        cache->entries[i].valid = 0;
        cache->entries[i].access_count = 0;
    }
    
    cache->stats.hits = 0;
    cache->stats.misses = 0;
    cache->stats.collisions = 0;
    cache->stats.evictions = 0;
    cache->clock = 0;
}

/**
 * Lookup PDF index in cache
 */
picoos_uint8 picodt_cache_lookup(picodt_cache_t *cache,
                                 picoos_uint32 context_hash,
                                 picoos_uint16 tree_id,
                                 picoos_uint16 *pdf_index) {
    if (!picodt_cache_is_enabled(cache) || pdf_index == NULL) {
        return 0;
    }
    
    picoos_uint16 index = get_cache_index(context_hash);
    
    /* Check primary slot */
    if (cache->entries[index].valid &&
        cache->entries[index].context_hash == context_hash &&
        cache->entries[index].tree_id == tree_id) {
        
        /* Cache hit! */
        *pdf_index = cache->entries[index].pdf_index;
        cache->entries[index].access_count = cache->clock++;
        cache->stats.hits++;
        return 1;
    }
    
    /* Check nearby slots for collision handling */
    for (picoos_uint16 i = 1; i < 4; i++) {
        picoos_uint16 idx = (index + i) & (PICO_DT_CACHE_SIZE - 1);
        
        if (cache->entries[idx].valid &&
            cache->entries[idx].context_hash == context_hash &&
            cache->entries[idx].tree_id == tree_id) {
            
            /* Cache hit in collision slot */
            *pdf_index = cache->entries[idx].pdf_index;
            cache->entries[idx].access_count = cache->clock++;
            cache->stats.hits++;
            cache->stats.collisions++;
            return 1;
        }
    }
    
    /* Cache miss */
    cache->stats.misses++;
    return 0;
}

/**
 * Insert PDF index into cache
 */
void picodt_cache_insert(picodt_cache_t *cache,
                         picoos_uint32 context_hash,
                         picoos_uint16 tree_id,
                         picoos_uint16 pdf_index) {
    if (!picodt_cache_is_enabled(cache)) {
        return;
    }
    
    picoos_uint16 index = get_cache_index(context_hash);
    
    /* Try to insert in primary slot */
    if (!cache->entries[index].valid) {
        /* Empty slot, insert directly */
        cache->entries[index].context_hash = context_hash;
        cache->entries[index].pdf_index = pdf_index;
        cache->entries[index].tree_id = tree_id;
        cache->entries[index].valid = 1;
        cache->entries[index].access_count = cache->clock++;
        return;
    }
    
    /* Collision - find LRU entry */
    index = find_lru_entry(cache, index);
    
    if (cache->entries[index].valid) {
        cache->stats.evictions++;
    }
    
    /* Insert in LRU slot */
    cache->entries[index].context_hash = context_hash;
    cache->entries[index].pdf_index = pdf_index;
    cache->entries[index].tree_id = tree_id;
    cache->entries[index].valid = 1;
    cache->entries[index].access_count = cache->clock++;
}

/**
 * Get cache statistics
 */
void picodt_cache_get_stats(picodt_cache_t *cache,
                            picodt_cache_stats_t *stats) {
    if (cache && stats) {
        *stats = cache->stats;
    }
}

/**
 * Calculate hit rate percentage
 */
picoos_uint8 picodt_cache_hit_rate(picodt_cache_t *cache) {
    if (cache == NULL) {
        return 0;
    }
    
    picoos_uint32 total = cache->stats.hits + cache->stats.misses;
    if (total == 0) {
        return 0;
    }
    
    return (picoos_uint8)((cache->stats.hits * 100) / total);
}

/**
 * Extended context hash
 */
picoos_uint32 picodt_compute_extended_hash(const picoos_uint8 *features,
                                           picoos_uint16 num_features) {
    picoos_uint32 hash = 2166136261U;
    const picoos_uint32 FNV_prime = 16777619U;
    
    for (picoos_uint16 i = 0; i < num_features; i++) {
        hash ^= features[i];
        hash *= FNV_prime;
    }
    
    return hash;
}
