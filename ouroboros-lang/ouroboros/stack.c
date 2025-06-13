#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

StackFrame* create_stack_frame(const char* name, StackFrame* parent) {
    StackFrame* frame = (StackFrame*)calloc(1, sizeof(StackFrame)); // Use calloc
    if (!frame) {
        fprintf(stderr, "Error: Memory allocation failed for stack frame '%s'\n", name);
        // In a real application, might try to recover or throw a more specific error.
        exit(EXIT_FAILURE); // For simplicity, exit on critical alloc failure
    }
    
    strncpy(frame->name, name, sizeof(frame->name) - 1);
    frame->name[sizeof(frame->name) - 1] = '\0';
    
    // If 'name' is a function call, function_name can be the same.
    // If it's a block, function_name might be inherited or set to the enclosing function.
    // For now, simple copy.
    strncpy(frame->function_name, name, sizeof(frame->function_name) - 1);
    frame->function_name[sizeof(frame->function_name) - 1] = '\0';
    
    frame->parent = parent;
    // var_count is 0 due to calloc
    
    return frame;
}

void destroy_stack_frame(StackFrame* frame) {
    if (frame) {
        // Variables are stack-allocated within the frame struct, so no individual free needed for them
        // unless their `value` field becomes dynamically allocated.
        free(frame);
    }
}

void set_variable(StackFrame* frame, const char* name, const char* value) {
    if (!frame || !name || !value) {
        // fprintf(stderr, "Warning: Attempt to set variable with null frame, name, or value.\n");
        return;
    }
    
    // Try to update existing variable in the current frame only
    for (int i = 0; i < frame->var_count; i++) {
        if (strcmp(frame->variables[i].name, name) == 0) {
            strncpy(frame->variables[i].value, value, sizeof(frame->variables[i].value) - 1);
            frame->variables[i].value[sizeof(frame->variables[i].value) - 1] = '\0';
            return;
        }
    }
    
    // If not found in current frame, add as a new variable in the current frame
    if (frame->var_count < MAX_VARIABLES) {
        strncpy(frame->variables[frame->var_count].name, name, sizeof(frame->variables[frame->var_count].name) - 1);
        frame->variables[frame->var_count].name[sizeof(frame->variables[frame->var_count].name) - 1] = '\0';
        
        strncpy(frame->variables[frame->var_count].value, value, sizeof(frame->variables[frame->var_count].value) - 1);
        frame->variables[frame->var_count].value[sizeof(frame->variables[frame->var_count].value) - 1] = '\0';
        
        frame->var_count++;
    } else {
        fprintf(stderr, "Error: Stack frame '%s' variable limit (%d) reached when setting '%s'.\n", frame->name, MAX_VARIABLES, name);
        // This is a critical runtime error.
    }
}

const char* get_variable(StackFrame* frame, const char* name) {
    if (!name) return NULL; // Or "undefined"

    StackFrame* current_frame_iter = frame;
    while (current_frame_iter) {
        for (int i = 0; i < current_frame_iter->var_count; i++) {
            if (strcmp(current_frame_iter->variables[i].name, name) == 0) {
                return current_frame_iter->variables[i].value;
            }
        }
        current_frame_iter = current_frame_iter->parent; // Go to parent frame
    }
    
    return NULL; // Variable not found in this frame or any parent frames
}
