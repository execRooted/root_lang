#ifndef ROOTLANG_RUNTIME_H
#define ROOTLANG_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

size_t rt_text_len(const char *s);
char  *rt_text_concat(const char *a, const char *b);
bool   rt_text_equal(const char *a, const char *b);
char  *rt_text_slice(const char *s, int32_t start, int32_t end);
int64_t rt_text_to_int(const char *s);
double  rt_text_to_real(const char *s);

void   *rt_span_make(size_t elem_size, size_t count, const void *src);
int32_t rt_span_len(const void *span);

void rt_out_int(int64_t v);
void rt_out_uint(uint64_t v);
void rt_out_real(double v);
void rt_out_text(const char *s);
void rt_out_char(char c);
void rt_out_bool(bool b);
void rt_out_newline(void);

#define rt_print(x) _Generic((x),                 \
    char *:          rt_out_text,                 \
    const char *:    rt_out_text,                 \
    float:           rt_out_real,                 \
    double:          rt_out_real,                 \
    bool:            rt_out_bool,                 \
    int8_t:          rt_out_int,                  \
    int16_t:         rt_out_int,                  \
    int32_t:         rt_out_int,                  \
    int64_t:         rt_out_int,                  \
    uint8_t:         rt_out_uint,                 \
    uint16_t:        rt_out_uint,                 \
    uint32_t:        rt_out_uint,                 \
    uint64_t:        rt_out_uint,                 \
    default:         rt_out_int)(x)

int32_t rt_in_int(void);
double  rt_in_real(void);
char   *rt_in_line(void);

double rt_root(double x);
double rt_power(double base, double exp);
int64_t rt_abs_int(int64_t v);

void rt_halt(int32_t code);

void *rt_reserve(int32_t size);
void  rt_release(void *ptr);
void *rt_copy(void *dst, void *src, int32_t size);
void *rt_regrow(void *ptr, int32_t size);

void *rt_file_open(const char *path, const char *mode);
void  rt_file_close(void *file);
void  rt_file_write(void *file, const char *text);
char *rt_file_read_line(void *file);
int32_t rt_file_ended(void *file);
void  rt_file_remove(const char *path);

#endif
