#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "instance.h"

extern ClassDef *classes;

Instance* create_instance(const char *class_name) {
    ClassDef *cls = classes;
    while (cls) {
        if (strcmp(cls->name, class_name) == 0) {
            Instance *inst = malloc(sizeof(Instance));
            inst->cls = cls;
            inst->fields = create_symbol_table();
            printf("[INSTANCE] Created instance of: %s\n", class_name);
            return inst;
        }
        cls = cls->next;
    }
    printf("[INSTANCE] Class not found: %s\n", class_name);
    return NULL;
}

void set_instance_field(Instance *instance, const char *field, const char *value) {
    define_symbol(instance->fields, field, value);
    printf("[INSTANCE] %s.%s = %s\n", instance->cls->name, field, value);
}

const char* get_instance_field(Instance *instance, const char *field) {
    const char *val = lookup_symbol(instance->fields, field);
    if (val) return val;
    return get_class_field(instance->cls->name, field);
}
