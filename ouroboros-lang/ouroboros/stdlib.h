#ifndef STDLIB_H
#define STDLIB_H

// No direct ASTNode dependencies needed for the public API of stdlib itself.
// The VM handles ASTNode evaluation before calling stdlib functions.

void register_stdlib_functions();
// Changed name to avoid potential conflict if vm.c's internal call_built_in_function was ever exposed differently.
// This is the function that stdlib.c implements and vm.c calls.
int call_builtin_function_impl(const char *name, const char **args, int arg_count); 
void set_call_args(const char **args, int count); // Used internally by stdlib.c wrappers

#endif // STDLIB_H
