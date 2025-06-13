# Ouroboros Language Extension for VS Code

This extension provides syntax highlighting, IntelliSense, and project templates for the Ouroboros programming language.

## Features

- **Syntax Highlighting**: Full syntax highlighting for Ouroboros files (`.ouro`)
- **IntelliSense**: Code completion, hover information, and symbol navigation
- **Code Snippets**: Quickly insert common code patterns
- **Project Templates**: Create new Ouroboros projects with a starter template
- **Integrated Compiler**: Includes the Ouroboros compiler (`ouroc.exe`)

## What's New in v0.3.0

- **Typed Functions**: Support for function return types (int, float, string, etc.)
- **Struct Support**: Highlighting and snippets for struct declarations
- **Class Support**: Syntax highlighting for classes, methods, and member access
- **Generics**: Support for generic type parameters like `<T>`
- **New Snippets**: Added snippets for the new language features

## Installation

1. Open Visual Studio Code
2. Go to Extensions (Ctrl+Shift+X)
3. Click on "..." and select "Install from VSIX..."
4. Select the downloaded `.vsix` file

## Usage

### Syntax Highlighting

The extension automatically provides syntax highlighting for any `.ouro` file.

### IntelliSense

While editing Ouroboros files, you'll get:
- **Auto-completion** for keywords, built-in functions, and your own variables/functions
- **Hover information** showing documentation for symbols
- **Context-aware suggestions** as you type

For more details, see [INTELLISENSE.md](./INTELLISENSE.md).

### Creating a New Project

1. Open the Command Palette (Ctrl+Shift+P)
2. Type "Create New Ouroboros Project" and select it
3. Choose a location and name for your project

### Using Snippets

Type any of the following prefixes and press Tab to insert the corresponding snippet:

- `function` - Create a function
- `typedfunction` - Create a typed function with return type
- `void` - Create a void function
- `main` - Create a main function
- `if` - Create an if statement
- `ifelse` - Create an if-else statement
- `while` - Create a while loop
- `for` - Create a for loop
- `import` - Import a module
- `struct` - Create a struct declaration
- `class` - Create a class declaration
- `genericclass` - Create a generic class
- `var` - Create a typed variable declaration
- `new` - Create a new object instance
- `opengl` - Initialize OpenGL
- `shader` - Create shader program
- `renderloop` - Create OpenGL render loop

## Building the Extension

```bash
# Install dependencies
npm install

# Package the extension
./build.ps1
```

## License

This extension is open source. See LICENSE file for details. 
