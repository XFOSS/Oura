# Fuzzing Harness

This directory contains the AFL++ based lexer fuzzer. Build it using Zig:

```bash
zig build fuzz-lexer
```

Provide a seed corpus in `corpus` and run the fuzzer:

```bash
./fuzz_lexer -i ../tools/fuzz/corpus -o ./findings
```

Crashes will be written to the `findings` directory for investigation.
