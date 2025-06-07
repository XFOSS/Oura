#ifndef OUROBOROS_COMMON_H
#define OUROBOROS_COMMON_H

#include <stdlib.h>
#include <stdio.h>

// Compiler information
#define OUROBOROS_COMPILER_NAME "Ouroboros"
#define OUROBOROS_COMPILER_VERSION "0.1.0"

// Memory allocation wrapper
#define ouro_malloc(size) malloc(size)
#define ouro_free(ptr) free(ptr)

// Error reporting
void report_error(const char* format, ...);
void report_warning(const char* format, ...);

#endif // OUROBOROS_COMMON_H 