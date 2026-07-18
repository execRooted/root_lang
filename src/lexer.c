#include "lexer.h"
#include <ctype.h>

/* Keyword table mapping spellings to token kinds. */
typedef struct {
    const char *word;
    TokKind     kind;
} KeywordEntry;

static const KeywordEntry KEYWORDS[] = {
    {"int",       TK_KW_INT},
    {"int8",      TK_KW_INT8},
    {"int16",     TK_KW_INT16},
    {"int64",     TK_KW_INT64},
    {"uint",      TK_KW_UINT},
    {"uint8",     TK_KW_UINT8},
    {"uint16",    TK_KW_UINT16},
    {"uint64",    TK_KW_UINT64},
    {"float",     TK_KW_FLOAT},
    {"double",    TK_KW_DOUBLE},
    {"char",      TK_KW_CHAR},
    {"text",      TK_KW_TEXT},
    {"bool",      TK_KW_BOOL},
    {"void",      TK_KW_VOID},
    {"any",       TK_KW_ANY},
    {"struct",    TK_KW_BLUEPRINT},
    {"enum",      TK_KW_CHOICES},
    {"fn",        TK_KW_FN},
    {"returns",   TK_KW_GIVES},
    {"return",    TK_KW_GIVE},
    {"let",       TK_KW_LET},
    {"const",     TK_KW_CONST},
    {"native",    TK_KW_NATIVE},
    {"if",        TK_KW_WHEN},
    {"else",      TK_KW_ORELSE},
    {"while",     TK_KW_LOOP},
    {"for",       TK_KW_WALK},
    {"stop",      TK_KW_STOP},
    {"skip",      TK_KW_SKIP},
    {"use",       TK_KW_USE},
    {"aka",       TK_KW_AKA},
    {"to",        TK_KW_TO},
    {"nil",       TK_KW_NIL},
    {"SpanOf",    TK_KW_SPANOF},
    {"RefOf",     TK_KW_REFOF},
    {"sizeof",    TK_KW_SIZEOF},
    {"yes",       TK_BOOL_LIT},
    {"no",        TK_BOOL_LIT},
    {NULL, TK_IDENT},
};

TokKind rl_keyword_lookup(const char *word) {
    for (const KeywordEntry *k = KEYWORDS; k->word; k++) {
        if (strcmp(k->word, word) == 0)
            return k->kind;
    }
    return TK_IDENT;
}

const char *rl_token_name(TokKind kind) {
    switch (kind) {
    case TK_INT_LIT:    return "integer";
    case TK_FLOAT_LIT:  return "float";
    case TK_DOUBLE_LIT: return "double";
    case TK_STRING_LIT: return "text literal";
    case TK_CHAR_LIT:   return "char literal";
    case TK_BOOL_LIT:   return "bool";
    case TK_IDENT:      return "identifier";
    case TK_SEMI:       return "';'";
    case TK_COMMA:      return "','";
    case TK_DOT:        return "'.'";
    case TK_LPAREN:     return "'('";
    case TK_RPAREN:     return "')'";
    case TK_LBRACE:     return "'{'";
    case TK_RBRACE:     return "'}'";
    case TK_LBRACKET:   return "'['";
    case TK_RBRACKET:   return "']'";
    case TK_EOF:        return "end of file";
    default:            return "token";
    }
}

typedef struct {
    const char *src;
    const char *file;
    size_t      pos;
    int         line;
    int         col;
    TokenList   out;
} Lexer;

static void emit(Lexer *lx, TokKind kind, const char *start, size_t len,
                 int line, int col) {
    if (lx->out.count + 1 > lx->out.cap) {
        lx->out.cap = lx->out.cap ? lx->out.cap * 2 : 64;
        lx->out.items = rl_realloc(lx->out.items, lx->out.cap * sizeof(Token));
    }
    Token *t = &lx->out.items[lx->out.count++];
    t->kind = kind;
    t->lexeme = rl_strndup(start, len);
    t->line = line;
    t->col = col;
}

static char peek(Lexer *lx) { return lx->src[lx->pos]; }
static char peek2(Lexer *lx) {
    return lx->src[lx->pos] ? lx->src[lx->pos + 1] : '\0';
}

static char advance(Lexer *lx) {
    char c = lx->src[lx->pos++];
    if (c == '\n') {
        lx->line++;
        lx->col = 1;
    } else {
        lx->col++;
    }
    return c;
}

static bool ident_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static bool ident_part(char c) { return isalnum((unsigned char)c) || c == '_'; }

static void lex_number(Lexer *lx) {
    int line = lx->line, col = lx->col;
    const char *start = &lx->src[lx->pos];
    bool is_float = false;
    bool is_double = false;

    while (isdigit((unsigned char)peek(lx)))
        advance(lx);

    if (peek(lx) == '.' && isdigit((unsigned char)peek2(lx))) {
        is_float = true;
        advance(lx); /* consume '.' */
        while (isdigit((unsigned char)peek(lx)))
            advance(lx);
    }

    /* Suffix 'd' forces double, 'f' forces float. */
    if (peek(lx) == 'd') {
        is_double = true;
        is_float = false;
        advance(lx);
    } else if (peek(lx) == 'f') {
        is_float = true;
        advance(lx);
    }

    size_t len = (size_t)(&lx->src[lx->pos] - start);
    /* Trim a trailing type suffix from the stored lexeme. */
    size_t store = len;
    if (store > 0 && (start[store - 1] == 'd' || start[store - 1] == 'f'))
        store--;

    TokKind kind = TK_INT_LIT;
    if (is_double)
        kind = TK_DOUBLE_LIT;
    else if (is_float)
        kind = TK_FLOAT_LIT;
    emit(lx, kind, start, store, line, col);
}

static void lex_string(Lexer *lx) {
    int line = lx->line, col = lx->col;
    advance(lx); /* opening quote */
    Rope buf;
    rope_init(&buf);
    while (peek(lx) != '"' && peek(lx) != '\0') {
        char c = advance(lx);
        if (c == '\\') {
            char e = advance(lx);
            switch (e) {
            case 'n': rope_pushc(&buf, '\n'); break;
            case 't': rope_pushc(&buf, '\t'); break;
            case 'r': rope_pushc(&buf, '\r'); break;
            case '0': rope_pushc(&buf, '\0'); break;
            case '\\': rope_pushc(&buf, '\\'); break;
            case '"': rope_pushc(&buf, '"'); break;
            case '\'': rope_pushc(&buf, '\''); break;
            default: rope_pushc(&buf, e); break;
            }
        } else {
            rope_pushc(&buf, c);
        }
    }
    if (peek(lx) != '"')
        rl_fatal(lx->file, line, col, "unterminated text literal");
    advance(lx); /* closing quote */
    emit(lx, TK_STRING_LIT, buf.data, buf.len, line, col);
    rope_free(&buf);
}

static void lex_char(Lexer *lx) {
    int line = lx->line, col = lx->col;
    advance(lx); /* opening quote */
    char value;
    char c = advance(lx);
    if (c == '\\') {
        char e = advance(lx);
        switch (e) {
        case 'n': value = '\n'; break;
        case 't': value = '\t'; break;
        case 'r': value = '\r'; break;
        case '0': value = '\0'; break;
        case '\\': value = '\\'; break;
        case '\'': value = '\''; break;
        case '"': value = '"'; break;
        default: value = e; break;
        }
    } else {
        value = c;
    }
    if (peek(lx) != '\'')
        rl_fatal(lx->file, line, col, "unterminated char literal");
    advance(lx); /* closing quote */
    emit(lx, TK_CHAR_LIT, &value, 1, line, col);
}

static void lex_ident(Lexer *lx) {
    int line = lx->line, col = lx->col;
    const char *start = &lx->src[lx->pos];
    while (ident_part(peek(lx)))
        advance(lx);
    size_t len = (size_t)(&lx->src[lx->pos] - start);
    char *word = rl_strndup(start, len);
    TokKind kind = rl_keyword_lookup(word);
    emit(lx, kind, start, len, line, col);
    free(word);
}

TokenList rl_lex(const char *source, const char *filename) {
    Lexer lx = {0};
    lx.src = source;
    lx.file = filename;
    lx.pos = 0;
    lx.line = 1;
    lx.col = 1;

    while (peek(&lx) != '\0') {
        char c = peek(&lx);

        /* whitespace */
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(&lx);
            continue;
        }

        /* '#' line comments */
        if (c == '#') {
            while (peek(&lx) != '\n' && peek(&lx) != '\0')
                advance(&lx);
            continue;
        }

        int line = lx.line, col = lx.col;

        if (isdigit((unsigned char)c)) {
            lex_number(&lx);
            continue;
        }
        if (c == '"') {
            lex_string(&lx);
            continue;
        }
        if (c == '\'') {
            lex_char(&lx);
            continue;
        }
        if (ident_start(c)) {
            lex_ident(&lx);
            continue;
        }

        advance(&lx); /* consume the first char of the operator */
        switch (c) {
        case '+':
            if (peek(&lx) == '=') { advance(&lx); emit(&lx, TK_PLUS_EQ, "+=", 2, line, col); }
            else emit(&lx, TK_PLUS, "+", 1, line, col);
            break;
        case '-':
            if (peek(&lx) == '=') { advance(&lx); emit(&lx, TK_MINUS_EQ, "-=", 2, line, col); }
            else emit(&lx, TK_MINUS, "-", 1, line, col);
            break;
        case '*':
            if (peek(&lx) == '=') { advance(&lx); emit(&lx, TK_STAR_EQ, "*=", 2, line, col); }
            else emit(&lx, TK_STAR, "*", 1, line, col);
            break;
        case '/':
            if (peek(&lx) == '=') { advance(&lx); emit(&lx, TK_SLASH_EQ, "/=", 2, line, col); }
            else emit(&lx, TK_SLASH, "/", 1, line, col);
            break;
        case '%': emit(&lx, TK_PERCENT, "%", 1, line, col); break;
        case '=':
            if (peek(&lx) == '=') { advance(&lx); emit(&lx, TK_EQ, "==", 2, line, col); }
            else emit(&lx, TK_ASSIGN, "=", 1, line, col);
            break;
        case '!':
            if (peek(&lx) == '=') { advance(&lx); emit(&lx, TK_NEQ, "!=", 2, line, col); }
            else emit(&lx, TK_NOT, "!", 1, line, col);
            break;
        case '<':
            if (peek(&lx) == '=') { advance(&lx); emit(&lx, TK_LE, "<=", 2, line, col); }
            else emit(&lx, TK_LT, "<", 1, line, col);
            break;
        case '>':
            if (peek(&lx) == '=') { advance(&lx); emit(&lx, TK_GE, ">=", 2, line, col); }
            else emit(&lx, TK_GT, ">", 1, line, col);
            break;
        case '&':
            if (peek(&lx) == '&') { advance(&lx); emit(&lx, TK_AND, "&&", 2, line, col); }
            else emit(&lx, TK_AMP, "&", 1, line, col);
            break;
        case '|':
            if (peek(&lx) == '|') { advance(&lx); emit(&lx, TK_OR, "||", 2, line, col); }
            else rl_fatal(lx.file, line, col, "unexpected character '|'");
            break;
        case ';': emit(&lx, TK_SEMI, ";", 1, line, col); break;
        case ',': emit(&lx, TK_COMMA, ",", 1, line, col); break;
        case '.': emit(&lx, TK_DOT, ".", 1, line, col); break;
        case '(': emit(&lx, TK_LPAREN, "(", 1, line, col); break;
        case ')': emit(&lx, TK_RPAREN, ")", 1, line, col); break;
        case '{': emit(&lx, TK_LBRACE, "{", 1, line, col); break;
        case '}': emit(&lx, TK_RBRACE, "}", 1, line, col); break;
        case '[': emit(&lx, TK_LBRACKET, "[", 1, line, col); break;
        case ']': emit(&lx, TK_RBRACKET, "]", 1, line, col); break;
        default:
            rl_fatal(lx.file, line, col, "unexpected character '%c'", c);
        }
    }

    emit(&lx, TK_EOF, "", 0, lx.line, lx.col);
    return lx.out;
}

void rl_tokens_free(TokenList *list) {
    for (size_t i = 0; i < list->count; i++)
        free(list->items[i].lexeme);
    free(list->items);
    list->items = NULL;
    list->count = list->cap = 0;
}
