#ifndef ROOTLANG_TOKEN_H
#define ROOTLANG_TOKEN_H

#include "common.h"

typedef enum {

    TK_INT_LIT,
    TK_FLOAT_LIT,
    TK_DOUBLE_LIT,
    TK_STRING_LIT,
    TK_CHAR_LIT,
    TK_BOOL_LIT,

    TK_KW_INT,
    TK_KW_INT8,
    TK_KW_INT16,
    TK_KW_INT64,
    TK_KW_UINT,
    TK_KW_UINT8,
    TK_KW_UINT16,
    TK_KW_UINT64,
    TK_KW_FLOAT,
    TK_KW_DOUBLE,
    TK_KW_CHAR,
    TK_KW_TEXT,
    TK_KW_BOOL,
    TK_KW_VOID,
    TK_KW_ANY,

    TK_KW_BLUEPRINT,
    TK_KW_CHOICES,
    TK_KW_FN,
    TK_KW_GIVES,
    TK_KW_GIVE,
    TK_KW_LET,
    TK_KW_CONST,
    TK_KW_NATIVE,

    TK_KW_WHEN,
    TK_KW_ELSEWHEN,
    TK_KW_ORELSE,
    TK_KW_LOOP,
    TK_KW_WALK,
    TK_KW_STOP,
    TK_KW_SKIP,

    TK_KW_USE,
    TK_KW_AKA,
    TK_KW_TO,
    TK_KW_NIL,
    TK_KW_SPANOF,
    TK_KW_REFOF,
    TK_KW_SIZEOF,

    TK_IDENT,

    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_PERCENT,
    TK_PLUS_EQ,
    TK_MINUS_EQ,
    TK_STAR_EQ,
    TK_SLASH_EQ,
    TK_ASSIGN,
    TK_EQ,
    TK_NEQ,
    TK_LT,
    TK_GT,
    TK_LE,
    TK_GE,
    TK_AND,
    TK_OR,
    TK_NOT,
    TK_AMP,

    TK_SEMI,
    TK_COMMA,
    TK_DOT,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_LBRACKET,
    TK_RBRACKET,

    TK_EOF
} TokKind;

typedef struct {
    TokKind kind;
    char   *lexeme;
    int     line;
    int     col;
} Token;

typedef struct {
    Token  *items;
    size_t  count;
    size_t  cap;
} TokenList;

TokKind rl_keyword_lookup(const char *word);

const char *rl_token_name(TokKind kind);

#endif
