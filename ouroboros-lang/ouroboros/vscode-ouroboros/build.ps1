# Build script for Ouroboros VS Code Extension v0.5.0

# Install vsce if not already installed
if (-not (Get-Command npx -ErrorAction SilentlyContinue)) {
    Write-Host "Installing npm..."
    Write-Host "Please install Node.js and npm before running this script."
    exit 1
}

# Create the extension package
Write-Host "Building Ouroboros VS Code Extension v0.5.0..."
npx vsce package

# Rename the package to include version number
$packagePath = Get-ChildItem -Filter *.vsix | Sort-Object -Property LastWriteTime -Descending | Select-Object -First 1
$newName = "ouroboros-language-0.5.0.vsix"
if ($packagePath) {
    Rename-Item -Path $packagePath.Name -NewName $newName -Force
    Write-Host "Renamed package to $newName"
}

Write-Host "Extension package created successfully!"
Write-Host "To install, use:"
Write-Host "1. Open VS Code"
Write-Host "2. Go to Extensions (Ctrl+Shift+X)"
Write-Host "3. Click '...' and select 'Install from VSIX...'"
Write-Host "4. Choose $newName file created by this script" 
