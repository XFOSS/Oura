#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "module.h"
#include "lexer.h"
#include "parser.h"
#include "ast_types.h"
#include "semantic.h"
#include "ir.h"
#include "vm.h"

// Global module manager
ModuleManager g_module_manager = {NULL, NULL, 0};

// Initialize module manager
void module_manager_init() {
    g_module_manager.modules = NULL;
    g_module_manager.search_paths = NULL;
    g_module_manager.search_path_count = 0;
    
    // Add current directory as default search path
    module_manager_add_search_path(".");
}

// Cleanup module manager
void module_manager_cleanup() {
    // Free all modules
    Module *current = g_module_manager.modules;
    while (current) {
        Module *next = current->next;
        free(current->name);
        free(current->filename);
        if (current->dependencies) {
            free(current->dependencies);
        }
        // Note: AST cleanup should be handled elsewhere
        free(current);
        current = next;
    }
    
    // Free search paths
    for (int i = 0; i < g_module_manager.search_path_count; i++) {
        free(g_module_manager.search_paths[i]);
    }
    free(g_module_manager.search_paths);
    
    g_module_manager.modules = NULL;
    g_module_manager.search_paths = NULL;
    g_module_manager.search_path_count = 0;
}

// Add a search path for modules
void module_manager_add_search_path(const char *path) {
    g_module_manager.search_paths = realloc(g_module_manager.search_paths, 
                                            sizeof(char*) * (g_module_manager.search_path_count + 1));
    g_module_manager.search_paths[g_module_manager.search_path_count] = strdup(path);
    g_module_manager.search_path_count++;
}

// Extract module name from filename
static char* extract_module_name(const char *filename) {
    char *name = strdup(filename);
    char *dot = strrchr(name, '.');
    if (dot) *dot = '\0';  // Remove extension
    
    // Replace path separators with dots
    for (char *p = name; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '.';
        }
    }
    
    return name;
}

// Find a module file in search paths
static char* find_module_file(const char *module_name) {
    char filename[1024];
    struct stat st;
    
    // Try direct file name first
    snprintf(filename, sizeof(filename), "%s.ouro", module_name);
    if (stat(filename, &st) == 0) {
        return strdup(filename);
    }
    
    // Try each search path
    for (int i = 0; i < g_module_manager.search_path_count; i++) {
        snprintf(filename, sizeof(filename), "%s/%s.ouro", 
                 g_module_manager.search_paths[i], module_name);
        if (stat(filename, &st) == 0) {
            return strdup(filename);
        }
        
        // Try with dots replaced by slashes (for hierarchical modules)
        char mod_name[256];
        strncpy(mod_name, module_name, sizeof(mod_name));
        for (char *p = mod_name; *p; p++) {
            if (*p == '.') *p = '/';
        }
        
        snprintf(filename, sizeof(filename), "%s/%s.ouro", 
                 g_module_manager.search_paths[i], mod_name);
        if (stat(filename, &st) == 0) {
            return strdup(filename);
        }
    }
    
    return NULL;
}

// Find an already loaded module
Module* module_find(const char *module_name) {
    Module *current = g_module_manager.modules;
    while (current) {
        if (strcmp(current->name, module_name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Read file content
static char* read_file_content(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    // Read file content
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';
    
    fclose(file);
    return buffer;
}

// Load a module
Module* module_load(const char *module_name) {
    // Check if already loaded
    Module *existing = module_find(module_name);
    if (existing) {
        return existing;
    }
    
    // Find module file
    char *filename = find_module_file(module_name);
    if (!filename) {
        fprintf(stderr, "Error: Module '%s' not found\n", module_name);
        return NULL;
    }
    
    printf("[MODULE] Loading module: %s from %s\n", module_name, filename);
    
    // Create new module
    Module *module = (Module*)calloc(1, sizeof(Module));
    module->name = strdup(module_name);
    module->filename = filename;
    module->is_loaded = 0;
    
    // Add to module list
    module->next = g_module_manager.modules;
    g_module_manager.modules = module;
    
    // Mark as being loaded (prevent circular dependencies)
    module->is_loaded = 1;
    
    // Read and parse the module
    char *source = read_file_content(filename);
    if (!source) {
        return NULL;
    }
    
    // Lex the source
    Token *tokens = lex(source);
    if (!tokens) {
        fprintf(stderr, "Error: Failed to lex module %s\n", module_name);
        free(source);
        return NULL;
    }
    
    /* Parse the source – but preserve the caller's global AST.  The parser
       assigns to the global `program` variable, which we want to keep pointing
       at the *main* compilation unit, not each imported module. */
    extern ASTNode *program;   // declared in parser.c
    ASTNode *prev_program = program;

    module->ast = parse(tokens);

    /* Restore previous global AST so that later compilation stages (e.g.
       field default-initialisation) can still see the main program's classes. */
    if (!module->ast) {
        fprintf(stderr, "Error: Failed to parse module %s\n", module_name);
        free(source);
        program = prev_program; // make sure it's reset even on error
        return NULL;
    }

    program = prev_program;

    // Analyze the module
    analyze_program(module->ast);
    
    free(source);
    
    printf("[MODULE] Successfully loaded module: %s\n", module_name);
    
    return module;
}

// Import a module into another module
int module_import(Module *importer, const char *module_name) {
    Module *imported = module_load(module_name);
    if (!imported) {
        return 0;
    }
    
    // Check for circular dependency
    for (int i = 0; i < importer->dependency_count; i++) {
        if (importer->dependencies[i] == imported) {
            return 1; // Already imported
        }
    }
    
    // Add to dependencies
    importer->dependencies = realloc(importer->dependencies, 
                                    sizeof(Module*) * (importer->dependency_count + 1));
    importer->dependencies[importer->dependency_count++] = imported;
    
    return 1;
}

// Get an exported symbol from a module
ASTNode* module_get_export(Module *module, const char *symbol_name) {
    if (!module || !module->ast) {
        return NULL;
    }
    
    // Search for the symbol in the module's AST
    // This would need to traverse the AST looking for the symbol
    // For now, we'll implement a simple version that looks for functions
    
    ASTNode *current = module->ast;
    while (current) {
        if (current->type == AST_FUNCTION && 
            current->value && 
            strcmp(current->value, symbol_name) == 0) {
            return current;
        }
        
        // Check if this is a program node with functions
        if (current->type == AST_PROGRAM && current->left) {
            ASTNode *func = current->left;
            while (func) {
                if (func->type == AST_FUNCTION && 
                    func->value && 
                    strcmp(func->value, symbol_name) == 0) {
                    return func;
                }
                func = func->next;
            }
        }
        
        current = current->next;
    }
    
    return NULL;
}

// create_node is now defined in ast.c and declared in ast_types.h

// Compile multiple files into a single AST
ASTNode* compile_multiple_files(char **filenames, int file_count) {
    if (file_count == 0) {
        return NULL;
    }
    
    // Initialize module manager if not already done
    if (!g_module_manager.search_paths) {
        module_manager_init();
    }
    
    // Create a root program node
    ASTNode *root = create_node(AST_PROGRAM, "program", 1, 1);
    ASTNode *last_func = NULL;
    
    // Load each file as a module
    for (int i = 0; i < file_count; i++) {
        char *module_name = extract_module_name(filenames[i]);
        Module *module = module_load(module_name);
        free(module_name);
        
        if (!module) {
            fprintf(stderr, "Error: Failed to load file %s\n", filenames[i]);
            continue;
        }
        
        // Add all functions from this module to the root
        if (module->ast && module->ast->type == AST_PROGRAM) {
            ASTNode *func = module->ast->left;
            while (func) {
                ASTNode *next = func->next;
                
                // Clone the function node to add to root
                ASTNode *cloned = create_node(func->type, func->value, func->line, func->col);
                cloned->left = func->left;
                cloned->right = func->right;
                // Copy type information
                strncpy(cloned->data_type, func->data_type, sizeof(cloned->data_type) - 1);
                cloned->data_type[sizeof(cloned->data_type) - 1] = '\0';
                strncpy(cloned->generic_type, func->generic_type, sizeof(cloned->generic_type) - 1);
                cloned->generic_type[sizeof(cloned->generic_type) - 1] = '\0';
                cloned->is_void = func->is_void;
                cloned->is_array = func->is_array;
                cloned->array_size = func->array_size;
                
                // Add to root's function list
                if (!root->left) {
                    root->left = cloned;
                    last_func = cloned;
                } else {
                    last_func->next = cloned;
                    last_func = cloned;
                }
                
                func = next;
            }
        }
    }
    
    return root;
}
