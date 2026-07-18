#ifndef ROOTLANG_AST_H
#define ROOTLANG_AST_H

#include "common.h"

/* ------------------------------------------------------------------ */
/* Types                                                              */
/* ------------------------------------------------------------------ */

typedef enum {
    TY_VOID,
    TY_INT,
    TY_INT8,
    TY_INT16,
    TY_INT64,
    TY_UINT,
    TY_UINT8,
    TY_UINT16,
    TY_UINT64,
    TY_FLOAT,
    TY_DOUBLE,
    TY_CHAR,
    TY_TEXT,    /* string */
    TY_BOOL,
    TY_ANY,     /* opaque pointer, castable */
    TY_SPAN,    /* array: element type in `inner` */
    TY_REF,     /* pointer: pointee type in `inner` */
    TY_NAMED    /* blueprint (struct) referenced by name */
} TypeKind;

typedef struct Type {
    TypeKind      kind;
    struct Type  *inner; /* element/pointee type for SPAN/REF */
    char         *name;  /* for TY_NAMED */
} Type;

Type *type_new(TypeKind kind);
Type *type_span(Type *inner);
Type *type_ref(Type *inner);
Type *type_named(const char *name);
Type *type_clone(const Type *t);
bool  type_equal(const Type *a, const Type *b);

/* ------------------------------------------------------------------ */
/* Expressions                                                        */
/* ------------------------------------------------------------------ */

typedef enum {
    EX_INT,
    EX_FLOAT,
    EX_DOUBLE,
    EX_STRING,
    EX_CHAR,
    EX_BOOL,
    EX_NIL,
    EX_IDENT,
    EX_BINARY,
    EX_UNARY,
    EX_ASSIGN,      /* target op= value / = value */
    EX_CALL,        /* callee(args...) */
    EX_MODULE_CALL, /* alias.func(args...) */
    EX_INDEX,       /* base[index] */
    EX_FIELD,       /* base.field  (also .address/.value/.length) */
    EX_ARRAY_LIT,   /* {a, b, c} */
    EX_CAST,        /* value to Type */
    EX_SIZEOF       /* sizeof(Type) */
} ExprKind;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LE, OP_GE,
    OP_AND, OP_OR,
    OP_NEG, OP_NOT,
    OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN, OP_MUL_ASSIGN, OP_DIV_ASSIGN
} OpKind;

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef struct {
    Expr **items;
    size_t count;
} ExprList;

struct Expr {
    ExprKind kind;
    int      line;
    int      col;
    Type    *inferred; /* filled during semantic analysis */

    /* literals */
    long long ival;
    double    fval;
    char     *sval;   /* string literal text / identifier name */
    char      cval;
    bool      bval;

    /* binary / unary / assign */
    OpKind op;
    Expr  *lhs;
    Expr  *rhs;

    /* call / module call */
    char     *callee;   /* function name */
    char     *module;   /* alias for module calls */
    ExprList  args;

    /* index / field */
    Expr *base;
    Expr *index;
    char *field;

    /* array literal */
    ExprList elems;

    /* cast / sizeof */
    Type *cast_type;
};

Expr *expr_new(ExprKind kind, int line, int col);

/* ------------------------------------------------------------------ */
/* Statements                                                         */
/* ------------------------------------------------------------------ */

typedef enum {
    ST_VAR_DECL,
    ST_EXPR,
    ST_GIVE,      /* return */
    ST_WHEN,      /* if / elsewhen / orelse */
    ST_LOOP,      /* while */
    ST_WALK,      /* for */
    ST_BLOCK,
    ST_STOP,      /* break */
    ST_SKIP       /* continue */
} StmtKind;

typedef struct {
    Stmt **items;
    size_t count;
    size_t cap;
} StmtList;

void stmt_list_push(StmtList *list, Stmt *s);

struct Stmt {
    StmtKind kind;
    int      line;
    int      col;

    /* var decl */
    Type *decl_type;
    char *decl_name;
    Expr *decl_init;
    bool  decl_const;

    /* expression statement / return value */
    Expr *expr;

    /* when: condition + then + else chain */
    Expr    *cond;
    StmtList then_body;
    StmtList else_body;   /* may be empty */
    bool     has_else;

    /* loop / walk */
    Stmt    *walk_init;   /* var decl */
    Expr    *walk_step;   /* assignment expr */
    StmtList body;
};

Stmt *stmt_new(StmtKind kind, int line, int col);

/* ------------------------------------------------------------------ */
/* Top level declarations                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    Type *type;
    char *name;
} Param;

typedef struct {
    char    *name;
    Type    *return_type;
    Param   *params;
    size_t   param_count;
    StmtList body;
    bool     is_native;    /* linked to a C symbol */
    char    *native_name;  /* the C symbol to call */
    bool     has_body;
} FuncDecl;

typedef struct {
    Type *type;
    char *name;
} Field;

typedef struct {
    char   *name;
    Field  *fields;
    size_t  field_count;
} BlueprintDecl;

typedef struct {
    char     *name;
    long long value;
    bool      explicit_value;
} EnumEntry;

typedef struct {
    char       *name;
    EnumEntry  *entries;
    size_t      entry_count;
} ChoicesDecl;

typedef struct {
    char *path;   /* file path as written */
    char *alias;  /* aka name */
} ImportDecl;

typedef enum {
    DECL_FUNC,
    DECL_BLUEPRINT,
    DECL_CHOICES,
    DECL_IMPORT
} DeclKind;

typedef struct {
    DeclKind kind;
    union {
        FuncDecl      func;
        BlueprintDecl blueprint;
        ChoicesDecl   choices;
        ImportDecl    import;
    } as;
    /* Module alias applied when this decl came from an imported file. */
    char *module_alias;
} Decl;

typedef struct {
    Decl  *items;
    size_t count;
    size_t cap;
} Program;

void program_push(Program *p, Decl decl);

#endif /* ROOTLANG_AST_H */
