#ifndef AST_TYPES_H
#define AST_TYPES_H

// Node types for AST
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_CLASS,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_RETURN,
    AST_IF,
    AST_ELSE,
    AST_WHILE,
    AST_FOR,
    AST_BLOCK,
    AST_CALL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_ARRAY,
    AST_IMPORT,
    AST_STRUCT,           // New: struct declaration
    AST_STRUCT_INIT,      // New: struct initialization
    AST_CLASS_METHOD,     // New: class method
    AST_NEW,              // New: 'new' operator for object creation
    AST_MEMBER_ACCESS,    // New: accessing object properties with dot notation
    AST_THIS,             // New: 'this' keyword inside classes
    AST_GENERIC,          // New: generic type parameter <T>
    AST_TYPED_VAR_DECL,   // New: typed variable declaration (int, float, etc.)
    AST_TYPED_FUNCTION,   // New: typed function with return type
    AST_TYPE,             // New: type specifier
    AST_PARAMETER,        // New: function parameter
    AST_STRUCT_FIELD,     // New: struct field
    AST_CLASS_FIELD,      // New: class field
    AST_PRINT,            // Print statement
    AST_INDEX_ACCESS,     // Array index access
    AST_MAP,              // Map literal {key: value}
    AST_TERNARY,          // Ternary conditional ?: operator
    AST_BREAK,            // loop control: break
    AST_CONTINUE,         // loop control: continue
    AST_SUPER,            // 'super' reference inside classes
    AST_UNKNOWN           // Unknown node type
} ASTNodeType;

// AST node structure
typedef struct ASTNode {
    ASTNodeType type;
    char value[256];
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
    
    // Line and column for error reporting
    int line;
    int col;

    // New fields for type information
    char data_type[64];      // Data type (int, float, Vector2D, etc.)
    char generic_type[64];   // Generic type parameter (T, U, etc.)
    int is_void;             // Flag for void functions
    int is_array;            // Flag for array fields/variables
    int array_size;          // Size of array (if specified)
    
    // Access modifiers for object properties
    char access_modifier[16]; // "public", "private", "static"
    char* parent_class_name; // Name of parent class for methods (strdup'd)
} ASTNode;

// Function prototypes
ASTNode* create_node(ASTNodeType type, const char* value, int line, int col); // Updated signature
void print_ast(ASTNode* node, int level);
const char* node_type_to_string(ASTNodeType type);
void free_ast(ASTNode* node);

#endif // AST_TYPES_H
