# Build System

The repository can be built using vanilla CMake or the Zig build
script.  Typical CMake usage:

```bash
mkdir build && cd build
cmake ..
cmake --build . -- -j$(nproc)
ctest --output-on-failure
```

The Zig file `build.zig` mirrors the above configuration and also
exposes custom steps for running the module demo:

```bash
zig build mod-run
zig build mod-test
```

No external dependencies are required; all modules are built from the
checked-in sources.
