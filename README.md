# Oura

This repository contains experimental implementations of the OuroLang programming language.

## Building

### Standalone Interpreter

A simple C++ interpreter is provided in `ouro_lang.cc`. You can build it using g++:

```bash
g++ -std=c++23 ouro_lang.cc -o ouro_lang
```

### Running the REPL

After building, start the interpreter REPL:

```bash
./ouro_lang
```

If built with CMake or Zig, binaries are placed in `build/` and `zig-out/bin`:

```bash
./build/ouro_lang         # from CMake
./zig-out/bin/ourolang_repl  # from Zig
```

### CMake

Configure and build all components with:

```bash
cmake -S . -B build
cmake --build build
```

Executables will appear in `build/`. Use `./build/ouro_lang` to launch the REPL.

### Zig

A Zig build script (`build.zig`) is available:

```bash
zig build
```

Artifacts are placed under `zig-out/bin`. The REPL binary is `zig-out/bin/ourolang_repl`.

## Repository Structure

The project now includes a CMake-based build system, tests, container setup, and documentation. Run `cmake` in a `build` directory to configure and build the modules in `src/`.

## Zig Build with Modules

The `ouro_mod` directory showcases a minimal C++23 module setup. Build and run the modular example with:

```bash
zig build mod-run
```

## Running Examples

The `Ouroboros` C lexer can process the sample programs in `Ouroboros/Ouroboros_Compiler/examples`:

```bash
zig build
./zig-out/bin/ouroboros Ouroboros/Ouroboros_Compiler/examples/hello_world.ouro
```

You can also execute the modular C++ example with:

```bash
zig build mod-run
```

## Testing

After building with CMake run the test suite:

```bash
cd build && ctest
```

Module tests can be executed via Zig:

```bash
zig build mod-test
```

