#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1

#include "common.h"
#include "resolver.h"
#include "codegen.h"

#include <unistd.h>
#include <libgen.h>

typedef struct {
    const char *input;
    char       *output;
    const char *cc;
    bool        emit_c_only;
    bool        keep_intermediate;
} Options;

static char *find_runtime_dir(const char *argv0) {
    const char *env = getenv("ROOTLANG_HOME");
    if (env && *env)
        return rl_path_join(env, "runtime");

    char selfbuf[4096];
    ssize_t n = readlink("/proc/self/exe", selfbuf, sizeof(selfbuf) - 1);
    char *exe = NULL;
    if (n > 0) {
        selfbuf[n] = '\0';
        exe = rl_strdup(selfbuf);
    } else {
        exe = rl_strdup(argv0);
    }

    char *bindir = rl_dirname(exe);
    free(exe);

    char *cand = rl_path_join(bindir, "runtime");
    if (access(cand, F_OK) == 0) {
        free(bindir);
        return cand;
    }
    free(cand);

    char *parent = rl_dirname(bindir);
    free(bindir);
    cand = rl_path_join(parent, "runtime");
    free(parent);
    return cand;
}

static void run_or_die(const char *cmd) {
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "root_lang: C backend command failed (exit %d)\n", rc);
        exit(1);
    }
}

static void usage(const char *prog) {
    fprintf(stderr,
            "root_lang %s\n"
            "usage: %s <source%s> [options]\n"
            "  -o <file>      name the output executable\n"
            "  --emit-c       write the generated C only, do not compile\n"
            "  --keep         keep the generated C next to the output\n"
            "  --cc <program> C compiler to use (default: cc)\n"
            "  --version      print version and exit\n",
            ROOTLANG_VERSION, prog, ROOTLANG_SRC_EXT);
}

int main(int argc, char **argv) {
    Options opt = {0};
    opt.cc = getenv("ROOTLANG_CC");
    if (!opt.cc || !*opt.cc)
        opt.cc = "cc";

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "-o") == 0 && i + 1 < argc) {
            opt.output = rl_strdup(argv[++i]);
        } else if (strcmp(a, "--emit-c") == 0) {
            opt.emit_c_only = true;
        } else if (strcmp(a, "--keep") == 0) {
            opt.keep_intermediate = true;
        } else if (strcmp(a, "--cc") == 0 && i + 1 < argc) {
            opt.cc = argv[++i];
        } else if (strcmp(a, "--version") == 0) {
            printf("root_lang %s\n", ROOTLANG_VERSION);
            return 0;
        } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (a[0] == '-') {
            fprintf(stderr, "root_lang: unknown option '%s'\n", a);
            return 2;
        } else {
            opt.input = a;
        }
    }

    if (!opt.input) {
        usage(argv[0]);
        return 2;
    }

    if (!opt.output)
        opt.output = rl_strip_ext(opt.input);

    Program program = rl_resolve_program(opt.input);
    char *c_source = rl_generate_c(&program);

    char *c_path = rl_alloc(strlen(opt.output) + 16);
    sprintf(c_path, "%s.gen.c", opt.output);
    FILE *cf = fopen(c_path, "wb");
    if (!cf)
        rl_fatal(NULL, 0, 0, "cannot write generated C to '%s'", c_path);
    fwrite(c_source, 1, strlen(c_source), cf);
    fclose(cf);

    if (opt.emit_c_only) {
        printf("wrote %s\n", c_path);
        return 0;
    }

    char *runtime_dir = find_runtime_dir(argv[0]);
    char *rt_c = rl_path_join(runtime_dir, "rootrt.c");

    Rope cmd;
    rope_init(&cmd);
    rope_printf(&cmd, "%s -std=c11 -O2 -I\"%s\" \"%s\" \"%s\" -o \"%s\" -lm",
                opt.cc, runtime_dir, c_path, rt_c, opt.output);
    run_or_die(cmd.data);
    rope_free(&cmd);

    if (!opt.keep_intermediate)
        remove(c_path);

    free(c_source);
    free(c_path);
    free(rt_c);
    free(runtime_dir);
    return 0;
}
