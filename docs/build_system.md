# Build System

The entire repository is built using Zig.  The `build.zig` script now
replaces the previous CMake setup and compiles every component:

* `ourolang_repl` – the simple interpreter
* `ouro_lang` – the modular interpreter built from `src/`
* `ouroboros` – the historical C compiler
* `ouro_mod` – the modern module demo

Invoke the build with:

```bash
zig build
```

Additional convenience steps are available:

```bash
zig build mod-run
zig build mod-test
zig build test       # run the basic unit tests
```

No external dependencies are required; all modules are built from the
checked-in sources.
