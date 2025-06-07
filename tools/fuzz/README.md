# Fuzzing Harness

This directory contains the AFL++ based lexer fuzzer. Build it with your
system's `afl-clang-fast` compiler or another compatible setup. A simple
Makefile can also be used if preferred.

Provide a seed corpus in `fuzz/corpus` and run the fuzzer:

```bash
./fuzz_lexer -i ../fuzz/corpus -o ./findings
```

Crashes will be written to the `findings` directory for investigation.
