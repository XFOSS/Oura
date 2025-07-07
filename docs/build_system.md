# Build System

The repository uses Zig's build system by default. Simply run

```bash
zig build
```

to compile the interpreter and supporting modules. Additional steps
are available for the module demo:

```bash
zig build mod-run
zig build mod-test
```

No external dependencies are required; all modules are built from the
checked-in sources.

If Zig is unavailable, a CMake configuration remains:

```bash
mkdir build && cd build
cmake ..
cmake --build . -- -j\$(nproc)
ctest --output-on-failure
```
