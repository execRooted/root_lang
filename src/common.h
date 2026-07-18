#ifndef ROOTLANG_COMMON_H
#define ROOTLANG_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define ROOTLANG_VERSION "1.0.0"
#define ROOTLANG_SRC_EXT ".rtl"

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

char *rl_strdup(const char *s);
char *rl_strndup(const char *s, size_t n);

void *rl_alloc(size_t bytes);
void *rl_realloc(void *ptr, size_t bytes);

void rl_fatal(const char *file, int line, int col, const char *fmt, ...)
    __attribute__((noreturn));

char *rl_read_file(const char *path, size_t *out_len);

char *rl_path_join(const char *dir, const char *rel);

char *rl_dirname(const char *path);

char *rl_strip_ext(const char *path);

char *rl_find_home(void);

#endif
