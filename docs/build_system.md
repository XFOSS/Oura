# Build System

The repository is built using Zig.  Compile all artifacts with:

```bash
zig build
```

The `build.zig` file also exposes custom steps for running the module
demo and its unit tests:

```bash
zig build mod-run
zig build mod-test
```

No external dependencies are required; all modules are built from the
checked-in sources.
