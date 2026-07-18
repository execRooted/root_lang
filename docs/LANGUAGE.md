# root_lang Language Reference

A complete reference for the root_lang programming language. Source files use
the `.rtl` extension.

## Table of contents

- [Comments](#comments)
- [Types](#types)
- [Bindings](#bindings)
- [Operators](#operators)
- [Casting](#casting)
- [Control flow](#control-flow)
- [Functions](#functions)
- [Spans (arrays)](#spans-arrays)
- [Text (strings)](#text-strings)
- [References (pointers)](#references-pointers)
- [Blueprints (structs)](#blueprints-structs)
- [Choices (enums)](#choices-enums)
- [Modules](#modules)
- [Native functions](#native-functions)
- [Standard module](#standard-module)

---

## Comments

```rtl
# this is a single line comment
```

---

## Types

| Type      | Description             | Example          |
| --------- | ----------------------- | ---------------- |
| `int`     | 32-bit signed integer   | `42`             |
| `int8`    | 8-bit signed integer    | `-3`             |
| `int16`   | 16-bit signed integer   | `1000`           |
| `int64`   | 64-bit signed integer   | `9000000000`     |
| `uint`    | 32-bit unsigned integer | `42`             |
| `float`   | 32-bit floating point   | `3.14`           |
| `double`  | 64-bit floating point   | `3.14159d`       |
| `char`    | single character        | `'a'`            |
| `text`    | sequence of characters  | `"hello"`        |
| `bool`    | boolean                 | `yes` / `no`     |
| `T[]`     | span (array) of `T`     | `{1, 2, 3}`      |
| `T*`      | reference (pointer) to `T` | `x.address`   |
| `any`     | opaque reference, castable | file handles etc. |
| `void`    | absence of a value      | function returns |

Numeric literals may carry a suffix: `3.0f` is a `float`, `3.0d` is a `double`.
A bare decimal literal such as `3.14` is a `float`.

---

## Bindings

A binding is a type, a name, and an optional initializer:

```rtl
int x = 5;
float f = 3.14;
double d = 3.14159d;
char c = 'a';
text s = "hello world";
bool b = yes;
```

Bindings without an initializer take a zero value (`0`, `0.0`, `""`, `no`,
`NULL`, or an all-zero blueprint).

`let` is an optional keyword that reads nicely; `const` marks a binding as
immutable:

```rtl
let int count = 0;
const float pi = 3.14159;
```

### Assignment

```rtl
x = 10;
x += 5;
x -= 3;
x *= 2;
x /= 4;
```

---

## Operators

### Arithmetic

```rtl
int a = 10 + 3;   # 13
int b = 10 - 3;   # 7
int c = 10 * 3;   # 30
int d = 10 / 3;   # 3
int e = 10 % 3;   # 1
```

### Comparison

```rtl
bool a = 5 == 5;  # yes
bool b = 5 != 3;  # yes
bool c = 5 > 3;   # yes
bool d = 5 < 3;   # no
bool e = 5 >= 5;  # yes
bool f = 5 <= 3;  # no
```

### Logical

```rtl
bool a = yes && no;  # no
bool b = yes || no;  # yes
bool c = !yes;       # no
```

### Unary

```rtl
int x = -5;   # negation
bool b = !yes; # logical not
```

---

## Casting

Use the `to` operator to convert between types:

```rtl
float x = 3.14;
int y = x to int;   # 3
```

Casting a `text` to `int` parses the number:

```rtl
int n = "42" to int;   # 42
```

---

## Control flow

### when / elsewhen / orelse

```rtl
when (x > 0) {
    std.emit("positive");
} elsewhen (x == 0) {
    std.emit("zero");
} orelse {
    std.emit("negative");
}
```

### loop (conditional loop)

```rtl
loop (x > 0) {
    x -= 1;
}
```

### walk (counting loop)

```rtl
walk (int i = 0, i < 10, i += 1) {
    std.emit(i);
}
```

The three comma-separated parts are the initializer, the condition, and the
step.

### stop / skip

`stop` breaks out of the nearest loop; `skip` continues with the next
iteration.

```rtl
walk (int i = 0, i < 10, i += 1) {
    when (i % 2 != 0) { skip; }
    when (i > 8) { stop; }
    std.emit(i);
}
```

---

## Functions

### Declaration

```rtl
fn add(int a, int b) gives int {
    give a + b;
}
```

### Calling

```rtl
int result = add(3, 4);
```

### Void functions

Use `void` as the return type for functions that produce no value:

```rtl
fn greet(text name) gives void {
    std.emit(name);
}
```

### Recursion

```rtl
fn fib(int n) gives int {
    when (n <= 1) {
        give n;
    }
    give fib(n - 1) + fib(n - 2);
}
```

---

## Spans (arrays)

### Declaration

```rtl
int[] nums = {1, 2, 3, 4, 5};
float[] reals = {1.1, 2.2, 3.3};
text[] words = {"hello", "world"};
```

### Access

```rtl
int first = nums[0];
nums[0] = 10;
```

### Length

```rtl
int n = nums.length;   # 5
```

### Passing to functions

```rtl
fn total(int[] arr, int size) gives int {
    int sum = 0;
    walk (int i = 0, i < size, i += 1) {
        sum += arr[i];
    }
    give sum;
}
```

---

## Text (strings)

```rtl
text s = "Hello world";
text empty = "";
```

### Concatenation

```rtl
text a = "Hello";
text b = " World";
text c = a + b;              # "Hello World"
text d = std.join(a, b);    # same result
```

### Length

```rtl
int n = "hello".length;     # 5
```

### Comparison

Comparing text with `==` compares contents, not addresses.

```rtl
when ("abc" == "abc") { std.emit("equal"); }
```

---

## References (pointers)

### Taking a reference

```rtl
int x = 5;
int *p = x.address;
```

### Reading and writing through a reference

```rtl
int value = p.value;
p.value = 10;
```

### Pass by reference

```rtl
fn bump(int *p) gives void {
    p.value += 1;
}

int x = 5;
bump(x.address);   # x is now 6
```

---

## Blueprints (structs)

### Declaration

```rtl
blueprint person = {
    int age;
    float height;
    text name;
}
```

### Instantiation

```rtl
person p;   # all fields zeroed
```

### Field access

```rtl
p.age = 30;
p.height = 1.75;
p.name = "Ada";
```

References to blueprints reach fields through `.value`:

```rtl
fn resize(person *p) gives void {
    p.value.age = 40;
}
```

---

## Choices (enums)

### Declaration

```rtl
choices direction = {
    NORTH,   # 0
    SOUTH,   # 1
    EAST,    # 2
    WEST     # 3
}
```

### Manual values

```rtl
choices level = {
    EASY,        # 0
    MEDIUM,      # 1
    HARD = 9,    # 9
    EXTREME      # 10
}
```

Choice values are integers and can be used anywhere an `int` is expected.

---

## Modules

Split code across files with `use`:

```rtl
use("calc.rtl") aka calc;

int result = calc.add(3, 4);
```

The alias (`calc`) prefixes every function pulled in from that file. Paths are
resolved relative to the importing file's directory.

---

## Native functions

Functions can bind directly to a C symbol provided by the runtime (or any
linked C library) with `native`:

```rtl
fn root(double x) gives double native "rt_root" {}
```

A native function has no root_lang body — calls are lowered straight to the
named C symbol.

---

## Standard module

Import with `use("root.rtl") aka std;`

### Output

```rtl
std.emit(value);   # prints any scalar value
std.line();        # prints a newline
```

### Input

```rtl
int x = std.ask_int();
double f = std.ask_real();
text line = std.ask_line();
```

### Math

```rtl
double s = std.root(16.0);       # 4.0
double p = std.power(2.0, 10.0); # 1024.0
```

### Files

```rtl
any f = std.file_open("notes.txt", "w");
std.file_write(f, "Hello!\n");
std.file_close(f);

any r = std.file_open("notes.txt", "r");
text line = std.file_read_line(r);
std.file_close(r);
std.file_remove("notes.txt");
```

### Program control

```rtl
std.halt(0);   # exit success
std.halt(1);   # exit with an error code
```
