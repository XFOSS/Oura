# Testing

The repository includes a tiny suite of unit tests and an optional
fuzzing harness.

### Unit tests

Run the small suite of module tests with Zig:

```bash
zig build mod-test
```

### Fuzzing

If AFL++ is installed you can build the lexer fuzzer using Zig:

```bash
zig build fuzz-lexer
./zig-out/bin/fuzz_lexer -i fuzz/corpus -o findings
```
