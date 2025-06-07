// common.h
// Common definitions and utilities used across the Ouroboros compiler components.
// This includes global constants, utility macros, and forward declarations
// for basic types or functions that might be shared.

#ifndef OUROBOROS_COMMON_H
#define OUROBOROS_COMMON_H

#include <stdbool.h> // For bool, true, false
#include <stddef.h>  // For size_t
#include <stdio.h>   // For FILE, fprintf, stderr

// --- Compiler Version Information ---
#define OUROBOROS_COMPILER_VERSION "0.1.0-alpha"
#define OUROBOROS_COMPILER_NAME    "Ouroboros"

// --- Error Reporting ---
// Macro for reporting compilation errors.
// In a real compiler, this would likely be part of a more sophisticated
// error reporting system that collects errors rather than exiting immediately.
#define COMPILER_ERROR(token, format, ...) \
    do { \
        fprintf(stderr, "%s Error at %s:%d:%d: " format "\n", \
                OUROBOROS_COMPILER_NAME, \
                token->source_name ? token->source_name : "UnknownSource", \
                token->line, token->column, ##__VA_ARGS__); \
        /* In a production compiler, you might increment an error count and continue */ \
        /* For this simple example, we'll exit for unrecoverable errors */ \
        exit(EXIT_FAILURE); \
    } while (0)

// --- Memory Management Helpers (Optional but Good Practice) ---
// These could be used to wrap malloc/realloc for basic error checking
// or to integrate with a custom memory allocator.

// Safe malloc that checks for NULL and exits on failure
static inline void* ouro_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Fatal Error: Memory allocation failed for %zu bytes.\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Safe realloc that checks for NULL and exits on failure
static inline void* ouro_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        fprintf(stderr, "Fatal Error: Memory reallocation failed for %zu bytes.\n", size);
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

// Safe strdup that checks for NULL and exits on failure
// (Note: strdup is a POSIX function, so might need _strdup for MSVC or custom impl)
#ifdef _WIN32
#include <string.h> // For _strdup
#define ouro_strdup _strdup
#else
#include <string.h> // For strdup
#define ouro_strdup strdup
#endif

// --- Forward Declarations / Basic Structs if needed ---
// For example, if you had a global error list.
// typedef struct Error {
//     int line;
//     int column;
//     char* message;
// } Error;

#endif // OUROBOROS_COMMON_H