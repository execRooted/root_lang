# root_lang

**A small, statically typed, compiled programming language written entirely in C.**

root_lang programs live in `.rtl` files. The compiler, `rootc`, is a self
contained C program: it lexes and parses your source, checks it, lowers it to
portable C99, and hands that off to your system C compiler to produce a native
executable. There is no VM, no interpreter, and no heavyweight backend
dependency — just C all the way down.

## Why

I wanted to learn how a real compiler fits together — tokenizing, parsing,
scope-aware code generation, a module system, and a runtime library — without
hiding behind a framework. Transpiling to C keeps the whole toolchain small and
portable while still producing genuinely native binaries.

The language is complete and stable: everything documented here compiles and
runs today.

## Features

- **Familiar, distinct syntax** — `fn`, `returns`, `if`/`else`, `while`, `for`
- **Statically typed** — mistakes are caught before your program runs
- **Native binaries** — lowered to C99 and compiled with your own `cc`
- **Real building blocks** — structs, enums, spans
  (arrays), references (pointers), and a module system with `use ... aka ...`
- **Batteries included** — a standard module (`root.rtl`) with I/O, text, math,
  memory, and file helpers
- **Tiny footprint** — the compiler is a few thousand lines of dependency-free C

## Quick example

```rtl
use("stdlib/root.rtl") aka std;

struct person = {
    int age;
    text name;
}

fn greet(person p) returns void {
    std.emit("Hello, ");
    std.emit(p.name);
    std.line();
}

fn main() returns void {
    person p;
    p.age = 25;
    p.name = "Ada";
    greet(p);
}
```

```
$ ./bin/rootc hello.rtl && ./hello
Hello, Ada
```

## Build

Requirements: any C11 compiler (`gcc` or `clang`), `make`.

```bash
git clone <your-repo-url> root_lang
cd root_lang
make            # produces ./bin/rootc
```

Or use the wrapper:

```bash
./build.sh
```

## Compile and run a program

Write a program (for example `hello.rtl`):

```rtl
use("stdlib/root.rtl") aka std;

fn main() returns void {
    std.emit("Hello from root_lang!");
    std.line();
}
```

Then build and run it:

```bash
./bin/rootc hello.rtl   # builds ./hello
./hello
```

Useful flags:

| Flag           | Effect                                            |
| -------------- | ------------------------------------------------- |
| `-o <file>`    | choose the output executable name                 |
| `--emit-c`     | write the generated C and stop (great for peeking)|
| `--keep`       | keep the generated `.gen.c` beside the executable |
| `--cc <prog>`  | use a specific C compiler                         |
| `--version`    | print the version                                 |

## Standard module

Import it with:

```rtl
use("stdlib/root.rtl") aka std;   # adjust the path to where root.rtl lives
```

| Function                               | Description                     |
| -------------------------------------- | ------------------------------- |
| `std.emit(value)`                      | print any scalar value          |
| `std.line()`                           | print a newline                 |
| `std.ask_int()` / `std.ask_real()`     | read a number from stdin        |
| `std.ask_line()`                       | read a line from stdin          |
| `std.root(x)` / `std.power(b, e)`      | square root / exponent          |
| `std.join(a, b)`                       | concatenate two texts           |
| `std.text_size(s)`                     | text length                     |
| `std.text_same(a, b)`                  | text equality                   |
| `std.text_slice(s, start, end)`        | substring                       |
| `std.file_open/close/write/...`        | file handling                   |
| `std.halt(code)`                       | exit the program                |

## How it works

```
 .rtl source
     |  lexer      (src/lexer.c)
     v  tokens
     |  parser     (src/parser.c)      ->  AST (src/ast.c)
     v  program
     |  resolver   (src/resolver.c)    ->  merges `use` modules
     v  flat program
     |  codegen    (src/codegen.c)     ->  scope-aware C99
     v  C source + runtime (runtime/rootrt.c)
     |  system cc
     v  native executable
```

## License

MIT — see [LICENSE](LICENSE).
