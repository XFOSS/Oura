#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "symbol.h"

// Create a new symbol table
SymbolTable* create_symbol_table() {
    SymbolTable* table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!table) {
        fprintf(stderr, "Error: Memory allocation failed for symbol table\n");
        return NULL;
    }
    
    table->count = 0;
    return table;
}

// Destroy a symbol table and free memory
void destroy_symbol_table(SymbolTable* table) {
    if (table) {
        free(table);
    }
}

// Define a symbol in the table
int define_symbol(SymbolTable* table, const char* name, const char* value) {
    if (!table || !name || !value) return 0;
    
    // Check if the symbol already exists
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            // Update existing symbol
            strncpy(table->symbols[i].value, value, sizeof(table->symbols[i].value) - 1);
            table->symbols[i].value[sizeof(table->symbols[i].value) - 1] = '\0';
            return 1;
        }
    }
    
    // Add new symbol if there's space
    if (table->count < MAX_SYMBOLS) {
        strncpy(table->symbols[table->count].name, name, sizeof(table->symbols[table->count].name) - 1);
        table->symbols[table->count].name[sizeof(table->symbols[table->count].name) - 1] = '\0';
        
        strncpy(table->symbols[table->count].value, value, sizeof(table->symbols[table->count].value) - 1);
        table->symbols[table->count].value[sizeof(table->symbols[table->count].value) - 1] = '\0';
        
        table->count++;
        return 1;
    } else {
        fprintf(stderr, "Error: Symbol table limit reached\n");
        return 0;
    }
}

// Look up a symbol in the table
const char* lookup_symbol(SymbolTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return table->symbols[i].value;
        }
    }
    
    return NULL; // Symbol not found
}
