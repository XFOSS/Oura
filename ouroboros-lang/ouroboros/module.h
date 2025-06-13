#ifndef MODULE_H
#define MODULE_H

#include "ast_types.h"

// Module representation
typedef struct Module {
    char *name;                    // Module name (derived from filename)
    char *filename;                // Full path to the module file
    ASTNode *ast;                  // Parsed AST for this module
    struct Module **dependencies;  // Array of module dependencies
    int dependency_count;          // Number of dependencies
    int is_loaded;                 // Flag to prevent circular loading
    struct Module *next;           // Linked list of modules
} Module;

// Module manager
typedef struct {
    Module *modules;               // Linked list of all loaded modules
    char **search_paths;           // Array of paths to search for modules
    int search_path_count;         // Number of search paths
} ModuleManager;

// Global module manager
extern ModuleManager g_module_manager;

// Module management functions
void module_manager_init();
void module_manager_cleanup();
void module_manager_add_search_path(const char *path);

// Module operations
Module* module_load(const char *module_name);
Module* module_find(const char *module_name);
int module_import(Module *importer, const char *module_name);
ASTNode* module_get_export(Module *module, const char *symbol_name);

// Multi-file compilation
ASTNode* compile_multiple_files(char **filenames, int file_count);

#endif // MODULE_H
