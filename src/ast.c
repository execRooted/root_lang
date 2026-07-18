#include "ast.h"

Type *type_new(TypeKind kind) {
    Type *t = rl_alloc(sizeof(Type));
    t->kind = kind;
    t->inner = NULL;
    t->name = NULL;
    return t;
}

Type *type_span(Type *inner) {
    Type *t = type_new(TY_SPAN);
    t->inner = inner;
    return t;
}

Type *type_ref(Type *inner) {
    Type *t = type_new(TY_REF);
    t->inner = inner;
    return t;
}

Type *type_named(const char *name) {
    Type *t = type_new(TY_NAMED);
    t->name = rl_strdup(name);
    return t;
}

Type *type_clone(const Type *t) {
    if (!t)
        return NULL;
    Type *c = type_new(t->kind);
    if (t->inner)
        c->inner = type_clone(t->inner);
    if (t->name)
        c->name = rl_strdup(t->name);
    return c;
}

bool type_equal(const Type *a, const Type *b) {
    if (!a || !b)
        return a == b;
    if (a->kind != b->kind)
        return false;
    if (a->kind == TY_NAMED)
        return strcmp(a->name, b->name) == 0;
    if (a->kind == TY_SPAN || a->kind == TY_REF)
        return type_equal(a->inner, b->inner);
    return true;
}

Expr *expr_new(ExprKind kind, int line, int col) {
    Expr *e = rl_alloc(sizeof(Expr));
    e->kind = kind;
    e->line = line;
    e->col = col;
    return e;
}

Stmt *stmt_new(StmtKind kind, int line, int col) {
    Stmt *s = rl_alloc(sizeof(Stmt));
    s->kind = kind;
    s->line = line;
    s->col = col;
    return s;
}

void stmt_list_push(StmtList *list, Stmt *s) {
    if (list->count + 1 > list->cap) {
        list->cap = list->cap ? list->cap * 2 : 8;
        list->items = rl_realloc(list->items, list->cap * sizeof(Stmt *));
    }
    list->items[list->count++] = s;
}

void program_push(Program *p, Decl decl) {
    if (p->count + 1 > p->cap) {
        p->cap = p->cap ? p->cap * 2 : 16;
        p->items = rl_realloc(p->items, p->cap * sizeof(Decl));
    }
    p->items[p->count++] = decl;
}
