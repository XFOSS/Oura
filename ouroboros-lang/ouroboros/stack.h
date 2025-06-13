#ifndef STACK_H
#define STACK_H

// Maximum number of variables in a stack frame
#define MAX_VARIABLES 64 // Consider making this dynamic or larger for complex functions

// Variable structure (within a stack frame)
typedef struct Variable {
    char name[128];       // Variable name
    char value[1024];     // Variable value (as string). Consider dynamic allocation for large strings/objects.
    // char type_name[64]; // Optionally store type here too, though symbol table is primary
} Variable;

// Stack frame structure
typedef struct StackFrame {
    char name[128];           // Debug name for the frame (e.g., function name, "global", "block")
    char function_name[128];  // Specifically for the function this frame belongs to (if applicable)
    Variable variables[MAX_VARIABLES];
    int var_count;
    // char *return_value; // Return value is now managed globally by vm.c's set_return_value
    struct StackFrame *parent; // Link to the parent (caller's) stack frame
} StackFrame;

// Stack frame functions
StackFrame* create_stack_frame(const char* name, StackFrame *parent);
void destroy_stack_frame(StackFrame *frame);

// Variable management within a frame
void set_variable(StackFrame *frame, const char *name, const char *value);
const char* get_variable(StackFrame *frame, const char *name); // Searches current and parent frames

#endif // STACK_H
