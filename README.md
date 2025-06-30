# Oura

This repository hosts experimental implementations of the OuroLang programming language.

## Building

All components are built through Zig. Run:

```bash
zig build
```

This replaces the old CMake workflow and produces the following targets:

* `ourolang_repl` – the simple interpreter in `ouro_lang.cc`
* `ouro_lang` – the modular interpreter under `src/`
* `ouroboros` – a legacy C compiler preserved for reference
* `ouro_mod` – the modern C++23 module demo

To run the simple interpreter directly with g++ you may also build `ouro_lang.cc` manually:

```bash
g++ -std=c++23 ouro_lang.cc -o ouro_lang
./ouro_lang
```

Additional convenience steps are:

```bash
zig build mod-run   # run the module demo
zig build mod-test  # execute module unit tests
zig build test      # run the basic tests
```

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

