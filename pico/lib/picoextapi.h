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
 * @file picoextapi.h
 *
 * API extensions for development use
 *
 * Copyright (C) 2008-2009 SVOX AG, Baslerstr. 30, 8048 Zuerich, Switzerland
 * All rights reserved.
 *
 * History:
 * - 2009-04-20 -- initial version
 *
 */

#ifndef PICOEXTAPI_H_
#define PICOEXTAPI_H_

#include "picodefs.h"
#include "picodbg.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif


/* ****************************************************************************/
/* Things that might be added to picoapi later but should not appear there    */
/* for the time being                                                         */
/* ****************************************************************************/

/* String type for Unicode text input *****************************************/

/* Unicode encodings supported by PICO. */

#define PICO_STRENC_UTF8    0
#define PICO_STRENC_UTF16   1

/* An UTF-8 string must point to a byte array, terminated by a null character
   ('\0'). An UTF-16 string must point to a contiguous array of 16-bit units
   (in native byte ordering), terminated by a 0. */

typedef char        *PICO_STRING_UTF8;
typedef pico_Uint16 *PICO_STRING_UTF16;

/* Generic pointer to a Unicode string, encoded either as UTF-8 or UTF-16.
   The application must make sure that for each 'PICO_STRING_PTR' it provides
   an argument of type 'PICO_STRING_UTF8' or 'PICO_STRING_UTF16' (or of a type
   compatible to one of these types). */

typedef void *PICO_STRING_PTR;


/* ****************************************************************************/
/* System-level API functions                                                 */
/* ****************************************************************************/

/* System initialization and termination functions ****************************/

/* Same as pico_initialize, but allows to enable memory protection
   functionality for testing purposes (enableMemProt != 0). */

PICO_FUNC picoext_initialize(
        void *memory,
        const pico_Uint32 size,
        pico_Int16 enableMemProt,
        pico_System *outSystem
        );


/* System and lingware inspection functions ***********************************/

/* Returns version information of the current Pico engine. */

PICO_FUNC picoext_getVersionInfo(
        pico_Retstring outInfo,
        const pico_Int16 outInfoMaxLen
    );

/* Returns unique resource name */

/*
PICO_FUNC picoext_getResourceName(
        pico_Resource resource,
        pico_Retstring outInfo
    );
*/

/* Debugging/testing support functions *****************************************/

/* Sets tracing level. Increasing amounts of information is displayed
   at each level. */

PICO_FUNC picoext_setTraceLevel(
        pico_System system,
        pico_Int32 level
        );

/* Sets trace filtering. Limits tracing output to tracing information
   resulting from the source file name being filtered. */

PICO_FUNC picoext_setTraceFilterFN(
        pico_System system,
        const pico_Char *name
        );

/* Enables logging of debug output to log file 'name'. If 'name' is NULL
   or an empty string, logging is disabled. */

PICO_FUNC picoext_setLogFile(
        pico_System system,
        const pico_Char *name
        );


/* Memory usage ***************************************************************/

PICO_FUNC picoext_getSystemMemUsage(
        pico_System system,
        pico_Int16 resetIncremental,
        pico_Int32 *outUsedBytes,
        pico_Int32 *outIncrUsedBytes,
        pico_Int32 *outMaxUsedBytes
        );

PICO_FUNC picoext_getEngineMemUsage(
        pico_Engine engine,
        pico_Int16 resetIncremental,
        pico_Int32 *outUsedBytes,
        pico_Int32 *outIncrUsedBytes,
        pico_Int32 *outMaxUsedBytes
        );

PICO_FUNC picoext_getLastScheduledPU(
        pico_Engine engine
        );

PICO_FUNC picoext_getLastProducedItemType(
        pico_Engine engine
        );

/* *** Extended Resource Loading Functions (for embedded systems) *************/

/**
   Loads a resource from a memory buffer instead of a file.
   This is useful for embedded systems that store language data in flash
   memory or want to use XIP (Execute-In-Place) to avoid copying data to RAM.
   
   The memory buffer must contain a complete, valid Pico resource file
   (same format as .bin files). The buffer must remain valid for the
   lifetime of the resource.
   
   @param system - Pico system handle
   @param memoryBuffer - Pointer to resource data in memory (must remain valid)
   @param bufferSize - Size of the resource data in bytes
   @param resourceName - Name to assign to this resource
   @param outResource - Pointer to receive the loaded resource handle
   @return PICO_OK on success, error code otherwise
   
   Example usage for ESP32 with embedded language files:
   @code
   extern const uint8_t en_us_ta_start[] asm("_binary_en_US_ta_bin_start");
   extern const uint8_t en_us_ta_end[]   asm("_binary_en_US_ta_bin_end");
   
   pico_Resource taRes;
   picoext_loadResourceFromMemory(system, en_us_ta_start,
                                  en_us_ta_end - en_us_ta_start,
                                  "en-US-ta", &taRes);
   @endcode
*/
PICO_FUNC picoext_loadResourceFromMemory(
        pico_System system,
        const void *memoryBuffer,
        const pico_Uint32 bufferSize,
        const pico_Char *resourceName,
        pico_Resource *outResource
        );

#ifdef __cplusplus
}
#endif


#endif /* PICOEXTAPI_H_ */
