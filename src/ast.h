#ifndef ROOTLANG_AST_H
#define ROOTLANG_AST_H

#include "common.h"

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
    TY_TEXT,
    TY_BOOL,
    TY_ANY,
    TY_SPAN,
    TY_REF,
    TY_NAMED
} TypeKind;

typedef struct Type {
    TypeKind      kind;
    struct Type  *inner;
    char         *name;
} Type;

Type *type_new(TypeKind kind);
Type *type_span(Type *inner);
Type *type_ref(Type *inner);
Type *type_named(const char *name);
Type *type_clone(const Type *t);
bool  type_equal(const Type *a, const Type *b);

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
    EX_ASSIGN,
    EX_CALL,
    EX_MODULE_CALL,
    EX_INDEX,
    EX_FIELD,
    EX_ARRAY_LIT,
    EX_CAST,
    EX_SIZEOF
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
    Type    *inferred;

    long long ival;
    double    fval;
    char     *sval;
    char      cval;
    bool      bval;

    OpKind op;
    Expr  *lhs;
    Expr  *rhs;

    char     *callee;
    char     *module;
    ExprList  args;

    Expr *base;
    Expr *index;
    char *field;

    ExprList elems;

    Type *cast_type;
};

Expr *expr_new(ExprKind kind, int line, int col);

typedef enum {
    ST_VAR_DECL,
    ST_EXPR,
    ST_GIVE,
    ST_WHEN,
    ST_LOOP,
    ST_WALK,
    ST_BLOCK,
    ST_STOP,
    ST_SKIP
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

    Type *decl_type;
    char *decl_name;
    Expr *decl_init;
    bool  decl_const;

    Expr *expr;

    Expr    *cond;
    StmtList then_body;
    StmtList else_body;
    bool     has_else;

    Stmt    *walk_init;
    Expr    *walk_step;
    StmtList body;
};

Stmt *stmt_new(StmtKind kind, int line, int col);

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
    bool     is_native;
    char    *native_name;
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
    char *path;
    char *alias;
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

    char *module_alias;
} Decl;

typedef struct {
    Decl  *items;
    size_t count;
    size_t cap;
} Program;

void program_push(Program *p, Decl decl);

#endif
