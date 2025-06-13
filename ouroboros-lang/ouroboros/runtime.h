#ifndef RUNTIME_H
#define RUNTIME_H

#include "symbol.h"

typedef struct StackFrame {
    SymbolTable *locals;
    struct StackFrame *parent;
} StackFrame;

StackFrame* push_frame(StackFrame *parent);
StackFrame* pop_frame(StackFrame *frame);
void define_local(StackFrame *frame, const char *name, const char *value);
const char* get_local(StackFrame *frame, const char *name);

#endif // RUNTIME_H
