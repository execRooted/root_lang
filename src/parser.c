#include "parser.h"

typedef struct {
    Token      *toks;
    size_t      count;
    size_t      pos;
    const char *file;
} Parser;

static Token *peek(Parser *p) { return &p->toks[p->pos]; }
static Token *prev(Parser *p) { return &p->toks[p->pos - 1]; }
static bool at_end(Parser *p) { return peek(p)->kind == TK_EOF; }

static Token *advance(Parser *p) {
    if (!at_end(p))
        p->pos++;
    return prev(p);
}

static bool check(Parser *p, TokKind k) { return peek(p)->kind == k; }

static bool match(Parser *p, TokKind k) {
    if (check(p, k)) {
        advance(p);
        return true;
    }
    return false;
}

static Token *expect(Parser *p, TokKind k, const char *what) {
    if (check(p, k))
        return advance(p);
    Token *t = peek(p);
    rl_fatal(p->file, t->line, t->col, "expected %s but found '%s'", what,
             t->lexeme[0] ? t->lexeme : rl_token_name(t->kind));
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Types                                                              */
/* ------------------------------------------------------------------ */

static bool is_base_type_token(TokKind k) {
    switch (k) {
    case TK_KW_INT: case TK_KW_INT8: case TK_KW_INT16: case TK_KW_INT64:
    case TK_KW_UINT: case TK_KW_UINT8: case TK_KW_UINT16: case TK_KW_UINT64:
    case TK_KW_FLOAT: case TK_KW_DOUBLE: case TK_KW_CHAR: case TK_KW_TEXT:
    case TK_KW_BOOL: case TK_KW_VOID: case TK_KW_ANY:
        return true;
    default:
        return false;
    }
}

static Type *base_type_from(TokKind k) {
    switch (k) {
    case TK_KW_INT:    return type_new(TY_INT);
    case TK_KW_INT8:   return type_new(TY_INT8);
    case TK_KW_INT16:  return type_new(TY_INT16);
    case TK_KW_INT64:  return type_new(TY_INT64);
    case TK_KW_UINT:   return type_new(TY_UINT);
    case TK_KW_UINT8:  return type_new(TY_UINT8);
    case TK_KW_UINT16: return type_new(TY_UINT16);
    case TK_KW_UINT64: return type_new(TY_UINT64);
    case TK_KW_FLOAT:  return type_new(TY_FLOAT);
    case TK_KW_DOUBLE: return type_new(TY_DOUBLE);
    case TK_KW_CHAR:   return type_new(TY_CHAR);
    case TK_KW_TEXT:   return type_new(TY_TEXT);
    case TK_KW_BOOL:   return type_new(TY_BOOL);
    case TK_KW_VOID:   return type_new(TY_VOID);
    case TK_KW_ANY:    return type_new(TY_ANY);
    default:           return NULL;
    }
}

/*
 * A type is a base type or blueprint name, optionally followed by any number
 * of `[]` (span) or `*` (ref) suffixes. `int[]` is a span of ints, `int*` a
 * reference to an int.
 */
static Type *parse_type(Parser *p) {
    Type *t = NULL;
    Token *tok = peek(p);

    if (is_base_type_token(tok->kind)) {
        advance(p);
        t = base_type_from(tok->kind);
    } else if (check(p, TK_IDENT)) {
        advance(p);
        t = type_named(tok->lexeme);
    } else {
        rl_fatal(p->file, tok->line, tok->col, "expected a type name");
    }

    for (;;) {
        if (match(p, TK_LBRACKET)) {
            expect(p, TK_RBRACKET, "']'");
            t = type_span(t);
        } else if (match(p, TK_STAR)) {
            t = type_ref(t);
        } else {
            break;
        }
    }
    return t;
}

/* ------------------------------------------------------------------ */
/* Expressions (precedence climbing)                                  */
/* ------------------------------------------------------------------ */

static Expr *parse_expr(Parser *p);

static ExprList parse_args(Parser *p) {
    ExprList list = {0};
    size_t cap = 0;
    expect(p, TK_LPAREN, "'('");
    if (!check(p, TK_RPAREN)) {
        do {
            Expr *e = parse_expr(p);
            if (list.count + 1 > cap) {
                cap = cap ? cap * 2 : 4;
                list.items = rl_realloc(list.items, cap * sizeof(Expr *));
            }
            list.items[list.count++] = e;
        } while (match(p, TK_COMMA));
    }
    expect(p, TK_RPAREN, "')'");
    return list;
}

static Expr *parse_primary(Parser *p) {
    Token *t = peek(p);
    switch (t->kind) {
    case TK_INT_LIT: {
        advance(p);
        Expr *e = expr_new(EX_INT, t->line, t->col);
        e->ival = strtoll(t->lexeme, NULL, 10);
        return e;
    }
    case TK_FLOAT_LIT: {
        advance(p);
        Expr *e = expr_new(EX_FLOAT, t->line, t->col);
        e->fval = strtod(t->lexeme, NULL);
        return e;
    }
    case TK_DOUBLE_LIT: {
        advance(p);
        Expr *e = expr_new(EX_DOUBLE, t->line, t->col);
        e->fval = strtod(t->lexeme, NULL);
        return e;
    }
    case TK_STRING_LIT: {
        advance(p);
        Expr *e = expr_new(EX_STRING, t->line, t->col);
        e->sval = rl_strdup(t->lexeme);
        return e;
    }
    case TK_CHAR_LIT: {
        advance(p);
        Expr *e = expr_new(EX_CHAR, t->line, t->col);
        e->cval = t->lexeme[0];
        return e;
    }
    case TK_BOOL_LIT: {
        advance(p);
        Expr *e = expr_new(EX_BOOL, t->line, t->col);
        e->bval = strcmp(t->lexeme, "yes") == 0;
        return e;
    }
    case TK_KW_NIL: {
        advance(p);
        return expr_new(EX_NIL, t->line, t->col);
    }
    case TK_KW_SIZEOF: {
        advance(p);
        expect(p, TK_LPAREN, "'('");
        Expr *e = expr_new(EX_SIZEOF, t->line, t->col);
        e->cast_type = parse_type(p);
        expect(p, TK_RPAREN, "')'");
        return e;
    }
    case TK_LPAREN: {
        advance(p);
        Expr *e = parse_expr(p);
        expect(p, TK_RPAREN, "')'");
        return e;
    }
    case TK_LBRACE: {
        advance(p);
        Expr *e = expr_new(EX_ARRAY_LIT, t->line, t->col);
        size_t cap = 0;
        if (!check(p, TK_RBRACE)) {
            do {
                Expr *el = parse_expr(p);
                if (e->elems.count + 1 > cap) {
                    cap = cap ? cap * 2 : 4;
                    e->elems.items =
                        rl_realloc(e->elems.items, cap * sizeof(Expr *));
                }
                e->elems.items[e->elems.count++] = el;
            } while (match(p, TK_COMMA));
        }
        expect(p, TK_RBRACE, "'}'");
        return e;
    }
    case TK_IDENT: {
        advance(p);
        /* module call: alias.func(...) */
        if (check(p, TK_DOT) && p->toks[p->pos + 1].kind == TK_IDENT &&
            p->toks[p->pos + 2].kind == TK_LPAREN) {
            advance(p); /* dot */
            Token *fn = advance(p);
            Expr *e = expr_new(EX_MODULE_CALL, t->line, t->col);
            e->module = rl_strdup(t->lexeme);
            e->callee = rl_strdup(fn->lexeme);
            e->args = parse_args(p);
            return e;
        }
        /* plain call: name(...) */
        if (check(p, TK_LPAREN)) {
            Expr *e = expr_new(EX_CALL, t->line, t->col);
            e->callee = rl_strdup(t->lexeme);
            e->args = parse_args(p);
            return e;
        }
        Expr *e = expr_new(EX_IDENT, t->line, t->col);
        e->sval = rl_strdup(t->lexeme);
        return e;
    }
    default:
        rl_fatal(p->file, t->line, t->col, "unexpected '%s' in expression",
                 t->lexeme[0] ? t->lexeme : rl_token_name(t->kind));
        return NULL;
    }
}

/* postfix: field access, indexing */
static Expr *parse_postfix(Parser *p) {
    Expr *e = parse_primary(p);
    for (;;) {
        if (match(p, TK_DOT)) {
            Token *name = expect(p, TK_IDENT, "field name");
            Expr *fe = expr_new(EX_FIELD, name->line, name->col);
            fe->base = e;
            fe->field = rl_strdup(name->lexeme);
            /* Allow method-style `.address()` / `.length()` empty calls. */
            if (check(p, TK_LPAREN)) {
                advance(p);
                expect(p, TK_RPAREN, "')'");
            }
            e = fe;
        } else if (match(p, TK_LBRACKET)) {
            Expr *ie = expr_new(EX_INDEX, e->line, e->col);
            ie->base = e;
            ie->index = parse_expr(p);
            expect(p, TK_RBRACKET, "']'");
            e = ie;
        } else {
            break;
        }
    }
    return e;
}

static Expr *parse_unary(Parser *p) {
    Token *t = peek(p);
    if (check(p, TK_MINUS) || check(p, TK_NOT)) {
        advance(p);
        Expr *e = expr_new(EX_UNARY, t->line, t->col);
        e->op = (t->kind == TK_MINUS) ? OP_NEG : OP_NOT;
        e->rhs = parse_unary(p);
        return e;
    }
    return parse_postfix(p);
}

/* cast: unary `to` Type */
static Expr *parse_cast(Parser *p) {
    Expr *e = parse_unary(p);
    while (check(p, TK_KW_TO)) {
        Token *t = advance(p);
        Expr *c = expr_new(EX_CAST, t->line, t->col);
        c->lhs = e;
        c->cast_type = parse_type(p);
        e = c;
    }
    return e;
}

static Expr *make_binary(OpKind op, Expr *l, Expr *r) {
    Expr *e = expr_new(EX_BINARY, l->line, l->col);
    e->op = op;
    e->lhs = l;
    e->rhs = r;
    return e;
}

static Expr *parse_factor(Parser *p) {
    Expr *e = parse_cast(p);
    for (;;) {
        if (match(p, TK_STAR)) e = make_binary(OP_MUL, e, parse_cast(p));
        else if (match(p, TK_SLASH)) e = make_binary(OP_DIV, e, parse_cast(p));
        else if (match(p, TK_PERCENT)) e = make_binary(OP_MOD, e, parse_cast(p));
        else break;
    }
    return e;
}

static Expr *parse_term(Parser *p) {
    Expr *e = parse_factor(p);
    for (;;) {
        if (match(p, TK_PLUS)) e = make_binary(OP_ADD, e, parse_factor(p));
        else if (match(p, TK_MINUS)) e = make_binary(OP_SUB, e, parse_factor(p));
        else break;
    }
    return e;
}

static Expr *parse_comparison(Parser *p) {
    Expr *e = parse_term(p);
    for (;;) {
        if (match(p, TK_LT)) e = make_binary(OP_LT, e, parse_term(p));
        else if (match(p, TK_GT)) e = make_binary(OP_GT, e, parse_term(p));
        else if (match(p, TK_LE)) e = make_binary(OP_LE, e, parse_term(p));
        else if (match(p, TK_GE)) e = make_binary(OP_GE, e, parse_term(p));
        else break;
    }
    return e;
}

static Expr *parse_equality(Parser *p) {
    Expr *e = parse_comparison(p);
    for (;;) {
        if (match(p, TK_EQ)) e = make_binary(OP_EQ, e, parse_comparison(p));
        else if (match(p, TK_NEQ)) e = make_binary(OP_NEQ, e, parse_comparison(p));
        else break;
    }
    return e;
}

static Expr *parse_logic_and(Parser *p) {
    Expr *e = parse_equality(p);
    while (match(p, TK_AND))
        e = make_binary(OP_AND, e, parse_equality(p));
    return e;
}

static Expr *parse_logic_or(Parser *p) {
    Expr *e = parse_logic_and(p);
    while (match(p, TK_OR))
        e = make_binary(OP_OR, e, parse_logic_and(p));
    return e;
}

/* Assignment is right associative and lowest precedence. */
static Expr *parse_expr(Parser *p) {
    Expr *e = parse_logic_or(p);
    OpKind op;
    switch (peek(p)->kind) {
    case TK_ASSIGN:   op = OP_ASSIGN; break;
    case TK_PLUS_EQ:  op = OP_ADD_ASSIGN; break;
    case TK_MINUS_EQ: op = OP_SUB_ASSIGN; break;
    case TK_STAR_EQ:  op = OP_MUL_ASSIGN; break;
    case TK_SLASH_EQ: op = OP_DIV_ASSIGN; break;
    default:
        return e;
    }
    Token *t = advance(p);
    Expr *rhs = parse_expr(p);
    Expr *a = expr_new(EX_ASSIGN, t->line, t->col);
    a->op = op;
    a->lhs = e;
    a->rhs = rhs;
    return a;
}

/* ------------------------------------------------------------------ */
/* Statements                                                         */
/* ------------------------------------------------------------------ */

static StmtList parse_block(Parser *p);
static Stmt *parse_stmt(Parser *p);

static Stmt *parse_var_decl(Parser *p) {
    int line = peek(p)->line, col = peek(p)->col;
    Type *type = parse_type(p);
    Token *name = expect(p, TK_IDENT, "variable name");
    Stmt *s = stmt_new(ST_VAR_DECL, line, col);
    s->decl_type = type;
    s->decl_name = rl_strdup(name->lexeme);
    if (match(p, TK_ASSIGN))
        s->decl_init = parse_expr(p);
    /* Optional `const` modifier after the initializer. */
    if (match(p, TK_KW_CONST))
        s->decl_const = true;
    return s;
}

/*
 * Distinguish a variable declaration (`Type name ...`) from an expression
 * statement. A declaration begins with a base type keyword, or with an
 * identifier that is immediately followed by another identifier (or by a
 * span/ref suffix and then an identifier).
 */
static bool starts_var_decl(Parser *p) {
    if (is_base_type_token(peek(p)->kind))
        return true;
    if (!check(p, TK_IDENT))
        return false;
    size_t i = p->pos + 1;
    /* skip [] and * suffixes */
    while (p->toks[i].kind == TK_LBRACKET || p->toks[i].kind == TK_STAR) {
        if (p->toks[i].kind == TK_LBRACKET) {
            if (p->toks[i + 1].kind != TK_RBRACKET)
                return false;
            i += 2;
        } else {
            i += 1;
        }
    }
    return p->toks[i].kind == TK_IDENT;
}

static Stmt *parse_when(Parser *p) {
    Token *kw = advance(p); /* if */
    expect(p, TK_LPAREN, "'('");
    Expr *cond = parse_expr(p);
    expect(p, TK_RPAREN, "')'");
    Stmt *s = stmt_new(ST_WHEN, kw->line, kw->col);
    s->cond = cond;
    s->then_body = parse_block(p);

    if (check(p, TK_KW_ORELSE)) {
        /* `else if (...)` chains into a nested conditional; a bare `else`
           takes a block. */
        if (p->toks[p->pos + 1].kind == TK_KW_WHEN) {
            advance(p); /* consume 'else' */
            Stmt *chained = parse_when(p);
            stmt_list_push(&s->else_body, chained);
            s->has_else = true;
        } else {
            advance(p); /* consume 'else' */
            s->else_body = parse_block(p);
            s->has_else = true;
        }
    }
    return s;
}

static Stmt *parse_stmt(Parser *p) {
    Token *t = peek(p);

    if (check(p, TK_LBRACE)) {
        Stmt *s = stmt_new(ST_BLOCK, t->line, t->col);
        s->body = parse_block(p);
        return s;
    }
    if (check(p, TK_KW_WHEN)) {
        return parse_when(p);
    }
    if (check(p, TK_KW_LOOP)) {
        advance(p);
        expect(p, TK_LPAREN, "'('");
        Expr *cond = parse_expr(p);
        expect(p, TK_RPAREN, "')'");
        Stmt *s = stmt_new(ST_LOOP, t->line, t->col);
        s->cond = cond;
        s->body = parse_block(p);
        return s;
    }
    if (check(p, TK_KW_WALK)) {
        advance(p);
        expect(p, TK_LPAREN, "'('");
        Stmt *s = stmt_new(ST_WALK, t->line, t->col);
        /* init , condition , step  (comma separated, matching the spec) */
        s->walk_init = parse_var_decl(p);
        expect(p, TK_COMMA, "','");
        s->cond = parse_expr(p);
        expect(p, TK_COMMA, "','");
        s->walk_step = parse_expr(p);
        expect(p, TK_RPAREN, "')'");
        s->body = parse_block(p);
        return s;
    }
    if (check(p, TK_KW_GIVE)) {
        advance(p);
        Stmt *s = stmt_new(ST_GIVE, t->line, t->col);
        if (!check(p, TK_SEMI))
            s->expr = parse_expr(p);
        expect(p, TK_SEMI, "';'");
        return s;
    }
    if (check(p, TK_KW_STOP)) {
        advance(p);
        expect(p, TK_SEMI, "';'");
        return stmt_new(ST_STOP, t->line, t->col);
    }
    if (check(p, TK_KW_SKIP)) {
        advance(p);
        expect(p, TK_SEMI, "';'");
        return stmt_new(ST_SKIP, t->line, t->col);
    }
    if (check(p, TK_KW_LET) || check(p, TK_KW_CONST)) {
        bool is_const = check(p, TK_KW_CONST);
        advance(p);
        Stmt *s = parse_var_decl(p);
        if (is_const)
            s->decl_const = true;
        expect(p, TK_SEMI, "';'");
        return s;
    }
    if (starts_var_decl(p)) {
        Stmt *s = parse_var_decl(p);
        expect(p, TK_SEMI, "';'");
        return s;
    }

    /* expression statement */
    Stmt *s = stmt_new(ST_EXPR, t->line, t->col);
    s->expr = parse_expr(p);
    expect(p, TK_SEMI, "';'");
    return s;
}

static StmtList parse_block(Parser *p) {
    expect(p, TK_LBRACE, "'{'");
    StmtList list = {0};
    while (!check(p, TK_RBRACE) && !at_end(p))
        stmt_list_push(&list, parse_stmt(p));
    expect(p, TK_RBRACE, "'}'");
    return list;
}

/* ------------------------------------------------------------------ */
/* Top level declarations                                             */
/* ------------------------------------------------------------------ */

static Decl parse_function(Parser *p) {
    advance(p); /* fn */
    Token *name = expect(p, TK_IDENT, "function name");
    Decl d = {0};
    d.kind = DECL_FUNC;
    d.as.func.name = rl_strdup(name->lexeme);

    expect(p, TK_LPAREN, "'('");
    size_t cap = 0;
    if (!check(p, TK_RPAREN)) {
        do {
            Type *ptype = parse_type(p);
            Token *pname = expect(p, TK_IDENT, "parameter name");
            if (d.as.func.param_count + 1 > cap) {
                cap = cap ? cap * 2 : 4;
                d.as.func.params =
                    rl_realloc(d.as.func.params, cap * sizeof(Param));
            }
            d.as.func.params[d.as.func.param_count].type = ptype;
            d.as.func.params[d.as.func.param_count].name =
                rl_strdup(pname->lexeme);
            d.as.func.param_count++;
        } while (match(p, TK_COMMA));
    }
    expect(p, TK_RPAREN, "')'");
    expect(p, TK_KW_GIVES, "'gives'");
    d.as.func.return_type = parse_type(p);

    if (match(p, TK_KW_NATIVE)) {
        d.as.func.is_native = true;
        Token *sym = expect(p, TK_STRING_LIT, "native symbol name");
        d.as.func.native_name = rl_strdup(sym->lexeme);
        /* Native declarations may carry an empty body. */
        if (check(p, TK_LBRACE)) {
            parse_block(p);
        }
        d.as.func.has_body = false;
    } else {
        d.as.func.body = parse_block(p);
        d.as.func.has_body = true;
    }
    return d;
}

static Decl parse_blueprint(Parser *p) {
    advance(p); /* blueprint */
    Token *name = expect(p, TK_IDENT, "blueprint name");
    Decl d = {0};
    d.kind = DECL_BLUEPRINT;
    d.as.blueprint.name = rl_strdup(name->lexeme);
    expect(p, TK_ASSIGN, "'='");
    expect(p, TK_LBRACE, "'{'");
    size_t cap = 0;
    while (!check(p, TK_RBRACE) && !at_end(p)) {
        Type *ftype = parse_type(p);
        Token *fname = expect(p, TK_IDENT, "field name");
        expect(p, TK_SEMI, "';'");
        if (d.as.blueprint.field_count + 1 > cap) {
            cap = cap ? cap * 2 : 4;
            d.as.blueprint.fields =
                rl_realloc(d.as.blueprint.fields, cap * sizeof(Field));
        }
        d.as.blueprint.fields[d.as.blueprint.field_count].type = ftype;
        d.as.blueprint.fields[d.as.blueprint.field_count].name =
            rl_strdup(fname->lexeme);
        d.as.blueprint.field_count++;
    }
    expect(p, TK_RBRACE, "'}'");
    return d;
}

static Decl parse_choices(Parser *p) {
    advance(p); /* choices */
    Token *name = expect(p, TK_IDENT, "choices name");
    Decl d = {0};
    d.kind = DECL_CHOICES;
    d.as.choices.name = rl_strdup(name->lexeme);
    expect(p, TK_ASSIGN, "'='");
    expect(p, TK_LBRACE, "'{'");
    size_t cap = 0;
    long long next = 0;
    if (!check(p, TK_RBRACE)) {
        do {
            if (check(p, TK_RBRACE))
                break;
            Token *ename = expect(p, TK_IDENT, "enumerator name");
            long long value = next;
            bool explicit_value = false;
            if (match(p, TK_ASSIGN)) {
                Token *v = expect(p, TK_INT_LIT, "integer value");
                value = strtoll(v->lexeme, NULL, 10);
                explicit_value = true;
            }
            next = value + 1;
            if (d.as.choices.entry_count + 1 > cap) {
                cap = cap ? cap * 2 : 8;
                d.as.choices.entries =
                    rl_realloc(d.as.choices.entries, cap * sizeof(EnumEntry));
            }
            EnumEntry *ent = &d.as.choices.entries[d.as.choices.entry_count++];
            ent->name = rl_strdup(ename->lexeme);
            ent->value = value;
            ent->explicit_value = explicit_value;
        } while (match(p, TK_COMMA));
    }
    expect(p, TK_RBRACE, "'}'");
    return d;
}

static Decl parse_import(Parser *p) {
    advance(p); /* use */
    expect(p, TK_LPAREN, "'('");
    Token *path = expect(p, TK_STRING_LIT, "import path");
    expect(p, TK_RPAREN, "')'");
    expect(p, TK_KW_AKA, "'aka'");
    Token *alias = expect(p, TK_IDENT, "module alias");
    expect(p, TK_SEMI, "';'");
    Decl d = {0};
    d.kind = DECL_IMPORT;
    d.as.import.path = rl_strdup(path->lexeme);
    d.as.import.alias = rl_strdup(alias->lexeme);
    return d;
}

Program rl_parse(TokenList *tokens, const char *filename) {
    Parser p = {0};
    p.toks = tokens->items;
    p.count = tokens->count;
    p.pos = 0;
    p.file = filename;

    Program prog = {0};
    while (!at_end(&p)) {
        if (check(&p, TK_KW_USE)) {
            program_push(&prog, parse_import(&p));
        } else if (check(&p, TK_KW_FN)) {
            program_push(&prog, parse_function(&p));
        } else if (check(&p, TK_KW_BLUEPRINT)) {
            program_push(&prog, parse_blueprint(&p));
        } else if (check(&p, TK_KW_CHOICES)) {
            program_push(&prog, parse_choices(&p));
        } else {
            Token *t = peek(&p);
            rl_fatal(p.file, t->line, t->col,
                     "expected a top-level declaration but found '%s'",
                     t->lexeme[0] ? t->lexeme : rl_token_name(t->kind));
        }
    }
    return prog;
}
