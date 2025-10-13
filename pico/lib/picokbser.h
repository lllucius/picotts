/*
 * Copyright (C) 2008-2009 SVOX AG, Baslerstr. 30, 8048 Zuerich, Switzerland
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
 * @file picokbser.h
 *
 * Knowledge Base Serialization API
 *
 * This module provides functionality to serialize loaded and specialized
 * knowledge bases to disk, allowing them to be reloaded directly without
 * re-processing. This significantly reduces startup time for embedded
 * systems, particularly when using a single language (e.g., English).
 *
 * Benefits:
 * - Faster startup (no specialization processing needed)
 * - Can be mapped directly from flash/SPIRAM on embedded systems
 * - Memory savings when using XIP (Execute-In-Place) architectures
 * 
 * Usage:
 * 1. Load and use TTS normally once
 * 2. Call picokbser_serializeResource to save specialized KB state
 * 3. On subsequent boots, use picokbser_deserializeResource for instant loading
 *
 */

#ifndef PICOKBSER_H_
#define PICOKBSER_H_

#include "picodefs.h"
#include "picoos.h"
#include "picoknow.h"
#include "picorsrc.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/* Magic number for serialized knowledge base files */
#define PICOKBSER_MAGIC_NUMBER 0x5049434F  /* "PICO" in hex */

/* Version of serialization format */
#define PICOKBSER_VERSION 1

/**
 * Serialize a loaded resource and all its knowledge bases to a file.
 * 
 * This saves the fully processed and specialized knowledge bases to disk,
 * including all internal structures. The resulting file can be memory-mapped
 * or loaded directly on subsequent runs, bypassing the specialization phase.
 *
 * @param resource - The resource to serialize (must be loaded and active)
 * @param fileName - Output file path for the serialized data
 * @param common - Common object for memory/error management
 * @return PICO_OK on success, error code otherwise
 */
pico_status_t picokbser_serializeResource(
    picorsrc_Resource resource,
    const picoos_char *fileName,
    picoos_Common common);

/**
 * Deserialize a previously serialized resource from a file.
 * 
 * This loads a pre-processed knowledge base directly, skipping all
 * specialization steps. The loaded resource is ready to use immediately.
 *
 * @param this - Resource manager
 * @param fileName - Input file path for the serialized data
 * @param resource - Output resource handle
 * @return PICO_OK on success, error code otherwise
 */
pico_status_t picokbser_deserializeResource(
    picorsrc_ResourceManager this,
    const picoos_char *fileName,
    picorsrc_Resource *resource);

/**
 * Check if a file is a valid serialized knowledge base.
 *
 * @param fileName - File to check
 * @param common - Common object for file operations
 * @return TRUE if file is a valid serialized KB, FALSE otherwise
 */
picoos_uint8 picokbser_isSerializedFile(
    const picoos_char *fileName,
    picoos_Common common);

#ifdef __cplusplus
}
#endif

#endif /* PICOKBSER_H_ */
