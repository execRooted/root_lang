/*
 * root_lang compiler - common utilities and forward declarations.
 *
 * root_lang is a small, statically typed, compiled language. The compiler
 * lowers .rtl source into portable C99 and then invokes the host C toolchain
 * to produce a native executable.
 */
#ifndef ROOTLANG_COMMON_H
#define ROOTLANG_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define ROOTLANG_VERSION "1.0.0"
#define ROOTLANG_SRC_EXT ".rtl"

/* A growable string buffer used throughout code generation. */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} Rope;

void  rope_init(Rope *r);
void  rope_free(Rope *r);
void  rope_push(Rope *r, const char *text);
void  rope_pushn(Rope *r, const char *text, size_t n);
void  rope_pushc(Rope *r, char c);
void  rope_printf(Rope *r, const char *fmt, ...);

/* Duplicate a NUL terminated string; aborts on allocation failure. */
char *rl_strdup(const char *s);
char *rl_strndup(const char *s, size_t n);

/* Allocation helpers that abort on failure so call sites stay clean. */
void *rl_alloc(size_t bytes);
void *rl_realloc(void *ptr, size_t bytes);

/* Report a fatal, user facing error and terminate the process. */
void rl_fatal(const char *file, int line, int col, const char *fmt, ...)
    __attribute__((noreturn));

/* Read an entire file into a freshly allocated NUL terminated buffer. */
char *rl_read_file(const char *path, size_t *out_len);

/* Join a directory path with a relative path (caller frees the result). */
char *rl_path_join(const char *dir, const char *rel);

/* Return the directory portion of a path (caller frees the result). */
char *rl_dirname(const char *path);

/* Strip the trailing extension from a path (caller frees the result). */
char *rl_strip_ext(const char *path);

#endif /* ROOTLANG_COMMON_H */
