# Fuzzing Harness

This directory contains the AFL++ based lexer fuzzer. Building it requires CMake with `-DENABLE_FUZZ=ON`:

```bash
mkdir build && cd build
cmake -DENABLE_FUZZ=ON ..
make fuzz_lexer
```

Provide a seed corpus in `corpus` and run the fuzzer:

```bash
./fuzz_lexer -i ../tools/fuzz/corpus -o ./findings
```

Crashes will be written to the `findings` directory for investigation.
