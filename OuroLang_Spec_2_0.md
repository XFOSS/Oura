# OuroLang Language Specification 2.0

This document captures the current state of the experimental language.
It is intentionally brief and will evolve as the prototype grows.

## Lexical Structure

Tokens are similar to other C-like languages with a few additions:

* `async` / `await` keywords for asynchronous functions
* `gpu` keyword introducing GPU execution blocks
* identifiers are UTF‑8 encoded

Comments begin with `//` and continue to end of line.

## Functions

Functions are introduced with the `fn` keyword and support concise
syntax:

```ouro
fn add(x: int, y: int) -> int {
    return x + y;
}
```

Asynchronous functions are declared with `async fn` and must be awaited
by the caller.

## Control Flow

Conditional execution uses `if`/`else`.  Iteration is provided by a
`for ... in` loop over ranges or containers.

## Concurrency

An `async` block can spawn concurrent tasks.  Awaiting a task joins it
with the current execution context.  The prototype scheduler is a
simple round‑robin executor implemented in `ouro_lang.cc`.

## GPU Blocks

Code inside a `gpu { ... }` block is earmarked for offload to a GPU
backend.  In the current interpreter this merely prints a message but
serves as a placeholder for future acceleration support.

## Types

Primitive types are inspired by common C-family languages:

* `int` – 64-bit signed integer
* `float` – 64-bit floating point number
* `bool` – logical true or false
* `string` – UTF‑8 encoded sequence of characters
* `char` – single Unicode scalar value

Values can be annotated with these types directly. Example tokens and control flow:

```ouro
let threshold: int = 10
var value: int = 5

if value < threshold {
    print("below")
} else {
    print("above")
}
```

Type annotations are optional when the compiler can infer them.
