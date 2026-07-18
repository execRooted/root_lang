#ifndef ROOTLANG_LEXER_H
#define ROOTLANG_LEXER_H

#include "token.h"

TokenList rl_lex(const char *source, const char *filename);

void rl_tokens_free(TokenList *list);

#endif
