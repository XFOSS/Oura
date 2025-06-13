# Ouroboros IntelliSense Features

The Ouroboros VS Code extension now includes IntelliSense support to help you write code more efficiently. This document explains the features and how to use them.

## Auto-Completion

While typing in a `.ouro` file, you'll get suggestions for:

- **Keywords**: `function`, `if`, `else`, `while`, `for`, `return`, `import`, `let`, `var`
- **Built-in Functions**: All Ouroboros standard library functions including `print`, `to_string`, and all OpenGL/Vulkan functions
- **User-Defined Variables**: Variables you've defined with `let` or `var` in the current file
- **User-Defined Functions**: Functions you've created in the current file

To trigger auto-completion:
- Start typing and it will appear automatically
- Manually trigger with `Ctrl+Space`

## Function Signatures

When typing a function call, you'll get information about the function:
- Function name
- Brief description
- Example usage

## Hover Information

Hover your mouse over any symbol to see:
- For keywords: A description of what the keyword does
- For built-in functions: A description with example usage
- For user-defined variables and functions: Their type and source location

## Document Symbols

The extension scans your code to identify:
- Functions
- Variables
- Import statements

This helps with navigation and understanding code structure.

## Tips for Using IntelliSense

1. **Function Completion**: After selecting a function from the completion list, press `Tab` to include parentheses and position the cursor between them.

2. **Navigation**: Use `F12` to go to definition (for user-defined functions and variables).

3. **Documentation**: Hover over any symbol for quick documentation.

4. **Contextual Suggestions**: IntelliSense prioritizes suggestions based on the current context.

## Limitations

Since this is a simple implementation, there are some limitations:

1. **Cross-File References**: Currently, IntelliSense only works within a single file and doesn't track symbols across multiple files.

2. **Complex Type Information**: The extension doesn't provide detailed type information for variables.

3. **Error Detection**: The extension doesn't provide real-time error detection.

These features may be added in future versions. 
