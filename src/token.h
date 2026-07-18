#ifndef ROOTLANG_TOKEN_H
#define ROOTLANG_TOKEN_H

#include "common.h"

/*
 * Lexical token categories for root_lang.
 *
 * root_lang keyword vocabulary:
 *   fn / returns / return    - function, return type, return
 *   let / const              - bindings
 *   if / else if / else      - conditionals
 *   while / for              - loops
 *   struct / enum            - aggregate and enumeration types
 *   void                     - absence of a value
 *   use ... aka ...          - module import
 *   native                   - foreign (C) linkage
 *   to                       - cast operator
 */
typedef enum {
    /* literals */
    TK_INT_LIT,
    TK_FLOAT_LIT,
    TK_DOUBLE_LIT,
    TK_STRING_LIT,
    TK_CHAR_LIT,
    TK_BOOL_LIT,

    /* type keywords */
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
    TK_KW_TEXT,     /* string type */
    TK_KW_BOOL,
    TK_KW_VOID,     /* none */
    TK_KW_ANY,

    /* declaration keywords */
    TK_KW_BLUEPRINT, /* struct */
    TK_KW_CHOICES,   /* enum */
    TK_KW_FN,        /* function */
    TK_KW_GIVES,     /* returns */
    TK_KW_GIVE,      /* return */
    TK_KW_LET,
    TK_KW_CONST,
    TK_KW_NATIVE,    /* extern */

    /* control flow keywords */
    TK_KW_WHEN,      /* if */
    TK_KW_ELSEWHEN,  /* else if */
    TK_KW_ORELSE,    /* else */
    TK_KW_LOOP,      /* while */
    TK_KW_WALK,      /* for */
    TK_KW_STOP,      /* break */
    TK_KW_SKIP,      /* continue */

    /* misc keywords */
    TK_KW_USE,       /* import */
    TK_KW_AKA,       /* as (alias) */
    TK_KW_TO,        /* cast operator */
    TK_KW_NIL,       /* null */
    TK_KW_SPANOF,    /* ArrayOf */
    TK_KW_REFOF,     /* PointerOf */
    TK_KW_SIZEOF,

    /* identifiers */
    TK_IDENT,

    /* operators */
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

    /* punctuation */
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
    char   *lexeme; /* owned copy of the raw text */
    int     line;
    int     col;
} Token;

typedef struct {
    Token  *items;
    size_t  count;
    size_t  cap;
} TokenList;

/* Look up a keyword by spelling; returns TK_IDENT when not a keyword. */
TokKind rl_keyword_lookup(const char *word);

const char *rl_token_name(TokKind kind);

#endif /* ROOTLANG_TOKEN_H */
