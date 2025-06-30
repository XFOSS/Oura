# Testing

The repository includes a tiny suite of unit tests and an optional
fuzzing harness.

### Unit tests

```
zig build mod-test  # tests for the modular demo
zig build test      # basic interpreter tests
```
These replace the old `ctest` invocation from the removed CMake build
and ensure the components behave correctly.

### Fuzzing

If AFL++ is installed you can build the lexer fuzzer as described in
`tools/fuzz/README.md` and run it against the included corpus.
