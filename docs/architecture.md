# Architecture

OuroLang is intentionally lightweight and split into a few small
components.  The current prototype focuses on:

* a hand written lexer and recursive descent parser
* an intermediate representation used for experimentation with
  optimisations and code generation
* a REPL front-end built on top of the interpreter so language
  features can be exercised interactively

The project is now driven by Zig's build system which compiles the
C and C++23 components. Future iterations will continue to evolve this
modular pipeline while keeping Zig at the centre of the build process.