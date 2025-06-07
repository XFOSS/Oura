# Build System

The repository is built entirely using Zig.  The `build.zig` script
handles compilation of the REPL, legacy tools and the modular
demonstration.

Invoke the build with:

```bash
zig build
```

Additional convenience steps are available:

```bash
zig build mod-run
zig build mod-test
```

No external dependencies are required; all modules are built from the
checked-in sources.
