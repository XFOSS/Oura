// Prism.js language definition for Ouroboros (Enhanced)
Prism.languages.ouroboros = {
    'comment': [
        { pattern: /(^|[^\\])\/\*[\s\S]*?(?:\*\/|$)/, lookbehind: true, greedy: true },
        { pattern: /(^|[^\\:])\/\/.*/, lookbehind: true, greedy: true }
    ],
    'string': {
        pattern: /"(?:\\(?:["\\\/bfnrt]|u[0-9a-fA-F]{4})|[^"\\\0-\x1F\x7F]+)*"/,
        greedy: true
    },
    'keyword': /\b(?:let|var|const|fn|function|return|if|else|while|for|class|struct|new|this|extends|super|import|public|private|static|break|continue|print|as|in|is|async|await|yield|enum|interface|implements|package|module|typeof|instanceof|true|false|null)\b/,
    
    // Distinguish built-in types from user-defined types/class names
    'builtin-type': {
        pattern: /\b(?:int|long|float|double|bool|boolean|string|char|void|any|array|object|Vector2|Vector3|Vector4|map)\b/,
        alias: 'class-name' // Style similarly to other types for consistency
    },

    // User-defined types (Classes/Structs) when used as types.
    // Matches: MyType varName, varName: MyType, (param: MyType), : MyType {
    // Needs to be high precedence to avoid being caught as generic identifier.
    'user-defined-type': {
        pattern: /(\b(?:let|var|const|function|fn)\s+\w+\s*:\s*|\b\w+\s*:\s*|\(\s*\w+\s*:\s*|:\s*)[A-Z_][A-Za-z0-9_]*(?:\s*<[^>]+>)?(?=\s*[\w=;,(){]|\s*$)/,
        lookbehind: true,
        greedy: true,
        alias: 'class-name'
    },
    
    // Class/Struct definition names
    'class-name-definition': {
        pattern: /(\b(?:class|struct|enum|interface)\s+)[A-Z_][A-Za-z0-9_]*(?:\s*<[^>]+>)?/,
        lookbehind: true,
        inside: {
            'keyword': /^\b(?:class|struct|enum|interface)\b/,
            'generic-parameter': { // For generics like <T>
                 pattern: /<[^>]+>/,
                 inside: {
                    'user-defined-type': /[A-Z_][A-Za-z0-9_]*/,
                    'punctuation': /[,<>]/
                 }
            },
            'identifier': /[A-Z_][A-Za-z0-9_]*/
        },
        alias: 'class-name'
    },

    'function-definition': {
        pattern: /(\b(?:function|fn)\s+)[a-zA-Z_]\w*/,
        lookbehind: true,
        alias: 'function'
    },
    
    'number': [
        /\b0x[0-9a-fA-F]+([LN])?\b/i, // Hexadecimal with optional L/N suffix
        /\b\d{1,3}(?:(?:_\d{3})+)([LN])?\b/, // Integers with underscores and optional L/N
        /\b\d+(?:\.\d*)?(?:e[+-]?\d+)?[FDLN]?\b/i, // Decimal, float, scientific with optional F/D/L/N
        /\B\.\d+(?:e[+-]?\d+)?[FDLN]?\b/i // .5, .5e-10 with optional F/D/L/N
    ],
    
    'method-or-property': {
        pattern: /(\.)\s*([a-zA-Z_]\w*)/,
        lookbehind: true,
        inside: {
            'punctuation': /^\./,
            'property': { pattern: /[a-zA-Z_]\w*(?!\s*\()/, alias: 'variable' }, // Property if not followed by (
            'method': { pattern: /[a-zA-Z_]\w*(?=\s*\()/, alias: 'function' }   // Method if followed by (
        }
    },
    
    'builtin-function': /\b(?:print|to_string|string_concat|string_length|sqrt|abs|max|min|assert|log|warn|error|opengl_[a-z_]+|vulkan_[a-z_]+|voxel_[a-z_]+|ml_[a-z_]+|init_gui|draw_window|draw_label|draw_button|gui_message_loop|connect_to_server|http_get|register_event|trigger_event|set_timeout)\b/,
    
    'operator': /--?|\+\+?|!=?=?|<=?|>=?|&&?|\|\|?|\?\.|[?*/~^%&|=<>:]|(?<!\w)-(?!\d)/, // Added : as potential operator for ternary or types
    'punctuation': /[{}[\]();,.]/, // Removed : from here, added .
    
    // General identifiers (should be lower precedence than specific types/keywords)
    'identifier': /\b[a-zA-Z_]\w*\b/
};

// Order is important. Specific patterns should come before general ones.
// Re-insert class-name after keyword for cases like 'new MyClass'
Prism.languages.insertBefore('ouroboros', 'identifier', {
    'class-name-usage': { // For `new MyClass`, `extends MyClass`
        pattern: /(\b(?:new|extends)\s+)[A-Z_][A-Za-z0-9_]*(?:\s*<[^>]+>)?/,
        lookbehind: true,
        alias: 'class-name'
    }
});


// Alias for convenience
Prism.languages.ouro = Prism.languages.ouroboros;
