#ifndef ROOTLANG_PARSER_H
#define ROOTLANG_PARSER_H

#include "ast.h"
#include "token.h"

Program rl_parse(TokenList *tokens, const char *filename);

#endif
