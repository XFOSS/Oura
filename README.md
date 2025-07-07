# Oura

This repository contains experimental implementations of the OuroLang programming language.

## Building

The recommended way to compile the project is via Zig:

```bash
zig build
```

This compiles the REPL and all supporting modules using C23/C++23 and links against LLVM. Run the modular demo with:

```bash
zig build mod-run
```

Execute its unit tests using:

```bash
zig build mod-test
```

If Zig is unavailable you can still build the simple interpreter directly:

```bash
g++ -std=c++23 ouro_lang.cc -o ouro_lang
./ouro_lang
```

Legacy CMake files remain for compatibility and can be used instead of Zig:

```bash
mkdir build && cd build
cmake ..
cmake --build . -- -j\$(nproc)
```

## Repository Structure

The source tree contains C and C++ code alongside a `build.zig` script.
Zig's build system is used to compile the interpreter and modules.  A
CMake configuration is kept in `cmake/` for environments that still
depend on it.

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

Experimental AFL++ harnesses live in `tools/fuzz`.  They can be
built with CMake using `-DENABLE_FUZZ=ON` and run against the sample
corpus in `fuzz/corpus` to discover crashes in the lexer and parser.


## Legacy Ouroboros Sources

The `ouroboros-lang` directory tracks the original C implementation of the
language from the `FyreFly-TM/ouroboros-lang` project. Use the provided
Makefile to build the interpreter and run example scripts such as
`test_all_tokens.ouro`. Binary build artifacts and bundled `node_modules`
have been removed from version control.

