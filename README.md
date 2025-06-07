# Oura

This repository contains experimental implementations of the OuroLang programming language.

## Building

A simple C++ interpreter is provided in `ouro_lang.cc`. You can build it using g++:

```bash
g++ -std=c++23 ouro_lang.cc -o ouro_lang
```

Then run the REPL:

```bash
./ouro_lang
```

A Zig build script is also included (`build.zig`) for environments with the Zig compiler installed.

## Repository Structure

The project now includes a CMake-based build system, tests, container setup, and documentation. Run `cmake` in a `build` directory to configure and build the modules in `src/`.

## Zig Build with Modules

The `ouro_mod` directory showcases a minimal C++23 module setup. Build and run the modular example with:

```bash
zig build mod-run
```

Run its tests using:

```bash
zig build mod-test
```

