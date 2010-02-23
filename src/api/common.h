/*
 * Copyright 2010, Greg Olszewski and Lloyd Hilaiel.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 *  3. Neither the name of Greg Olszewski and Lloyd Hilaiel nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef __ORDERLY_COMMON_H__
#define __ORDERLY_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif    

#define ORDERLY_MAX_DEPTH 128

/* msft dll export gunk.  To build a DLL on windows, you
 * must define WIN32, ORDERLY_SHARED, and ORDERLY_BUILD.  To use a shared
 * DLL, you must define ORDERLY_SHARED and WIN32 */
#if defined(WIN32) && defined(ORDERLY_SHARED)
#  ifdef ORDERLY_BUILD
#    define ORDERLY_API __declspec(dllexport)
#  else
#    define ORDERLY_API __declspec(dllimport)
#  endif
#else
#  define ORDERLY_API
#endif 

/** pointer to a malloc function, supporting client overriding memory
 *  allocation routines */
typedef void * (*orderly_malloc_func)(void *ctx, unsigned int sz);

/** pointer to a free function, supporting client overriding memory
 *  allocation routines */
typedef void (*orderly_free_func)(void *ctx, void * ptr);

/** pointer to a realloc function which can resize an allocation. */
typedef void * (*orderly_realloc_func)(void *ctx, void * ptr, unsigned int sz);

/** A structure which can be passed to orderly_*_alloc routines to allow the
 *  client to specify memory allocation functions to be used. */
typedef struct
{
    /** pointer to a function that can allocate uninitialized memory */
    orderly_malloc_func malloc;
    /** pointer to a function that can resize memory allocations */
    orderly_realloc_func realloc;
    /** pointer to a function that can free memory allocated using
     *  reallocFunction or mallocFunction */
    orderly_free_func free;
    /** a context pointer that will be passed to above allocation routines */
    void * ctx;
} orderly_alloc_funcs;

typedef enum {
    ORDERLY_UNKNOWN = 0,
    ORDERLY_JSONSCHEMA = 1,
    ORDERLY_TEXTUAL = 2
} orderly_format;
    

#ifdef __cplusplus
}
#endif    

#endif
