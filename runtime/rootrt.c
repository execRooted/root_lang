#include "rootrt.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

size_t rt_text_len(const char *s) { return s ? strlen(s) : 0; }

char *rt_text_concat(const char *a, const char *b) {
    size_t la = rt_text_len(a);
    size_t lb = rt_text_len(b);
    char *out = malloc(la + lb + 1);
    if (!out)
        return (char *)"";
    memcpy(out, a ? a : "", la);
    memcpy(out + la, b ? b : "", lb);
    out[la + lb] = '\0';
    return out;
}

bool rt_text_equal(const char *a, const char *b) {
    if (!a || !b)
        return a == b;
    return strcmp(a, b) == 0;
}

char *rt_text_slice(const char *s, int32_t start, int32_t end) {
    if (!s)
        return (char *)"";
    int32_t len = (int32_t)strlen(s);
    if (start < 0)
        start = 0;
    if (end > len)
        end = len;
    if (end < start)
        end = start;
    int32_t n = end - start;
    char *out = malloc((size_t)n + 1);
    if (!out)
        return (char *)"";
    memcpy(out, s + start, (size_t)n);
    out[n] = '\0';
    return out;
}

int64_t rt_text_to_int(const char *s) {
    return s ? strtoll(s, NULL, 10) : 0;
}

double rt_text_to_real(const char *s) {
    return s ? strtod(s, NULL) : 0.0;
}

void *rt_span_make(size_t elem_size, size_t count, const void *src) {
    size_t header = sizeof(size_t);
    unsigned char *block = malloc(header + elem_size * (count ? count : 1));
    if (!block)
        return NULL;
    *(size_t *)block = count;
    void *data = block + header;
    if (src && count)
        memcpy(data, src, elem_size * count);
    return data;
}

int32_t rt_span_len(const void *span) {
    if (!span)
        return 0;
    const unsigned char *block = (const unsigned char *)span - sizeof(size_t);
    return (int32_t)(*(const size_t *)block);
}

void rt_out_int(int64_t v) { printf("%lld", (long long)v); }
void rt_out_uint(uint64_t v) { printf("%llu", (unsigned long long)v); }
void rt_out_real(double v) { printf("%g", v); }
void rt_out_text(const char *s) { fputs(s ? s : "", stdout); }
void rt_out_char(char c) { putchar(c); }
void rt_out_bool(bool b) { fputs(b ? "yes" : "no", stdout); }
void rt_out_newline(void) { putchar('\n'); }

int32_t rt_in_int(void) {
    int32_t v = 0;
    if (scanf("%d", &v) != 1)
        v = 0;
    return v;
}

double rt_in_real(void) {
    double v = 0.0;
    if (scanf("%lf", &v) != 1)
        v = 0.0;
    return v;
}

char *rt_in_line(void) {
    static char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin))
        return (char *)"";
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n')
        buf[n - 1] = '\0';
    return buf;
}

double rt_root(double x) { return sqrt(x); }
double rt_power(double base, double exp) { return pow(base, exp); }
int64_t rt_abs_int(int64_t v) { return v < 0 ? -v : v; }

void rt_halt(int32_t code) { exit(code); }

void *rt_reserve(int32_t size) { return malloc((size_t)size); }
void  rt_release(void *ptr) { free(ptr); }
void *rt_copy(void *dst, void *src, int32_t size) {
    return memcpy(dst, src, (size_t)size);
}
void *rt_regrow(void *ptr, int32_t size) { return realloc(ptr, (size_t)size); }

void *rt_file_open(const char *path, const char *mode) {
    return fopen(path, mode);
}
void rt_file_close(void *file) {
    if (file)
        fclose((FILE *)file);
}
void rt_file_write(void *file, const char *text) {
    if (file)
        fputs(text ? text : "", (FILE *)file);
}
char *rt_file_read_line(void *file) {
    static char buf[4096];
    if (!file || !fgets(buf, sizeof(buf), (FILE *)file))
        return (char *)"";
    return buf;
}
int32_t rt_file_ended(void *file) {
    return file ? feof((FILE *)file) : 1;
}
void rt_file_remove(const char *path) { remove(path); }
