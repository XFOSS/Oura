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

```
cmake -DENABLE_FUZZ=ON ..
make fuzz_lexer
./fuzz_lexer -i ../tools/fuzz/corpus -o ./findings
```
