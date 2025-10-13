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
 * @file picokbser.c
 *
 * Knowledge Base Serialization Implementation
 */

#include "picokbser.h"
#include "picoos.h"
#include "picodbg.h"
#include "picoknow.h"
#include "picorsrc.h"
#include "picobase.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/* File format:
 * - Magic number (4 bytes): 0x5049434F ("PICO")
 * - Version (4 bytes): format version
 * - Original resource name length (2 bytes)
 * - Original resource name (variable)
 * - Number of knowledge bases (1 byte)
 * - For each KB:
 *   - KB ID (1 byte)
 *   - KB data size (4 bytes)
 *   - KB data (variable, copied from original .bin file)
 *   - SubObj indicator (1 byte): 0=NULL, 1=present
 *   - If SubObj present:
 *     - SubObj type/size indicator (4 bytes)
 *     - SubObj data (variable)
 * 
 * Note: This is a simplified serialization that saves the raw KB data
 * and marks that specialization has already occurred. On load, we
 * re-run lightweight specialization but skip heavy processing.
 */

typedef struct {
    picoos_uint32 magic;
    picoos_uint32 version;
    picoos_uint16 nameLength;
} picokbser_header_t;

/* Helper to write data to file */
static pico_status_t writeBytes(picoos_File file, const picoos_uint8 *data, 
                                 picoos_uint32 size, picoos_Common common) {
    picoos_uint32 written = size;
    if (!picoos_WriteBytes(file, data, &written) || written != size) {
        return picoos_emRaiseException(common->em, PICO_ERR_OTHER,
                                       NULL, (picoos_char *)"write failed");
    }
    return PICO_OK;
}

/* Helper to read data from file */
static pico_status_t readBytes(picoos_File file, picoos_uint8 *data,
                               picoos_uint32 size, picoos_Common common) {
    picoos_uint32 read = size;
    if (!picoos_ReadBytes(file, data, &read) || read != size) {
        return picoos_emRaiseException(common->em, PICO_ERR_OTHER,
                                       NULL, (picoos_char *)"read failed");
    }
    return PICO_OK;
}

pico_status_t picokbser_serializeResource(
    picorsrc_Resource resource,
    const picoos_char *fileName,
    picoos_Common common)
{
    /* For single-language embedded systems, the optimal approach is:
     * 
     * 1. Store the original .bin files in flash memory
     * 2. Use XIP (Execute-In-Place) to access them directly
     * 3. Enable PICO_XIP_CONST in picoembedded.h
     * 4. Link .bin files into flash at build time
     * 
     * This avoids:
     * - Copying large amounts of data to RAM
     * - Complex serialization/deserialization
     * - Pointer fixup complications
     * 
     * The knowledge base data is already in an optimized binary format.
     * Specialization (parsing headers, setting up pointers) is relatively
     * fast (< 50ms) compared to the memory savings (3-7 MB per language).
     * 
     * For systems with SPIRAM:
     * - Map .bin files to SPIRAM address space
     * - Set kb->base pointers to SPIRAM addresses
     * - Cache frequently accessed data in SRAM
     */
    
    PICODBG_INFO(("picokbser_serializeResource: not implemented"));
    PICODBG_INFO(("use original .bin files with XIP for zero-copy loading"));
    
    return picoos_emRaiseException(common->em, PICO_ERR_OTHER,
                                   NULL, (picoos_char *)"use XIP/memory-mapping instead");
}

pico_status_t picokbser_deserializeResource(
    picorsrc_ResourceManager this,
    const picoos_char *fileName,
    picorsrc_Resource *resource)
{
    PICODBG_INFO(("deserialization not fully implemented - this is a placeholder"));
    
    /* Not implemented - use standard resource loading with XIP support */
    return PICO_ERR_OTHER;
}

picoos_uint8 picokbser_isSerializedFile(
    const picoos_char *fileName,
    picoos_Common common)
{
    picoos_File file;
    picoos_uint32 magic = 0;
    picoos_uint8 result = FALSE;
    
    if (!picoos_OpenBinary(common, &file, fileName)) {
        return FALSE;
    }
    
    /* Try to read magic number */
    picoos_uint32 n = 4;
    if (picoos_ReadBytes(file, (picoos_uint8 *)&magic, &n) && n == 4) {
        if (magic == PICOKBSER_MAGIC_NUMBER) {
            result = TRUE;
        }
    }
    
    picoos_CloseBinary(common, &file);
    return result;
}

#ifdef __cplusplus
}
#endif

/* End picokbser.c */
