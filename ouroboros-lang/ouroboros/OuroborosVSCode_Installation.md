# Ouroboros VS Code Extension

This document provides instructions for building and installing the Ouroboros language extension for Visual Studio Code.

## What's Included

The extension provides:
- Syntax highlighting for `.ouro` files
- Code snippets for Ouroboros language constructs
- Project templates with examples
- Integration with the Ouroboros compiler (ouroc.exe)

## Building the Extension

1. Make sure you have Node.js and npm installed
2. Install the VSCE packaging tool:
   ```
   npm install -g @vscode/vsce
   ```
3. Navigate to the `vscode-ouroboros` directory
4. Run the build script:
   ```
   pwsh -File build.ps1
   ```
   This will create a `.vsix` file in the directory.

## Installing the Extension

1. Open Visual Studio Code
2. Go to Extensions (Ctrl+Shift+X)
3. Click the "..." menu in the top right corner
4. Select "Install from VSIX..."
5. Navigate to and select the `.vsix` file created in the build step
6. Restart VS Code when prompted

## Using the Extension

### Creating a New Project

1. Open the Command Palette (Ctrl+Shift+P)
2. Type "Create New Ouroboros Project" and select it
3. Choose a location for your project
4. Enter a name for your project

### Using Code Snippets

The extension provides several snippets for common Ouroboros patterns:
- `function` - Create a function
- `main` - Create a main function
- `if` - Create an if statement
- `while` - Create a while loop
- `opengl` - Initialize OpenGL context

Type the snippet prefix and press Tab to expand it.

### Running Your Code

1. Open a `.ouro` file
2. Press F5 or use the Run menu
3. The extension will compile and run your Ouroboros program

## Troubleshooting

If you encounter issues:
1. Ensure ouroc.exe is in the project directory
2. Check the output console for error messages
3. Make sure you have the necessary dependencies installed

## Manual Installation of Files

If you prefer to manually set up the extension:

1. Copy all files from the `vscode-ouroboros` directory to:
   - Windows: `%USERPROFILE%\.vscode\extensions\ouroboros-language-0.1.0`
   - Mac/Linux: `~/.vscode/extensions/ouroboros-language-0.1.0`
2. Restart VS Code

## Project Structure

- `syntaxes/` - Contains the syntax highlighting definitions
- `snippets/` - Contains code snippets
- `templates/project/` - Contains the project template with the compiler 
