#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "class.h"



typedef struct Instance {
    ClassDef *cls;
    SymbolTable *fields;
} Instance;

ClassDef *classes = NULL;

void register_class(const char *name) {
    register_class_ext(name, NULL);
}

void register_class_ext(const char *name, const char *base_name) {
    ClassDef *cls = malloc(sizeof(ClassDef));
    strncpy(cls->name, name, sizeof(cls->name));
    cls->fields = create_symbol_table();
    cls->base = NULL;

    if (base_name) {
        ClassDef *base = classes;
        while (base) {
            if (strcmp(base->name, base_name) == 0) {
                cls->base = base;
                break;
            }
            base = base->next;
        }
    }

    cls->next = classes;
    classes = cls;

    printf("[CLASS] Registered: %s", name);
    if (base_name) printf(" : %s", base_name);
    printf("\n");
}

void define_class_field(const char *class_name, const char *field, const char *value) {
    ClassDef *cls = classes;
    while (cls) {
        if (strcmp(cls->name, class_name) == 0) {
            define_symbol(cls->fields, field, value);
            printf("[CLASS] %s.%s = %s\n", class_name, field, value);
            return;
        }
        cls = cls->next;
    }
}

const char* get_class_field(const char *class_name, const char *field) {
    ClassDef *cls = classes;
    while (cls) {
        if (strcmp(cls->name, class_name) == 0) {
            const char *val = lookup_symbol(cls->fields, field);
            if (val) return val;
            if (cls->base) return get_class_field(cls->base->name, field);
        }
        cls = cls->next;
    }
    return NULL;
}
