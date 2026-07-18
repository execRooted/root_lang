#ifndef ROOTLANG_LEXER_H
#define ROOTLANG_LEXER_H

#include "token.h"

/*
 * Tokenize root_lang source text.
 *
 * The returned TokenList always ends with a TK_EOF token. The `filename`
 * argument is only used to produce readable diagnostics.
 */
TokenList rl_lex(const char *source, const char *filename);

void rl_tokens_free(TokenList *list);

#endif /* ROOTLANG_LEXER_H */
