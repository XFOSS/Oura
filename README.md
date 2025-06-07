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
