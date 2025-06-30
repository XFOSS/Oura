# Fuzzing Harness

This project includes a simple AFL++ harness used by CI to fuzz the lexer.
To build and run it manually run the same steps as the GitHub workflow:

```bash
mkdir build && cd build
cmake -DENABLE_FUZZ=ON ..
make fuzz_lexer
./fuzz_lexer -i ../fuzz/corpus -o ./findings -t 1000
```

Seed input files live in `fuzz/corpus`. Add more example `.ouro` programs in
that directory to help the fuzzer explore new code paths.
