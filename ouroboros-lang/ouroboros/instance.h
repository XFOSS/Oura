#ifndef INSTANCE_H
#define INSTANCE_H

#include "class.h"

typedef struct Instance {
    ClassDef *cls;
    SymbolTable *fields;
} Instance;

Instance* create_instance(const char *class_name);
void set_instance_field(Instance *instance, const char *field, const char *value);
const char* get_instance_field(Instance *instance, const char *field);

#endif // INSTANCE_H
