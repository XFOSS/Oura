# Ouroboros Documentation Site

A beautiful, modern documentation website for the Ouroboros programming language.

## Features

- 🎨 **Beautiful Design**: Modern, clean interface with smooth animations and gradient accents
- 🌓 **Dark/Light Theme**: Seamless theme switching with persistence
- 📱 **Responsive**: Perfectly optimized for desktop, tablet, and mobile devices
- 🎯 **Syntax Highlighting**: Enhanced Prism.js language definition for all Ouroboros features
- 📋 **Copy Code**: One-click code copying with visual feedback
- 🚀 **Fast Navigation**: Smooth scrolling, active section tracking, and enhanced sidebar
- ⚡ **Performance**: Optimized CSS and JavaScript for fast loading
- 📚 **Comprehensive**: Complete coverage of language features including:
  - Type system with static typing
  - Object-oriented programming with classes
  - Structs for data containers
  - Module system with imports
  - Built-in graphics (OpenGL/Vulkan)
  - Voxel engine support
  - Standard library functions

## Viewing the Documentation

### Local Development
Simply open `index.html` in your web browser:
```bash
# From the docs directory
open index.html      # macOS
start index.html     # Windows
xdg-open index.html  # Linux
```

### Live Server (Recommended)
For the best experience with automatic refresh, use a local web server:

Using Python:
```bash
python -m http.server 8000
# Then open http://localhost:8000
```

Using Node.js:
```bash
npx http-server
# Then open http://localhost:8080
```

Using VS Code Live Server:
1. Install the "Live Server" extension
2. Right-click on `index.html`
3. Select "Open with Live Server"

## File Structure

```
docs/
├── index.html           # Main documentation page with all content
├── styles.css          # Enhanced styling with CSS variables and animations
├── script.js           # Interactive features (theme, navigation, copy code)
├── prism-ouroboros.js  # Custom syntax highlighting for Ouroboros
└── README.md           # This file
```

## Customization

### Adding New Sections
1. Add a new `<section>` in `index.html` with a unique ID:
   ```html
   <section id="new-feature" class="section">
       <h2><i class="fas fa-star"></i> New Feature</h2>
       <!-- Content here -->
   </section>
   ```

2. Add a navigation link in the sidebar:
   ```html
   <li><a href="#new-feature"><i class="fas fa-star"></i> New Feature</a></li>
   ```

### Modifying Themes
Edit the CSS variables in `styles.css`:
- Light theme: `:root` selector
- Dark theme: `[data-theme="dark"]` selector

Key variables:
- `--primary-color`: Main accent color (purple)
- `--secondary-color`: Secondary accent (orange)
- `--bg-color`: Background colors
- `--text-color`: Text colors

### Adding Code Examples
Use the `language-ouroboros` class for syntax highlighting:
```html
<pre><code class="language-ouroboros">
// Your Ouroboros code here
function main() {
    print("Hello, World!");
    return 0;
}
</code></pre>
```

### Creating Output Boxes
For showing program output:
```html
<div class="output-box">
    <div class="output-header">Output:</div>
    <pre>Program output here</pre>
</div>
```

## Browser Support

- Chrome/Edge 90+ (recommended)
- Firefox 88+
- Safari 14+
- Mobile browsers (iOS Safari, Chrome Mobile)

## Features in Detail

### Syntax Highlighting
The custom Prism.js language definition supports:
- All Ouroboros keywords
- Type annotations
- Class and struct declarations
- Built-in functions
- Comments (single and multi-line)
- String literals with escape sequences
- Number literals (integers, floats, hex)
- Operators and punctuation

### Interactive Features
- **Theme Toggle**: Persistent dark/light mode
- **Copy Code**: Hover over code blocks to show copy button
- **Active Navigation**: Current section highlighted in sidebar
- **Smooth Scrolling**: Animated scrolling to sections
- **Responsive Sidebar**: Collapsible on mobile devices

### Content Sections
1. **Introduction**: Overview and key features
2. **Getting Started**: Installation and hello world
3. **Language Syntax**: Comments, keywords, basic structure
4. **Type System**: Static typing, type inference, arrays
5. **Functions**: Typed and untyped functions, recursion
6. **Variables**: Declarations, scope, constants
7. **Control Flow**: If/else, loops, conditionals
8. **Classes & OOP**: Object-oriented programming features
9. **Structs**: Lightweight data structures
10. **Operators**: Arithmetic, comparison, logical
11. **Modules**: Import system and organization
12. **Graphics APIs**: OpenGL, Vulkan, and voxel engine
13. **Standard Library**: Built-in functions
14. **Examples**: Complete code examples
15. **VS Extension**: Editor integration

## Contributing

To contribute to the documentation:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test in multiple browsers
5. Submit a pull request

### Style Guidelines
- Use semantic HTML5 elements
- Follow the existing code structure
- Add comments for complex sections
- Test responsive design
- Ensure accessibility (ARIA labels, alt text)

## Credits

- **Prism.js** - Syntax highlighting library
- **Font Awesome** - Icon library
- **System Fonts** - Optimized typography stack

## License

Part of the Ouroboros Programming Language project.
MIT License - See LICENSE file for details. 
