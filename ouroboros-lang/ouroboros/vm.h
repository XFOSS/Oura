#ifndef VM_H
#define VM_H

#include "ast_types.h"
#include "stack.h" // For StackFrame

// Property access modifiers (can be used by AST or VM internals if needed)
typedef enum {
    ACCESS_MODIFIER_PUBLIC,
    ACCESS_MODIFIER_PRIVATE,
    ACCESS_MODIFIER_PROTECTED
} AccessModifierEnum;

// Object property structure
typedef struct ObjectProperty {
    char name[128];
    char value[1024]; // Consider dynamic allocation for larger values
    AccessModifierEnum access; // e.g. ACCESS_PUBLIC, ACCESS_PRIVATE
    int is_static;          // 0 for instance, 1 for static
    struct ObjectProperty *next;
} ObjectProperty;

// Object structure
typedef struct Object {
    char class_name[128]; // Format: "ClassName#InstanceID" or "ClassName_static#ID"
    ObjectProperty *properties;
    struct Object *next; // For linked list of all objects
} Object;

// C function pointer type for native functions (if used)
typedef void (*CFunction)();

// External globals (if needed by other modules, e.g., for debugging)
extern Object *objects;
extern char current_class[128]; // Current class context for access checks

// VM initialization and cleanup
void vm_init();
void vm_cleanup();

// Function registration (user-defined from AST)
void register_user_function(ASTNode *func_node); // Takes ASTNode
ASTNode* find_user_function(const char *name, const char* class_context_name); // Find by name and optional class

// Object operations
Object* create_object(const char* class_name); // class_name is base name e.g. "MyClass"
void set_object_property(Object *obj, const char *name, const char *value); // Basic public setter
void set_object_property_with_access(Object *obj, const char *name, const char *value, AccessModifierEnum access, int is_static);
const char* get_object_property(Object *obj, const char *name); // Basic public getter
const char* get_object_property_with_access_check(Object *obj, const char *name, const char *accessing_class_context);
const char* get_static_property(const char *class_name, const char *prop_name); // Gets from ClassName_static object
void free_object(Object *obj);
Object* find_object_by_id(int id);
Object* find_static_class_object(const char *class_name); // Finds/creates ClassName_static object
void initialize_test_class(Object *obj); // Specific initializer, maybe remove/generalize
const char* get_object_property_with_access(Object *obj, const char *property_name, const char *current_class_context_for_access_check);


// VM execution
const char* execute_function_call(const char* qualified_name, ASTNode* args_ast_list, StackFrame* caller_frame);
void run_vm_node(ASTNode *node, StackFrame *frame);
void run_vm(ASTNode *root_ast_node);

// Return value handling
const char* get_return_value();
void set_return_value(const char* value);

// Class method resolution
ASTNode* find_class_method(const char *class_name, const char *method_name);

// Bridge to stdlib built-in functions (defined in stdlib.c)
// Arguments: func_name, list of ASTNodes for args, frame to evaluate args in.
const char* call_built_in_function(const char* func_name_to_call, ASTNode* args_ast_list, StackFrame* frame_for_evaluating_args);

// Add prototype for get_parent_class_name
const char* get_parent_class_name(const char *class_name);

#endif // VM_H
