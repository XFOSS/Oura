#include <stdlib.h>
#include <string.h>
#include "runtime.h"

StackFrame* push_frame(StackFrame *parent) {
    StackFrame *frame = malloc(sizeof(StackFrame));
    frame->locals = create_symbol_table();
    frame->parent = parent;
    return frame;
}

StackFrame* pop_frame(StackFrame *frame) {
    StackFrame *parent = frame->parent;
    destroy_symbol_table(frame->locals);
    free(frame);
    return parent;
}

void define_local(StackFrame *frame, const char *name, const char *value) {
    define_symbol(frame->locals, name, value);
}

const char* get_local(StackFrame *frame, const char *name) {
    StackFrame *current = frame;
    while (current) {
        const char *val = lookup_symbol(current->locals, name);
        if (val) return val;
        current = current->parent;
    }
    return NULL;
}
