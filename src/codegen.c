#include "codegen.h"

typedef struct VarSym {
    char           *name;
    Type           *type;
    struct VarSym  *next;
} VarSym;

typedef struct Scope {
    VarSym        *vars;
    struct Scope  *parent;
} Scope;

typedef struct {
    Program *prog;
    Rope     header;
    Rope     body;
    Scope   *scope;
    Type    *current_return;
    int      indent;
} Gen;

static void scope_push(Gen *g) {
    Scope *s = rl_alloc(sizeof(Scope));
    s->parent = g->scope;
    g->scope = s;
}

static void scope_pop(Gen *g) {
    Scope *s = g->scope;
    VarSym *v = s->vars;
    while (v) {
        VarSym *n = v->next;
        free(v->name);
        free(v);
        v = n;
    }
    g->scope = s->parent;
    free(s);
}

static void scope_add(Gen *g, const char *name, Type *type) {
    VarSym *v = rl_alloc(sizeof(VarSym));
    v->name = rl_strdup(name);
    v->type = type;
    v->next = g->scope->vars;
    g->scope->vars = v;
}

static Type *scope_lookup(Gen *g, const char *name) {
    for (Scope *s = g->scope; s; s = s->parent)
        for (VarSym *v = s->vars; v; v = v->next)
            if (strcmp(v->name, name) == 0)
                return v->type;
    return NULL;
}

static BlueprintDecl *find_blueprint(Gen *g, const char *name) {
    for (size_t i = 0; i < g->prog->count; i++)
        if (g->prog->items[i].kind == DECL_BLUEPRINT &&
            strcmp(g->prog->items[i].as.blueprint.name, name) == 0)
            return &g->prog->items[i].as.blueprint;
    return NULL;
}

static bool find_enum_value(Gen *g, const char *name, long long *out) {
    for (size_t i = 0; i < g->prog->count; i++) {
        if (g->prog->items[i].kind != DECL_CHOICES)
            continue;
        ChoicesDecl *c = &g->prog->items[i].as.choices;
        for (size_t j = 0; j < c->entry_count; j++)
            if (strcmp(c->entries[j].name, name) == 0) {
                *out = c->entries[j].value;
                return true;
            }
    }
    return false;
}

static FuncDecl *find_func(Gen *g, const char *module, const char *name) {
    for (size_t i = 0; i < g->prog->count; i++) {
        if (g->prog->items[i].kind != DECL_FUNC)
            continue;
        FuncDecl *f = &g->prog->items[i].as.func;
        const char *falias = g->prog->items[i].module_alias;
        if (module) {
            if (falias && strcmp(falias, module) == 0 &&
                strcmp(f->name, name) == 0)
                return f;
        } else {
            if (!falias && strcmp(f->name, name) == 0)
                return f;
        }
    }
    return NULL;
}

static char *mangle_func(const char *module, const char *name) {
    Rope r;
    rope_init(&r);
    rope_push(&r, "rl_u_");
    if (module) {
        rope_push(&r, module);
        rope_push(&r, "__");
    }
    rope_push(&r, name);
    char *out = rl_strdup(r.data);
    rope_free(&r);
    return out;
}

static char *mangle_struct(const char *name) {
    Rope r;
    rope_init(&r);
    rope_push(&r, "rl_s_");
    rope_push(&r, name);
    char *out = rl_strdup(r.data);
    rope_free(&r);
    return out;
}

static void emit_type(Gen *g, Rope *out, Type *t);

static void emit_base(Rope *out, Type *t) {
    switch (t->kind) {
    case TY_VOID:   rope_push(out, "void"); break;
    case TY_INT:    rope_push(out, "int32_t"); break;
    case TY_INT8:   rope_push(out, "int8_t"); break;
    case TY_INT16:  rope_push(out, "int16_t"); break;
    case TY_INT64:  rope_push(out, "int64_t"); break;
    case TY_UINT:   rope_push(out, "uint32_t"); break;
    case TY_UINT8:  rope_push(out, "uint8_t"); break;
    case TY_UINT16: rope_push(out, "uint16_t"); break;
    case TY_UINT64: rope_push(out, "uint64_t"); break;
    case TY_FLOAT:  rope_push(out, "float"); break;
    case TY_DOUBLE: rope_push(out, "double"); break;
    case TY_CHAR:   rope_push(out, "char"); break;
    case TY_TEXT:   rope_push(out, "char*"); break;
    case TY_BOOL:   rope_push(out, "bool"); break;
    case TY_ANY:    rope_push(out, "void*"); break;
    default: break;
    }
}

static void emit_type(Gen *g, Rope *out, Type *t) {
    switch (t->kind) {
    case TY_SPAN:
        emit_type(g, out, t->inner);
        rope_push(out, "*");
        break;
    case TY_REF:
        emit_type(g, out, t->inner);
        rope_push(out, "*");
        break;
    case TY_NAMED: {
        char *m = mangle_struct(t->name);
        rope_push(out, m);
        free(m);
        break;
    }
    default:
        emit_base(out, t);
        break;
    }
}

static Type *infer(Gen *g, Expr *e);

static Type *field_type_of(Gen *g, Type *base, const char *field) {

    if (strcmp(field, "address") == 0)
        return type_ref(type_clone(base));
    if (strcmp(field, "value") == 0 && base->kind == TY_REF)
        return type_clone(base->inner);
    if (strcmp(field, "length") == 0 &&
        (base->kind == TY_SPAN || base->kind == TY_TEXT))
        return type_new(TY_INT);

    Type *bt = base;
    if (bt->kind == TY_REF)
        bt = bt->inner;
    if (bt->kind == TY_NAMED) {
        BlueprintDecl *bp = find_blueprint(g, bt->name);
        if (bp) {
            for (size_t i = 0; i < bp->field_count; i++)
                if (strcmp(bp->fields[i].name, field) == 0)
                    return type_clone(bp->fields[i].type);
        }
    }
    return type_new(TY_INT);
}

static Type *infer(Gen *g, Expr *e) {
    if (e->inferred)
        return e->inferred;
    Type *t = NULL;
    switch (e->kind) {
    case EX_INT:    t = type_new(TY_INT); break;
    case EX_FLOAT:  t = type_new(TY_FLOAT); break;
    case EX_DOUBLE: t = type_new(TY_DOUBLE); break;
    case EX_STRING: t = type_new(TY_TEXT); break;
    case EX_CHAR:   t = type_new(TY_CHAR); break;
    case EX_BOOL:   t = type_new(TY_BOOL); break;
    case EX_NIL:    t = type_new(TY_ANY); break;
    case EX_IDENT: {
        long long ev;
        Type *vt = scope_lookup(g, e->sval);
        if (vt)
            t = type_clone(vt);
        else if (find_enum_value(g, e->sval, &ev))
            t = type_new(TY_INT);
        else
            t = type_new(TY_INT);
        break;
    }
    case EX_BINARY:
        switch (e->op) {
        case OP_EQ: case OP_NEQ: case OP_LT: case OP_GT:
        case OP_LE: case OP_GE: case OP_AND: case OP_OR:
            t = type_new(TY_BOOL);
            break;
        default: {
            Type *lt = infer(g, e->lhs);

            t = type_clone(lt);
            break;
        }
        }
        break;
    case EX_UNARY:
        t = (e->op == OP_NOT) ? type_new(TY_BOOL) : type_clone(infer(g, e->rhs));
        break;
    case EX_ASSIGN:
        t = type_clone(infer(g, e->lhs));
        break;
    case EX_CALL: {
        FuncDecl *f = find_func(g, NULL, e->callee);
        t = f ? type_clone(f->return_type) : type_new(TY_INT);
        break;
    }
    case EX_MODULE_CALL: {
        FuncDecl *f = find_func(g, e->module, e->callee);
        t = f ? type_clone(f->return_type) : type_new(TY_INT);
        break;
    }
    case EX_INDEX: {
        Type *bt = infer(g, e->base);
        if (bt->kind == TY_SPAN)
            t = type_clone(bt->inner);
        else if (bt->kind == TY_TEXT)
            t = type_new(TY_CHAR);
        else
            t = type_new(TY_INT);
        break;
    }
    case EX_FIELD:
        t = field_type_of(g, infer(g, e->base), e->field);
        break;
    case EX_ARRAY_LIT:
        if (e->elems.count > 0)
            t = type_span(type_clone(infer(g, e->elems.items[0])));
        else
            t = type_span(type_new(TY_INT));
        break;
    case EX_CAST:
        t = type_clone(e->cast_type);
        break;
    case EX_SIZEOF:
        t = type_new(TY_INT);
        break;
    default:
        t = type_new(TY_INT);
        break;
    }
    e->inferred = t;
    return t;
}

static void gen_expr(Gen *g, Rope *out, Expr *e);

static void gen_string_literal(Rope *out, const char *s) {
    rope_pushc(out, '"');
    for (const char *p = s; *p; p++) {
        switch (*p) {
        case '\n': rope_push(out, "\\n"); break;
        case '\t': rope_push(out, "\\t"); break;
        case '\r': rope_push(out, "\\r"); break;
        case '\\': rope_push(out, "\\\\"); break;
        case '"':  rope_push(out, "\\\""); break;
        default:   rope_pushc(out, *p); break;
        }
    }
    rope_pushc(out, '"');
}

static const char *binop_c(OpKind op) {
    switch (op) {
    case OP_ADD: return "+";
    case OP_SUB: return "-";
    case OP_MUL: return "*";
    case OP_DIV: return "/";
    case OP_MOD: return "%";
    case OP_EQ:  return "==";
    case OP_NEQ: return "!=";
    case OP_LT:  return "<";
    case OP_GT:  return ">";
    case OP_LE:  return "<=";
    case OP_GE:  return ">=";
    case OP_AND: return "&&";
    case OP_OR:  return "||";
    default:     return "?";
    }
}

static void gen_lvalue(Gen *g, Rope *out, Expr *e) {
    gen_expr(g, out, e);
}

static void gen_call_args(Gen *g, Rope *out, ExprList *args) {
    rope_pushc(out, '(');
    for (size_t i = 0; i < args->count; i++) {
        if (i)
            rope_push(out, ", ");
        gen_expr(g, out, args->items[i]);
    }
    rope_pushc(out, ')');
}

static void gen_field(Gen *g, Rope *out, Expr *e) {
    Type *bt = infer(g, e->base);

    if (strcmp(e->field, "address") == 0) {
        rope_push(out, "(&(");
        gen_expr(g, out, e->base);
        rope_push(out, "))");
        return;
    }
    if (strcmp(e->field, "value") == 0 && bt->kind == TY_REF) {
        rope_push(out, "(*(");
        gen_expr(g, out, e->base);
        rope_push(out, "))");
        return;
    }
    if (strcmp(e->field, "length") == 0) {
        if (bt->kind == TY_TEXT) {
            rope_push(out, "((int32_t)rt_text_len(");
            gen_expr(g, out, e->base);
            rope_push(out, "))");
            return;
        }
        if (bt->kind == TY_SPAN) {
            rope_push(out, "rt_span_len(");
            gen_expr(g, out, e->base);
            rope_push(out, ")");
            return;
        }
    }

    rope_pushc(out, '(');
    gen_expr(g, out, e->base);
    if (bt->kind == TY_REF)
        rope_push(out, ")->");
    else
        rope_push(out, ").");
    rope_push(out, e->field);
}

static void gen_expr(Gen *g, Rope *out, Expr *e) {
    switch (e->kind) {
    case EX_INT:
        rope_printf(out, "%lld", e->ival);
        break;
    case EX_FLOAT: {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.9g", e->fval);
        rope_push(out, buf);

        if (!strpbrk(buf, ".eEnN"))
            rope_push(out, ".0");
        rope_push(out, "f");
        break;
    }
    case EX_DOUBLE: {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.17g", e->fval);
        rope_push(out, buf);
        if (!strpbrk(buf, ".eEnN"))
            rope_push(out, ".0");
        break;
    }
    case EX_STRING:
        gen_string_literal(out, e->sval);
        break;
    case EX_CHAR:
        rope_printf(out, "%d", (int)e->cval);
        break;
    case EX_BOOL:
        rope_push(out, e->bval ? "true" : "false");
        break;
    case EX_NIL:
        rope_push(out, "NULL");
        break;
    case EX_IDENT: {
        long long ev;
        if (!scope_lookup(g, e->sval) && find_enum_value(g, e->sval, &ev))
            rope_printf(out, "%lld", ev);
        else
            rope_push(out, e->sval);
        break;
    }
    case EX_BINARY: {
        Type *lt = infer(g, e->lhs);
        Type *rt = infer(g, e->rhs);

        if (e->op == OP_ADD && lt->kind == TY_TEXT && rt->kind == TY_TEXT) {
            rope_push(out, "rt_text_concat(");
            gen_expr(g, out, e->lhs);
            rope_push(out, ", ");
            gen_expr(g, out, e->rhs);
            rope_push(out, ")");
            break;
        }

        if ((e->op == OP_EQ || e->op == OP_NEQ) && lt->kind == TY_TEXT &&
            rt->kind == TY_TEXT) {
            if (e->op == OP_NEQ)
                rope_push(out, "(!");
            rope_push(out, "rt_text_equal(");
            gen_expr(g, out, e->lhs);
            rope_push(out, ", ");
            gen_expr(g, out, e->rhs);
            rope_push(out, ")");
            if (e->op == OP_NEQ)
                rope_push(out, ")");
            break;
        }
        rope_pushc(out, '(');
        gen_expr(g, out, e->lhs);
        rope_printf(out, " %s ", binop_c(e->op));
        gen_expr(g, out, e->rhs);
        rope_pushc(out, ')');
        break;
    }
    case EX_UNARY:
        rope_pushc(out, '(');
        rope_push(out, e->op == OP_NEG ? "-" : "!");
        gen_expr(g, out, e->rhs);
        rope_pushc(out, ')');
        break;
    case EX_ASSIGN: {
        rope_pushc(out, '(');
        gen_lvalue(g, out, e->lhs);
        switch (e->op) {
        case OP_ASSIGN:     rope_push(out, " = "); break;
        case OP_ADD_ASSIGN: rope_push(out, " += "); break;
        case OP_SUB_ASSIGN: rope_push(out, " -= "); break;
        case OP_MUL_ASSIGN: rope_push(out, " *= "); break;
        case OP_DIV_ASSIGN: rope_push(out, " /= "); break;
        default: break;
        }
        gen_expr(g, out, e->rhs);
        rope_pushc(out, ')');
        break;
    }
    case EX_CALL: {
        FuncDecl *f = find_func(g, NULL, e->callee);
        if (f && f->is_native) {
            rope_push(out, f->native_name);
        } else {
            char *m = mangle_func(NULL, e->callee);
            rope_push(out, m);
            free(m);
        }
        gen_call_args(g, out, &e->args);
        break;
    }
    case EX_MODULE_CALL: {
        FuncDecl *f = find_func(g, e->module, e->callee);
        if (f && f->is_native) {
            rope_push(out, f->native_name);
        } else {
            char *m = mangle_func(e->module, e->callee);
            rope_push(out, m);
            free(m);
        }
        gen_call_args(g, out, &e->args);
        break;
    }
    case EX_INDEX:
        gen_expr(g, out, e->base);
        rope_pushc(out, '[');
        gen_expr(g, out, e->index);
        rope_pushc(out, ']');
        break;
    case EX_FIELD:
        gen_field(g, out, e);
        break;
    case EX_ARRAY_LIT: {

        Type *et = e->elems.count ? infer(g, e->elems.items[0])
                                  : type_new(TY_INT);
        Rope tr;
        rope_init(&tr);
        emit_type(g, &tr, et);
        if (e->elems.count == 0) {
            rope_printf(out, "rt_span_make(sizeof(%s), 0, NULL)", tr.data);
            rope_free(&tr);
            break;
        }
        rope_printf(out, "rt_span_make(sizeof(%s), %zu, (%s[]){",
                    tr.data, e->elems.count, tr.data);
        for (size_t i = 0; i < e->elems.count; i++) {
            if (i)
                rope_push(out, ", ");
            gen_expr(g, out, e->elems.items[i]);
        }
        rope_push(out, "})");
        rope_free(&tr);
        break;
    }
    case EX_CAST: {
        Rope tr;
        rope_init(&tr);
        emit_type(g, &tr, e->cast_type);
        Type *st = infer(g, e->lhs);

        if (st->kind == TY_TEXT &&
            (e->cast_type->kind == TY_INT || e->cast_type->kind == TY_INT64)) {
            rope_push(out, "rt_text_to_int(");
            gen_expr(g, out, e->lhs);
            rope_push(out, ")");
        } else {
            rope_printf(out, "((%s)(", tr.data);
            gen_expr(g, out, e->lhs);
            rope_push(out, "))");
        }
        rope_free(&tr);
        break;
    }
    case EX_SIZEOF: {
        Rope tr;
        rope_init(&tr);
        emit_type(g, &tr, e->cast_type);
        rope_printf(out, "((int32_t)sizeof(%s))", tr.data);
        rope_free(&tr);
        break;
    }
    default:
        rope_push(out, "0");
        break;
    }
}

static void gen_stmt(Gen *g, Rope *out, Stmt *s);

static void emit_indent(Gen *g, Rope *out) {
    for (int i = 0; i < g->indent; i++)
        rope_push(out, "    ");
}

static void gen_block(Gen *g, Rope *out, StmtList *body) {
    rope_push(out, "{\n");
    g->indent++;
    scope_push(g);
    for (size_t i = 0; i < body->count; i++)
        gen_stmt(g, out, body->items[i]);
    scope_pop(g);
    g->indent--;
    emit_indent(g, out);
    rope_push(out, "}");
}

static void gen_default_init(Gen *g, Rope *out, Type *t) {
    switch (t->kind) {
    case TY_TEXT:
        rope_push(out, "\"\"");
        break;
    case TY_BOOL:
        rope_push(out, "false");
        break;
    case TY_FLOAT:
        rope_push(out, "0.0f");
        break;
    case TY_DOUBLE:
        rope_push(out, "0.0");
        break;
    case TY_SPAN:
    case TY_REF:
    case TY_ANY:
        rope_push(out, "NULL");
        break;
    case TY_NAMED:
        rope_push(out, "{0}");
        break;
    default:
        rope_push(out, "0");
        break;
    }
}

static void gen_var_decl(Gen *g, Rope *out, Stmt *s) {
    Rope tr;
    rope_init(&tr);
    emit_type(g, &tr, s->decl_type);
    if (s->decl_const)
        rope_push(out, "const ");
    rope_printf(out, "%s %s", tr.data, s->decl_name);
    rope_free(&tr);
    rope_push(out, " = ");
    if (s->decl_init)
        gen_expr(g, out, s->decl_init);
    else
        gen_default_init(g, out, s->decl_type);
    scope_add(g, s->decl_name, s->decl_type);
}

static void gen_stmt(Gen *g, Rope *out, Stmt *s) {
    switch (s->kind) {
    case ST_VAR_DECL:
        emit_indent(g, out);
        gen_var_decl(g, out, s);
        rope_push(out, ";\n");
        break;
    case ST_EXPR:
        emit_indent(g, out);
        gen_expr(g, out, s->expr);
        rope_push(out, ";\n");
        break;
    case ST_GIVE:
        emit_indent(g, out);
        if (s->expr) {
            rope_push(out, "return ");
            gen_expr(g, out, s->expr);
            rope_push(out, ";\n");
        } else {
            rope_push(out, "return;\n");
        }
        break;
    case ST_WHEN:
        emit_indent(g, out);
        rope_push(out, "if (");
        gen_expr(g, out, s->cond);
        rope_push(out, ") ");
        gen_block(g, out, &s->then_body);
        if (s->has_else) {
            rope_push(out, " else ");

            if (s->else_body.count == 1 &&
                s->else_body.items[0]->kind == ST_WHEN) {

                Stmt *chained = s->else_body.items[0];
                rope_push(out, "if (");
                gen_expr(g, out, chained->cond);
                rope_push(out, ") ");
                gen_block(g, out, &chained->then_body);
                if (chained->has_else) {
                    rope_push(out, " else ");
                    gen_block(g, out, &chained->else_body);
                }
            } else {
                gen_block(g, out, &s->else_body);
            }
        }
        rope_push(out, "\n");
        break;
    case ST_LOOP:
        emit_indent(g, out);
        rope_push(out, "while (");
        gen_expr(g, out, s->cond);
        rope_push(out, ") ");
        gen_block(g, out, &s->body);
        rope_push(out, "\n");
        break;
    case ST_WALK:
        emit_indent(g, out);
        scope_push(g);
        rope_push(out, "for (");

        {
            Rope tr;
            rope_init(&tr);
            emit_type(g, &tr, s->walk_init->decl_type);
            rope_printf(out, "%s %s = ", tr.data, s->walk_init->decl_name);
            rope_free(&tr);
            if (s->walk_init->decl_init)
                gen_expr(g, out, s->walk_init->decl_init);
            else
                gen_default_init(g, out, s->walk_init->decl_type);
            scope_add(g, s->walk_init->decl_name, s->walk_init->decl_type);
        }
        rope_push(out, "; ");
        gen_expr(g, out, s->cond);
        rope_push(out, "; ");
        gen_expr(g, out, s->walk_step);
        rope_push(out, ") ");
        gen_block(g, out, &s->body);
        scope_pop(g);
        rope_push(out, "\n");
        break;
    case ST_BLOCK:
        emit_indent(g, out);
        gen_block(g, out, &s->body);
        rope_push(out, "\n");
        break;
    case ST_STOP:
        emit_indent(g, out);
        rope_push(out, "break;\n");
        break;
    case ST_SKIP:
        emit_indent(g, out);
        rope_push(out, "continue;\n");
        break;
    }
}

static void gen_func_signature(Gen *g, Rope *out, Decl *d) {
    FuncDecl *f = &d->as.func;
    Rope rt;
    rope_init(&rt);
    emit_type(g, &rt, f->return_type);
    char *name = mangle_func(d->module_alias, f->name);
    rope_printf(out, "%s %s(", rt.data, name);
    free(name);
    rope_free(&rt);
    if (f->param_count == 0) {
        rope_push(out, "void");
    } else {
        for (size_t i = 0; i < f->param_count; i++) {
            if (i)
                rope_push(out, ", ");
            Rope pt;
            rope_init(&pt);
            emit_type(g, &pt, f->params[i].type);
            rope_printf(out, "%s %s", pt.data, f->params[i].name);
            rope_free(&pt);
        }
    }
    rope_pushc(out, ')');
}

static void gen_blueprint(Gen *g, Rope *out, BlueprintDecl *bp) {
    char *name = mangle_struct(bp->name);
    rope_printf(out, "typedef struct %s {\n", name);
    for (size_t i = 0; i < bp->field_count; i++) {
        Rope ft;
        rope_init(&ft);
        emit_type(g, &ft, bp->fields[i].type);
        rope_printf(out, "    %s %s;\n", ft.data, bp->fields[i].name);
        rope_free(&ft);
    }
    rope_printf(out, "} %s;\n\n", name);
    free(name);
}

char *rl_generate_c(Program *program) {
    Gen g = {0};
    g.prog = program;
    rope_init(&g.header);
    rope_init(&g.body);
    scope_push(&g);

    rope_push(&g.header, "#include \"rootrt.h\"\n\n");

    for (size_t i = 0; i < program->count; i++)
        if (program->items[i].kind == DECL_BLUEPRINT)
            gen_blueprint(&g, &g.header, &program->items[i].as.blueprint);

    for (size_t i = 0; i < program->count; i++) {
        Decl *d = &program->items[i];
        if (d->kind != DECL_FUNC || d->as.func.is_native)
            continue;
        gen_func_signature(&g, &g.header, d);
        rope_push(&g.header, ";\n");
    }
    rope_push(&g.header, "\n");

    for (size_t i = 0; i < program->count; i++) {
        Decl *d = &program->items[i];
        if (d->kind != DECL_FUNC || d->as.func.is_native)
            continue;
        FuncDecl *f = &d->as.func;

        gen_func_signature(&g, &g.body, d);
        rope_push(&g.body, " {\n");
        g.indent = 1;
        scope_push(&g);
        for (size_t pi = 0; pi < f->param_count; pi++)
            scope_add(&g, f->params[pi].name, f->params[pi].type);
        g.current_return = f->return_type;
        for (size_t si = 0; si < f->body.count; si++)
            gen_stmt(&g, &g.body, f->body.items[si]);
        scope_pop(&g);
        rope_push(&g.body, "}\n\n");
    }

    rope_push(&g.body,
              "int main(int rl_argc, char **rl_argv) {\n"
              "    (void)rl_argc; (void)rl_argv;\n"
              "    rl_u_main();\n"
              "    return 0;\n"
              "}\n");

    Rope full;
    rope_init(&full);
    rope_push(&full, g.header.data);
    rope_push(&full, g.body.data);
    char *result = rl_strdup(full.data);

    rope_free(&full);
    rope_free(&g.header);
    rope_free(&g.body);
    scope_pop(&g);
    return result;
}
