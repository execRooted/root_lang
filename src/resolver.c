#include "resolver.h"
#include "lexer.h"
#include "parser.h"

typedef struct {
    char  **paths;
    size_t  count;
    size_t  cap;
} LoadedSet;

static bool loaded_contains(LoadedSet *set, const char *path) {
    for (size_t i = 0; i < set->count; i++)
        if (strcmp(set->paths[i], path) == 0)
            return true;
    return false;
}

static void loaded_add(LoadedSet *set, const char *path) {
    if (set->count + 1 > set->cap) {
        set->cap = set->cap ? set->cap * 2 : 8;
        set->paths = rl_realloc(set->paths, set->cap * sizeof(char *));
    }
    set->paths[set->count++] = rl_strdup(path);
}

static void load_file(const char *path, const char *alias, LoadedSet *loaded,
                      Program *out) {
    if (loaded_contains(loaded, path))
        return;
    loaded_add(loaded, path);

    size_t len = 0;
    char *source = rl_read_file(path, &len);
    TokenList toks = rl_lex(source, path);
    Program prog = rl_parse(&toks, path);

    char *dir = rl_dirname(path);

    for (size_t i = 0; i < prog.count; i++) {
        Decl d = prog.items[i];
        if (d.kind == DECL_IMPORT) {
            char *child = rl_path_join(dir, d.as.import.path);
            load_file(child, d.as.import.alias, loaded, out);
            free(child);
            continue;
        }
        if (d.kind == DECL_FUNC && alias)
            d.module_alias = rl_strdup(alias);
        program_push(out, d);
    }

    free(dir);

    rl_tokens_free(&toks);
    free(source);
    free(prog.items);
}

Program rl_resolve_program(const char *main_path) {
    LoadedSet loaded = {0};
    Program out = {0};
    load_file(main_path, NULL, &loaded, &out);
    for (size_t i = 0; i < loaded.count; i++)
        free(loaded.paths[i]);
    free(loaded.paths);
    return out;
}
