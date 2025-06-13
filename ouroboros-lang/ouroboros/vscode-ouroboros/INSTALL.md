# Installing the Ouroboros VS Code Extension

Follow these steps to install and use the Ouroboros VS Code Extension.

## Prerequisites

- Visual Studio Code installed (1.60.0 or later)
- Node.js and npm installed (if building from source)

## Installation Options

### Option 1: Install from VSIX file

1. Download the `ouroboros-language-0.1.0.vsix` file
2. Open Visual Studio Code
3. Go to Extensions view (Ctrl+Shift+X)
4. Click on the "..." menu in the top-right of the Extensions view
5. Select "Install from VSIX..."
6. Browse to and select the downloaded VSIX file
7. Restart VS Code if prompted

### Option 2: Build and Install from Source

1. Clone or download the extension repository
2. Open a terminal in the extension directory
3. Run the build script:
   ```
   pwsh -File build.ps1
   ```
4. Follow the instructions displayed after building
5. The VSIX file will be created in the extension directory

## Verifying Installation

After installation:

1. Open or create a `.ouro` file
2. Verify that syntax highlighting is working
3. Try typing snippets like `function` or `if` and press Tab to expand them

## Creating a New Project

1. Open the Command Palette (Ctrl+Shift+P)
2. Type "Create New Ouroboros Project" and select it
3. Choose a location for your project
4. Enter a name for your project
5. VS Code will create and open the new project

## Running Ouroboros Programs

1. Open a `.ouro` file in your project
2. Press F5 or use the Run menu to run the current file
3. The extension will automatically compile and run your program

## Support

If you encounter any issues with the extension, please report them on the project's issue tracker. 
