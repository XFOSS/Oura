# Ouroboros Project

This is a template project for the Ouroboros programming language.

## Getting Started

This project includes:
- `main.ouro` - The main entry point for your application
- `graphics.ouro` - Example graphics code using OpenGL
- `utils.ouro` - Utility functions for common tasks
- `ouroc.exe` - The Ouroboros compiler

## How to Run

To run your Ouroboros project:

```bash
# Compile and run
./ouroc.exe main.ouro
```

## Project Structure

- **main.ouro**: Main entry point with the `main()` function
- **graphics.ouro**: Example OpenGL graphics code
- **utils.ouro**: Utility functions you can import in your project

## Examples

To use the graphics example, uncomment the `draw_triangle()` function call in graphics.ouro or import it in your main.ouro file:

```ouroboros
import "graphics";

function main() {
    print("Starting graphics demo");
    draw_triangle();
    return 0;
}
```

To use utility functions:

```ouroboros
import "utils";

function main() {
    result = add(5, 3);
    print("5 + 3 = " + result);
    return 0;
}
```

## Documentation

For more information about the Ouroboros language, check the official documentation. 
