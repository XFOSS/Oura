# Architecture

OuroLang is intentionally lightweight and split into a few small
components.  The current prototype focuses on:

* a hand written lexer and recursive descent parser
* an intermediate representation used for experimentation with
  optimisations and code generation
* a REPL front-end built on top of the interpreter so language
  features can be exercised interactively

Future iterations will replace the experimental pieces with a modular
pipeline written in modern C++23 with optional Zig build integration.
