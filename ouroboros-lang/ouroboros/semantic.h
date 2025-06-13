#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast_types.h"

// Maximum number of symbols in a single scope
#define MAX_SCOPE_SYMBOLS 100
// Maximum scope depth (nesting)
#define MAX_SCOPE_DEPTH 50

// Symbol Kinds
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_CLASS,
    SYMBOL_STRUCT,
    SYMBOL_PARAMETER,
    SYMBOL_TYPE // For type aliases, if any, or just to note a type name
} SymbolKind;

// Symbol structure
typedef struct Symbol {
    char name[128];
    SymbolKind kind;
    char type_name[64];       // Data type name (e.g., "int", "MyClass")
    ASTNode* declaration_node; // Pointer to the AST node where it was declared
    int scope_level;          // Scope level where defined
    // Add other attributes as needed: const, static, visibility, etc.
} Symbol;

// Scope structure (part of the SymbolTable)
typedef struct Scope {
    Symbol symbols[MAX_SCOPE_SYMBOLS];
    int symbol_count;
    struct Scope* parent_scope; // Enclosing scope
    int level;                // Nesting level (0 for global)
    char scope_name[128];     // e.g., function name, class name, "block"
} Scope;

// Symbol Table structure (manages a stack of scopes)
typedef struct SymbolTable {
    Scope* scope_stack[MAX_SCOPE_DEPTH];
    int current_scope_idx; // Points to the top of the stack, -1 if empty
    int next_scope_level_to_assign;
} SymbolTable;

// Symbol Table functions (prototypes)
SymbolTable* symbol_table_create();
void symbol_table_destroy(SymbolTable* st);
void symbol_table_enter_scope(SymbolTable* st, const char* scope_name);
void symbol_table_exit_scope(SymbolTable* st);
int symbol_table_add_symbol(SymbolTable* st, const char* name, SymbolKind kind, const char* type_name, ASTNode* decl_node);
Symbol* symbol_table_lookup_current_scope(SymbolTable* st, const char* name);
Symbol* symbol_table_lookup_all_scopes(SymbolTable* st, const char* name);
Scope* symbol_table_get_current_scope(SymbolTable* st);


// Analysis functions
void analyze_program(ASTNode *ast); // Takes the root AST node
void check_semantics(ASTNode *ast);  // Placeholder for more detailed checks

#endif // SEMANTIC_H
