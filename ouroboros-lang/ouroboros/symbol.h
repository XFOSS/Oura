#ifndef SYMBOL_H
#define SYMBOL_H

#include "parser.h"

#define MAX_SYMBOLS 100

typedef struct {
    char name[64];
    char value[128];
} Symbol;

typedef struct {
    Symbol symbols[MAX_SYMBOLS];
    int count;
} SymbolTable;

SymbolTable* create_symbol_table();
void destroy_symbol_table(SymbolTable *table);
int define_symbol(SymbolTable *table, const char *name, const char *value);
const char* lookup_symbol(SymbolTable *table, const char *name);

#endif // SYMBOL_H
