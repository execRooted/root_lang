# root_lang

**A small, statically typed, compiled programming language written entirely in C.**

root_lang source files use the `.rtl` extension. The compiler, `rtlc`, lexes and
parses your program, type-checks it, lowers it to portable C99, and hands that
off to your system C compiler to produce a native executable. No VM, no
interpreter, no heavyweight backend — just C all the way down.

This README is the complete language reference.

---

## Table of contents

- [Getting started](#getting-started)
  - [Install system-wide](#install-system-wide-recommended)
  - [Build locally](#build-locally-without-installing)
  - [The `rtlc` command](#the-rtlc-command)
- [Program structure](#program-structure)
- [Comments](#comments)
- [Types](#types)
- [Literals](#literals)
- [Variables](#variables)
- [Operators](#operators)
- [Casting](#casting)
- [Control flow](#control-flow)
- [Functions](#functions)
- [Arrays (spans)](#arrays-spans)
- [Text (strings)](#text-strings)
- [References (pointers)](#references-pointers)
- [Structs](#structs)
- [Enums](#enums)
- [Modules and imports](#modules-and-imports)
- [Native functions](#native-functions)
- [Standard module reference](#standard-module-reference)
- [Grammar summary](#grammar-summary)
- [How it works](#how-it-works)
- [License](#license)

---

## Getting started

### Install system-wide (recommended)

Run the installer as root. It builds the compiler, installs it as `rtlc`, and
sets up the runtime and standard library so they can be found from anywhere:

```bash
sudo ./install.sh
```

The installer:

- installs the compiler to `/usr/local/bin/rtlc`
- installs the runtime and standard library to `/usr/local/lib/root_lang`
- registers `ROOTLANG_HOME` in `/etc/profile.d/root_lang.sh`

Open a new terminal afterwards (so `ROOTLANG_HOME` is loaded), then write a
program, `hello.rtl`:

```rtl
use("stdlib/root.rtl") as std;

fn main() return void {
    std.out("Hello, root_lang!");
    std.line();
}
```

Compile and run it:

```bash
rtlc hello.rtl   # produces ./hello
./hello
```

To remove root_lang later:

```bash
sudo ./uninstall.sh
```

### Build locally (without installing)

If you would rather not install anything, just build the compiler:

```bash
make            # produces ./bin/rootc
```

When running locally, invoke the compiler as `./bin/rootc`. Keep the
`stdlib/` folder next to your program (or set `ROOTLANG_HOME` to this
directory) so imports resolve:

```bash
./bin/rootc hello.rtl   # produces ./hello
./hello
```

### The `rtlc` command

```
rtlc <source.rtl> [options]
```

| Option          | Effect                                             |
| --------------- | -------------------------------------------------- |
| `-o <file>`     | name the output executable                         |
| `--emit-c`      | write the generated C only, do not compile         |
| `--keep`        | keep the generated `.gen.c` next to the executable |
| `--cc <program>`| C compiler to use (default `cc`)                   |
| `-h`, `--help`  | show usage                                         |

Examples:

```bash
rtlc app.rtl                 # -> ./app
rtlc app.rtl -o build/app    # -> ./build/app
rtlc app.rtl --emit-c        # writes app.gen.c and stops
rtlc app.rtl --cc clang      # compile the generated C with clang
```

The order of the source file and options does not matter, so
`rtlc -o app app.rtl` works too. Without `-o`, the output name is the source
filename with its extension stripped.

Environment variables:

| Variable        | Purpose                                                       |
| --------------- | ------------------------------------------------------------ |
| `ROOTLANG_HOME` | where the runtime and standard library live (set by installer)|
| `ROOTLANG_CC`   | default C compiler, if `--cc` is not given                    |

---

## Program structure

A program is a sequence of **top-level declarations**. There are four kinds:

- imports — `use(...) as ...;`
- functions — `fn ...`
- structs — `struct ...`
- enums — `enum ...`

Execution begins at the function named `main`. Declarations may appear in any
order; a function can call another declared later in the file.

```rtl
use("stdlib/root.rtl") as std;

struct point = { int x; int y; }

enum color = { RED, GREEN, BLUE }

fn main() return void {
    std.out("ready");
    std.line();
}
```

---

## Comments

Comments start with `#` and run to the end of the line. 


```rtl
# this is a comment
int x = 5;   # trailing comment
```

---

## Types

| Type      | Meaning                    | C representation   |
| --------- | -------------------------- | ------------------ |
| `int`     | 32-bit signed integer      | `int32_t`          |
| `int8`    | 8-bit signed integer       | `int8_t`           |
| `int16`   | 16-bit signed integer      | `int16_t`          |
| `int64`   | 64-bit signed integer      | `int64_t`          |
| `uint`    | 32-bit unsigned integer    | `uint32_t`         |
| `uint8`   | 8-bit unsigned integer     | `uint8_t`          |
| `uint16`  | 16-bit unsigned integer    | `uint16_t`         |
| `uint64`  | 64-bit unsigned integer    | `uint64_t`         |
| `float`   | 32-bit floating point      | `float`            |
| `double`  | 64-bit floating point      | `double`           |
| `char`    | single character           | `char`             |
| `str`    | string of characters       | `char*`            |
| `bool`    | boolean (`true` / `false`) | `bool`             |
| `void`    | no value (function result) | `void`             |
| `any`     | opaque reference, castable | `void*`            |
| `T[]`     | array (span) of `T`        | length-tagged `T*` |
| `T*`      | reference (pointer) to `T` | `T*`               |

Type suffixes compose left to right: `int[]` is an array of ints, `int*` a
reference to an int, `int[]*` a reference to an array.

---

## Literals

```rtl
42            # int
3.14          # float  (a bare decimal is a float)
3.14f         # float  (explicit)
3.14159d      # double (the 'd' suffix)
'a'           # char
"hello"       # str
true          # bool true
false         # bool false
nil           # null reference
```

Escape sequences inside `str` and `char` literals: `\n`, `\t`, `\r`, `\0`,
`\\`, `\"`, `\'`.

---

## Variables

A declaration is a type, a name, and an optional initializer:

```rtl
int x = 5;
float f = 3.14;
double d = 3.14159d;
char c = 'a';
str s = "hello world";
bool b = true;
```

Without an initializer, a variable takes its zero value (`0`, `0.0`, `""`,
`false`, `nil`, or an all-zero struct):

```rtl
int counter;      # 0
str name;        # ""
```

The keyword `let` is optional and reads nicely. `const` marks a binding as
immutable and may lead the declaration or follow the initializer:

```rtl
let int total = 0;
const float pi = 3.14159;
int max = 100 const;   # equivalent trailing form
```

### Assignment

```rtl
x = 10;
x += 5;
x -= 3;
x *= 2;
x /= 4;
```

Assignment is an expression, so it can appear inside a `for` step.

---

## Operators

### Arithmetic

```rtl
10 + 3    # 13
10 - 3    # 7
10 * 3    # 30
10 / 3    # 3   (integer division)
10 % 3    # 1
-x        # negation
```

### Comparison

```rtl
a == b    a != b
a <  b    a >  b
a <= b    a >= b
```

For `str`, `==` and `!=` compare **contents**, not addresses.

### Logical

```rtl
a && b    # and
a || b    # or
!a        # not
```

Precedence, lowest to highest: assignment, `||`, `&&`, equality, comparison,
`+ -`, `* / %`, the `to` cast, unary `- !`, then postfix (`.field`,
`[index]`), then primaries.

---

## Casting

Convert between types with the `to` operator:

```rtl
float x = 3.14;
int y = x to int;        # 3

int big = 300;
int8 small = big to int8;

str digits = "42";
int n = digits to int;   # parses the number -> 42
```

---

## Control flow

### if / else if / else

```rtl
if (x > 0) {
    std.out("positive");
} else if (x == 0) {
    std.out("zero");
} else {
    std.out("negative");
}
```

A condition must be parenthesized and each branch is a brace-delimited block.

### while

```rtl
while (x > 0) {
    x -= 1;
}
```

### for

The `for` header has three **comma-separated** parts — initializer, condition,
step:

```rtl
for (int i = 0, i < 10, i += 1) {
    std.out(i);
}
```

### stop and skip

`stop` breaks out of the nearest loop; `skip` continues with the next
iteration.

```rtl
for (int i = 0, i < 10, i += 1) {
    if (i % 2 != 0) { skip; }   # skip odd numbers
    if (i > 8)      { stop; }   # stop past 8
    std.out(i);
}
```

---

## Functions

Declared with `fn`, a parameter list, and a `return` type:

```rtl
fn add(int a, int b) return int {
    return a + b;
}
```

Calling:

```rtl
int result = add(3, 4);
```

Functions that produce no value return `void`:

```rtl
fn greet(str name) return void {
    std.out("Hi ");
    std.out(name);
    std.line();
}
```

Recursion is fully supported:

```rtl
fn fib(int n) return int {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}
```

---

## Arrays (spans)

An array literal is written with braces. Arrays carry their own length.

```rtl
int[] nums = {1, 2, 3, 4, 5};
float[] reals = {1.1, 2.2, 3.3};
str[] words = {"hello", "world"};
```

Indexing and assignment:

```rtl
int first = nums[0];
nums[0] = 10;
```

The `.length` pseudo-field gives the element count:

```rtl
int n = nums.length;   # 5
```

Passing to functions (arrays are passed by reference to their storage):

```rtl
fn total(int[] arr, int size) return int {
    int sum = 0;
    for (int i = 0, i < size, i += 1) {
        sum += arr[i];
    }
    return sum;
}

int s = total(nums, nums.length);
```

---

## Text (strings)

```rtl
str s = "Hello world";
str empty = "";
```

Concatenate with `+` or `std.join`:

```rtl
str a = "Hello";
str b = " World";
str c = a + b;             # "Hello World"
str d = std.join(a, b);    # same result
```

Length via `.length`, and content comparison with `==`:

```rtl
int len = "hello".length;         # 5
if ("abc" == "abc") { ... }       # true (compares contents)
```

More text helpers live in the [standard module](#standard-module-reference).

---

## References (pointers)

Take a reference to a value with the `.address` pseudo-field:

```rtl
int x = 5;
int *p = x.address;
```

Read or write through a reference with `.value`:

```rtl
int v = p.value;    # read
p.value = 10;       # write
```

Pass by reference so a function can mutate the caller's value:

```rtl
fn bump(int *p) return void {
    p.value += 1;
}

int x = 5;
bump(x.address);    # x is now 6
```

`nil` is the empty reference.

---

## Structs

Declare a struct with named, typed fields:

```rtl
struct person = {
    int age;
    float height;
    str name;
}
```

Instantiate (all fields start at their zero value) and access fields with `.`:

```rtl
person p;
p.age = 30;
p.height = 1.75;
p.name = "Ada";
```

Reach fields through a reference with `.value`:

```rtl
fn set_age(person *p, int age) return void {
    p.value.age = age;
}

set_age(p.address, 40);
```

Structs can be stored in arrays and passed to functions by value.

---

## Enums

An enum defines named integer constants. Values start at `0` and increase by
one unless assigned explicitly:

```rtl
enum direction = {
    NORTH,   # 0
    SOUTH,   # 1
    EAST,    # 2
    WEST     # 3
}
```

Explicit values, with later members continuing from the last one:

```rtl
enum level = {
    EASY,        # 0
    MEDIUM,      # 1
    HARD = 9,    # 9
    EXTREME      # 10
}
```

Enum members are plain integers and can be used anywhere an `int` is expected:

```rtl
int d = NORTH;
if (d == NORTH) { std.out("north"); }
```

> Note: enum members are **names**, not bare numbers. `enum e = {1, 2, 3}` is
> invalid. A list of numbers is an array literal instead: `int[] e = {1, 2, 3};`.

---

## Modules and imports

Split code across files with `use ... as ...`. The alias prefixes every
function pulled in from that file. Paths are resolved **relative to the
importing file's directory**; if not found there, the compiler also looks under
`ROOTLANG_HOME` (which is how the installed standard library is found).

`calc.rtl`:

```rtl
fn add(int a, int b) return int {
    return a + b;
}
```

`main.rtl`:

```rtl
use("calc.rtl") as calc;
use("stdlib/root.rtl") as std;

fn main() return void {
    std.out(calc.add(3, 4));   # 7
    std.line();
}
```

Imports are de-duplicated, so importing the same file through multiple paths is
safe.

---

## Native functions

A function can bind directly to a C symbol (from the runtime or any linked C
library) using `native` followed by the symbol name. A native function has an
empty body — calls are lowered straight to the named C function.

```rtl
fn root(double x) return double native "rt_root" {}
```

The entire standard module is written this way (see `stdlib/root.rtl`).

---

## Standard module reference

Import it with:

```rtl
use("stdlib/root.rtl") as std;
```

### Output

| Function            | Description                                    |
| ------------------- | ---------------------------------------------- |
| `std.out(value)`    | print any scalar value (int, real, str, bool) |
| `std.out_char(c)`   | print a single character                       |
| `std.line()`        | print a newline                                |

### Input

| Function          | Description                      |
| ----------------- | -------------------------------- |
| `std.ask_int()`   | read an `int` from stdin         |
| `std.ask_real()`  | read a `double` from stdin       |
| `std.ask_line()`  | read a line of `str` from stdin |

### Math

| Function                | Description              |
| ----------------------- | ------------------------ |
| `std.root(x)`           | square root              |
| `std.power(base, exp)`  | exponentiation           |
| `std.abs(n)`            | absolute value of an int |

### Text

| Function                        | Description               |
| ------------------------------- | ------------------------- |
| `std.join(a, b)`                | concatenate two texts     |
| `std.str_size(s)`              | length of a text          |
| `std.str_same(a, b)`           | content equality (`bool`) |
| `std.str_slice(s, start, end)` | substring `[start, end)`  |

### Memory

| Function                   | Description                   |
| -------------------------- | ----------------------------- |
| `std.reserve(size)`        | allocate `size` bytes (`any`) |
| `std.release(ptr)`         | free a reservation            |
| `std.copy(dst, src, size)` | copy `size` bytes             |
| `std.regrow(ptr, size)`    | resize a reservation          |

### Files

| Function                       | Description                          |
| ------------------------------ | ------------------------------------ |
| `std.file_open(path, mode)`    | open a file, returns an `any` handle |
| `std.file_close(handle)`       | close a file                         |
| `std.file_write(handle, text)` | write text to a file                 |
| `std.file_read_line(handle)`   | read one line as `str`              |
| `std.file_ended(handle)`       | `1` if at end of file, else `0`      |
| `std.file_remove(path)`        | delete a file                        |

### Program control

| Function         | Description            |
| ---------------- | ---------------------- |
| `std.halt(code)` | exit with an exit code |

Example using files:

```rtl
use("stdlib/root.rtl") as std;

fn main() return void {
    any f = std.file_open("notes.txt", "w");
    std.file_write(f, "Hello!\n");
    std.file_close(f);

    any r = std.file_open("notes.txt", "r");
    str line = std.file_read_line(r);
    std.out(line);
    std.file_close(r);

    std.file_remove("notes.txt");
}
```

---

## Summary

An informal overview of the syntax.

```
program        := decl*
decl           := import | function | struct | enum

import         := "use" "(" string ")" "as" ident ";"
function       := "fn" ident "(" params? ")" "return" type
                  ( "native" string "{" "}" | block )
params         := param ("," param)*
param          := type ident
struct         := "struct" ident "=" "{" field* "}"
field          := type ident ";"
enum           := "enum" ident "=" "{" enumerator ("," enumerator)* "}"
enumerator     := ident ( "=" int )?

type           := base ("[]" | "*")*
base           := "int" | "int8" | "int16" | "int64"
                | "uint" | "uint8" | "uint16" | "uint64"
                | "float" | "double" | "char" | "str"
                | "bool" | "void" | "any" | ident

block          := "{" statement* "}"
statement      := var_decl ";" | expr ";" | return | if | while
                | for | block | "stop" ";" | "skip" ";"
var_decl       := ("let" | "const")? type ident ("=" expr)? "const"?
return         := "return" expr? ";"
if             := "if" "(" expr ")" block ( "else" (if | block) )?
while          := "while" "(" expr ")" block
for            := "for" "(" var_decl "," expr "," expr ")" block

expr           := assignment
assignment     := logic_or ( ("=" | "+=" | "-=" | "*=" | "/=") expr )?
logic_or       := logic_and ("||" logic_and)*
logic_and      := equality ("&&" equality)*
equality       := comparison (("==" | "!=") comparison)*
comparison     := term (("<" | ">" | "<=" | ">=") term)*
term           := factor (("+" | "-") factor)*
factor         := cast (("*" | "/" | "%") cast)*
cast           := unary ("to" type)*
unary          := ("-" | "!") unary | postfix
postfix        := primary ( "." ident ("(" ")")? | "[" expr "]" )*
primary        := int | float | double | string | char | bool | "nil"
                | "sizeof" "(" type ")"
                | ident | ident "(" args? ")" | ident "." ident "(" args? ")"
                | "(" expr ")" | "{" (expr ("," expr)*)? "}"
```

Pseudo-fields available via `.` on any expression: `.address` (take a
reference), `.value` (dereference a reference), and `.length` (element count of
an array or character count of a text).

---

## License

MIT — see [LICENSE](LICENSE).

---
***Made by execRooted***