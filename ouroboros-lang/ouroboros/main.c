#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strcmp
#include "lexer.h"
#include "parser.h"
#include "ast_types.h" // For ASTNode, print_ast, free_ast
#include "semantic.h"  // For analyze_program
#include "optimize.h"  // For optimize_ast
#include "vm.h"        // For vm_init, run_vm, vm_cleanup
#include "stdlib.h"    // For register_stdlib_functions
#include "module.h"    // For module_manager_init/cleanup, if used directly

// Function to read file content into a string
char* read_file_to_string(const char* filename) {
    FILE* file = fopen(filename, "rb"); // Open in binary mode to handle line endings correctly for ftell
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed for reading file '%s'\n", filename);
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(buffer, 1, file_size, file);
    if (read_size != (size_t)file_size) { // size_t vs long
        fprintf(stderr, "Error: Failed to read entire file '%s'\n", filename);
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[read_size] = '\0'; // Null-terminate the string
    
    fclose(file);
    return buffer;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename.ouro> [options...]\n", argv[0]);
        // Example options: -print-tokens, -print-ast, -no-optimize, -no-run
        return 1;
    }
    
    const char* filename = argv[1];
    int print_tokens_flag = 0;
    int print_ast_flag = 0;
    int no_optimize_flag = 0;
    int no_run_flag = 0;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-print-tokens") == 0) print_tokens_flag = 1;
        else if (strcmp(argv[i], "-print-ast") == 0) print_ast_flag = 1;
        else if (strcmp(argv[i], "-no-optimize") == 0) no_optimize_flag = 1;
        else if (strcmp(argv[i], "-no-run") == 0) no_run_flag = 1;
    }

    char* source_code = read_file_to_string(filename);
    if (!source_code) {
        return 1; 
    }

    // --- Lexical Analysis ---
    Token* tokens = lex(source_code);
    if (!tokens) {
        fprintf(stderr, "Lexical analysis failed.\n");
        free(source_code);
        return 1;
    }

    if (print_tokens_flag) {
        printf("\n==== Tokens ====\n");
        int i = 0;
        while (tokens[i].type != TOKEN_EOF) {
            printf("Token: Type=%d, Text='%s', Line=%d, Col=%d\n", 
                   tokens[i].type, tokens[i].text, tokens[i].line, tokens[i].col);
            i++;
        }
        printf("Token: Type=EOF, Text='', Line=%d, Col=%d\n", tokens[i].line, tokens[i].col); // Print EOF
    }

    // --- Parsing ---
    ASTNode* ast_root = parse(tokens); // parser.c sets its global `program` to ast_root
    free(tokens); // Tokens are copied into AST or no longer needed after parsing
    
    if (!ast_root) {
        fprintf(stderr, "Parsing failed.\n");
        free(source_code);
        return 1;
    }

    if (print_ast_flag) {
        printf("\n==== Abstract Syntax Tree (Before Optimization) ====\n");
        print_ast(ast_root, 0);
    }

    // --- Semantic Analysis ---
    analyze_program(ast_root); // Populates symbol tables, does basic type checks, etc.
    // check_semantics(ast_root); // Optional second pass for more complex checks

    // --- Optimization ---
    if (!no_optimize_flag) {
        printf("\n\n===============================\n");
        printf("==== OPTIMIZATION STARTING ====\n");
        printf("===============================\n\n");
        optimize_ast(ast_root);
        printf("\n==== OPTIMIZATION COMPLETE ====\n\n");
        if (print_ast_flag) {
            printf("\n==== Abstract Syntax Tree (After Optimization) ====\n");
            print_ast(ast_root, 0);
        }
    } else {
        printf("\n==== Optimization Skipped ====\n");
    }
    
    // --- Execution (VM) ---
    if (!no_run_flag) {
        module_manager_init(); // Initialize module system if used by VM or stdlib
        register_stdlib_functions(); // Make standard library functions available to the VM
        
        vm_init();    // Initialize VM state
        run_vm(ast_root); // Execute the AST
        vm_cleanup(); // Clean up VM state

        module_manager_cleanup(); // Cleanup module system
    } else {
         printf("\n==== Execution Skipped ====\n");
    }

    // --- Cleanup ---
    free_ast(ast_root);
    free(source_code);

    printf("\nCompilation and execution pipeline finished.\n");
    return 0;
}
