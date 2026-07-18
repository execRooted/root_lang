#include "common.h"
#include <stdarg.h>

void *rl_alloc(size_t bytes) {
    void *p = calloc(1, bytes ? bytes : 1);
    if (!p) {
        fprintf(stderr, "root_lang: out of memory\n");
        exit(70);
    }
    return p;
}

void *rl_realloc(void *ptr, size_t bytes) {
    void *p = realloc(ptr, bytes ? bytes : 1);
    if (!p) {
        fprintf(stderr, "root_lang: out of memory\n");
        exit(70);
    }
    return p;
}

char *rl_strdup(const char *s) {
    size_t n = strlen(s);
    char *p = rl_alloc(n + 1);
    memcpy(p, s, n + 1);
    return p;
}

char *rl_strndup(const char *s, size_t n) {
    char *p = rl_alloc(n + 1);
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

void rope_init(Rope *r) {
    r->cap = 256;
    r->len = 0;
    r->data = rl_alloc(r->cap);
    r->data[0] = '\0';
}

void rope_free(Rope *r) {
    free(r->data);
    r->data = NULL;
    r->len = r->cap = 0;
}

static void rope_reserve(Rope *r, size_t extra) {
    if (r->len + extra + 1 > r->cap) {
        while (r->len + extra + 1 > r->cap)
            r->cap *= 2;
        r->data = rl_realloc(r->data, r->cap);
    }
}

void rope_pushn(Rope *r, const char *text, size_t n) {
    rope_reserve(r, n);
    memcpy(r->data + r->len, text, n);
    r->len += n;
    r->data[r->len] = '\0';
}

void rope_push(Rope *r, const char *text) {
    rope_pushn(r, text, strlen(text));
}

void rope_pushc(Rope *r, char c) {
    rope_reserve(r, 1);
    r->data[r->len++] = c;
    r->data[r->len] = '\0';
}

void rope_printf(Rope *r, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (needed < 0) {
        va_end(ap2);
        return;
    }
    rope_reserve(r, (size_t)needed);
    vsnprintf(r->data + r->len, (size_t)needed + 1, fmt, ap2);
    va_end(ap2);
    r->len += (size_t)needed;
}

void rl_fatal(const char *file, int line, int col, const char *fmt, ...) {
    fflush(stdout);
    if (file && line > 0)
        fprintf(stderr, "%s:%d:%d: error: ", file, line, col);
    else
        fprintf(stderr, "root_lang: error: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(65);
}

char *rl_read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f)
        rl_fatal(NULL, 0, 0, "cannot open source file '%s'", path);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        rl_fatal(NULL, 0, 0, "cannot read source file '%s'", path);
    }
    fseek(f, 0, SEEK_SET);
    char *buf = rl_alloc((size_t)size + 1);
    size_t read = fread(buf, 1, (size_t)size, f);
    buf[read] = '\0';
    fclose(f);
    if (out_len)
        *out_len = read;
    return buf;
}

char *rl_dirname(const char *path) {
    const char *slash = strrchr(path, '/');
    if (!slash)
        return rl_strdup(".");
    if (slash == path)
        return rl_strdup("/");
    return rl_strndup(path, (size_t)(slash - path));
}

char *rl_path_join(const char *dir, const char *rel) {
    if (rel[0] == '/')
        return rl_strdup(rel);
    size_t dn = strlen(dir);
    size_t rn = strlen(rel);
    char *out = rl_alloc(dn + rn + 2);
    memcpy(out, dir, dn);
    size_t pos = dn;
    if (dn > 0 && dir[dn - 1] != '/')
        out[pos++] = '/';
    memcpy(out + pos, rel, rn);
    out[pos + rn] = '\0';
    return out;
}

char *rl_strip_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    const char *slash = strrchr(path, '/');
    if (!dot || (slash && dot < slash))
        return rl_strdup(path);
    return rl_strndup(path, (size_t)(dot - path));
}
