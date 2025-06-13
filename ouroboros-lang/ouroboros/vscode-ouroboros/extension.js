const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

// Language features
const KEYWORDS = [
    'function', 'if', 'else', 'while', 'for', 'return', 'import', 'let', 'var'
];

const BUILT_IN_FUNCTIONS = [
    'print', 'to_string', 'string_concat', 'string_length',
    'opengl_init', 'opengl_create_context', 'opengl_create_shader', 'opengl_use_shader',
    'opengl_create_buffer', 'opengl_bind_buffer', 'opengl_buffer_data', 'opengl_draw_arrays',
    'opengl_clear', 'opengl_swap_buffers', 'opengl_destroy_context', 'opengl_is_context_valid',
    'vulkan_init', 'vulkan_create_instance', 'vulkan_select_physical_device',
    'vulkan_create_logical_device', 'vulkan_create_surface', 'vulkan_create_swapchain',
    'vulkan_create_render_pass', 'vulkan_create_graphics_pipeline', 'vulkan_draw_frame'
];

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
    console.log('Ouroboros Language Extension activated');

    // Register the createProject command
    let createProjectCommand = vscode.commands.registerCommand('ouroboros.createProject', async function () {
        // Ask for project location
        const projectFolderUri = await vscode.window.showOpenDialog({
            canSelectFiles: false,
            canSelectFolders: true,
            canSelectMany: false,
            openLabel: 'Select Project Location'
        });

        if (!projectFolderUri || projectFolderUri.length === 0) {
            return;
        }

        // Ask for project name
        const projectName = await vscode.window.showInputBox({
            prompt: 'Enter project name',
            placeHolder: 'MyOuroborosProject'
        });

        if (!projectName) {
            return;
        }

        const projectPath = path.join(projectFolderUri[0].fsPath, projectName);

        // Create project folder
        try {
            if (!fs.existsSync(projectPath)) {
                fs.mkdirSync(projectPath);
            }

            // Copy project template
            const templatePath = path.join(context.extensionPath, 'templates', 'project');
            copyDirectory(templatePath, projectPath);

            // Copy compiler
            const compilerPath = path.join(context.extensionPath, 'templates', 'project', 'ouroc.exe');
            if (fs.existsSync(compilerPath)) {
                fs.copyFileSync(compilerPath, path.join(projectPath, 'ouroc.exe'));
            }

            // Open the project in VS Code
            vscode.commands.executeCommand('vscode.openFolder', vscode.Uri.file(projectPath));

            vscode.window.showInformationMessage(`Ouroboros project "${projectName}" created successfully!`);
        } catch (err) {
            vscode.window.showErrorMessage(`Error creating project: ${err.message}`);
        }
    });

    context.subscriptions.push(createProjectCommand);

    // Register completion provider for IntelliSense
    const completionProvider = vscode.languages.registerCompletionItemProvider('ouroboros', {
        provideCompletionItems(document, position, token, context) {
            const completionItems = [];

            // Add keywords
            KEYWORDS.forEach(keyword => {
                const item = new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Keyword);
                item.detail = 'Ouroboros keyword';
                completionItems.push(item);
            });

            // Add built-in functions
            BUILT_IN_FUNCTIONS.forEach(funcName => {
                const item = new vscode.CompletionItem(funcName, vscode.CompletionItemKind.Function);
                item.detail = 'Ouroboros built-in function';
                
                // Add documentation for common functions
                if (funcName === 'print') {
                    item.documentation = new vscode.MarkdownString('Prints a value to the console.\n\n```ouroboros\nprint("Hello, World!");\n```');
                } else if (funcName === 'to_string') {
                    item.documentation = new vscode.MarkdownString('Converts a value to a string.\n\n```ouroboros\nstr = to_string(42);\n```');
                } else if (funcName.startsWith('opengl_')) {
                    item.documentation = new vscode.MarkdownString(`OpenGL function: ${funcName}`);
                } else if (funcName.startsWith('vulkan_')) {
                    item.documentation = new vscode.MarkdownString(`Vulkan function: ${funcName}`);
                }
                
                completionItems.push(item);
            });

            // Scan the document for variable and function names
            const text = document.getText();
            const varRegex = /(?:let|var)\s+(\w+)\s*=/g;
            const funcRegex = /function\s+(\w+)\s*\(/g;
            
            let match;
            while ((match = varRegex.exec(text))) {
                const varName = match[1];
                const item = new vscode.CompletionItem(varName, vscode.CompletionItemKind.Variable);
                item.detail = 'Variable';
                completionItems.push(item);
            }
            
            while ((match = funcRegex.exec(text))) {
                const funcName = match[1];
                const item = new vscode.CompletionItem(funcName, vscode.CompletionItemKind.Function);
                item.detail = 'Function';
                item.insertText = new vscode.SnippetString(`${funcName}($1)`);
                completionItems.push(item);
            }

            return completionItems;
        }
    });

    // Register hover provider for documentation
    const hoverProvider = vscode.languages.registerHoverProvider('ouroboros', {
        provideHover(document, position, token) {
            const range = document.getWordRangeAtPosition(position);
            const word = document.getText(range);
            
            // Check if it's a keyword
            if (KEYWORDS.includes(word)) {
                return new vscode.Hover(`**${word}** - Ouroboros keyword`);
            }
            
            // Check if it's a built-in function
            if (BUILT_IN_FUNCTIONS.includes(word)) {
                let documentation = `**${word}** - Ouroboros built-in function`;
                
                // Add more detailed documentation for common functions
                if (word === 'print') {
                    documentation += '\n\n```ouroboros\nprint(value);\n```\n\nPrints a value to the console. The value can be a string, number, or variable.';
                } else if (word === 'to_string') {
                    documentation += '\n\n```ouroboros\nto_string(value);\n```\n\nConverts a value to a string.';
                } else if (word.startsWith('opengl_')) {
                    documentation += `\n\nOpenGL function for graphics operations.`;
                }
                
                return new vscode.Hover(new vscode.MarkdownString(documentation));
            }
            
            return null;
        }
    });

    context.subscriptions.push(completionProvider, hoverProvider);
}

/**
 * Copy a directory recursively
 * @param {string} source - Source directory
 * @param {string} destination - Destination directory
 */
function copyDirectory(source, destination) {
    // Create destination directory if it doesn't exist
    if (!fs.existsSync(destination)) {
        fs.mkdirSync(destination);
    }

    // Get all files and directories in source
    const entries = fs.readdirSync(source, { withFileTypes: true });

    for (const entry of entries) {
        const sourcePath = path.join(source, entry.name);
        const destPath = path.join(destination, entry.name);

        // If directory, recursively copy
        if (entry.isDirectory()) {
            copyDirectory(sourcePath, destPath);
        } else {
            // Copy file
            fs.copyFileSync(sourcePath, destPath);
        }
    }
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
}; 
