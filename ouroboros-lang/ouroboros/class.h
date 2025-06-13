#ifndef CLASS_H
#define CLASS_H

#include "parser.h"
#include "symbol.h"

typedef struct ClassDef {
    char name[64];
    struct ClassDef *base;
    struct ClassDef *next;
    SymbolTable *fields;
} ClassDef;

extern ClassDef *classes;

void register_class(const char *name);
void register_class_ext(const char *name, const char *base_name);
void define_class_field(const char *class_name, const char *field, const char *value);
const char* get_class_field(const char *class_name, const char *field);

#endif // CLASS_H
