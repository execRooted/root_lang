# root_lang Design Notes

A short account of the decisions behind root_lang and its compiler.

## Goals

1. **Understandable end to end.** Every stage — lexer, parser, resolver,
   code generator, runtime — should be small enough to read in one sitting.
2. **Genuinely native output.** Programs become real executables, not bytecode
   for an interpreter.
3. **No heavy dependencies.** The compiler is plain C11 and needs only a C
   compiler and `make`.

## Why transpile to C?

Emitting C99 and shelling out to the host `cc` gives us native performance,
portability across every platform that has a C compiler, and access to decades
of optimizer work — all without linking a large backend library into the
compiler itself. The generated translation unit is deliberately readable, so
`--emit-c` doubles as a teaching aid.

## Pipeline

```
lexer -> parser -> resolver -> code generator -> host C compiler
```

- **Lexer** (`src/lexer.c`) turns characters into tokens. Comments start with
  `#`; booleans are the words `yes` and `no`.
- **Parser** (`src/parser.c`) is a hand-written recursive-descent parser with
  precedence climbing for expressions. Assignment is an expression, which keeps
  `walk` loop steps simple.
- **Resolver** (`src/resolver.c`) loads `use`d modules recursively, guards
  against cycles, and flattens everything into a single program. Functions from
  a module are tagged with their alias.
- **Code generator** (`src/codegen.c`) is scope-aware: it tracks the type of
  every binding so it can lower pseudo-fields (`.address`, `.value`, `.length`),
  overload text `+` and `==`, and reach blueprint fields through the correct
  `.`/`->` operator.

## Name mangling

To avoid clashing with C keywords and runtime symbols, user functions are
emitted with an `rl_u_` prefix, and module functions fold their alias into the
name (`std.emit` becomes a call to the runtime symbol it binds to; a plain user
function `add` becomes `rl_u_add`). Blueprints become `rl_s_<name>` structs.

## Spans

A span (`T[]`) is a single heap allocation with a `size_t` length header placed
immediately before the element region. Generated code receives a pointer to the
first element, so ordinary C indexing works unchanged, while `.length` reads the
hidden header via `rt_span_len`.

## Text

Text values are NUL-terminated `char*`. The `+` operator on two texts lowers to
`rt_text_concat`, and `==`/`!=` lower to `rt_text_equal`, so comparisons are by
content rather than by address.

## Overloaded printing

`std.emit` binds to the runtime's `rt_print` macro, which uses C11 `_Generic` to
dispatch to the right formatter based on the argument's static type. This lets a
single function name print integers, reals, text, and booleans without the
language needing generics or overloading of its own.

## Possible future directions

- A proper diagnostic pass with more precise type errors
- More container helpers in the standard module
- An optional bytecode/interpreter mode for quick iteration
