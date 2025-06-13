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

A Zig build script is also included (`build.zig`) for environments with the Zig compiler installed.  It exposes tasks for running the module example and its unit tests.

## Repository Structure

The project now relies solely on Zig for building, testing and container setup. Run `zig build` in the repository root to compile the executables.

## Zig Build with Modules

The `ouro_mod` directory showcases a minimal C++23 module setup. Build and run the modular example with:

```bash
zig build mod-run
```

Run its tests using:

```bash
zig build mod-test
```

## Documentation and Language Specification

Detailed design notes live in the `docs/` directory. The evolving language
specification is provided in `OuroLang_Spec_2_0.md` and outlines syntax,
semantics and concurrency features currently implemented. Community
contributions are welcome—see `docs/contrib.md` for guidelines.

## Fuzzing Tools

Experimental AFL++ harnesses live in `tools/fuzz`.  See that directory for build instructions and an example corpus to discover crashes in the lexer and parser.

