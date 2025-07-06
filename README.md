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

A Zig build script is also included (`build.zig`) for environments with the Zig compiler installed.  It now compiles the C portions of the project using the C23 standard and links against LLVM in addition to `libc`. The script mirrors the basic CMake configuration and exposes tasks for running the module example and its unit tests.

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

## Documentation and Language Specification

Detailed design notes live in the `docs/` directory. The evolving language
specification is provided in `OuroLang_Spec_2_0.md` and outlines syntax,
semantics and concurrency features currently implemented. The specification now
includes a **Types** section describing primitive datatypes and short examples
of control flow. Community contributions are welcome—see `docs/contrib.md` for
guidelines.

## Fuzzing Tools

Experimental AFL++ harnesses live in `tools/fuzz`.  They can be
built with CMake using `-DENABLE_FUZZ=ON` and run against the sample
corpus in `fuzz/corpus` to discover crashes in the lexer and parser.


## Legacy Ouroboros Sources

The `ouroboros-lang` directory tracks the original C implementation of the
language from the `FyreFly-TM/ouroboros-lang` project. Use the provided
Makefile to build the interpreter and run example scripts such as
`test_all_tokens.ouro`. Binary build artifacts and bundled `node_modules`
have been removed from version control.

