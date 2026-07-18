#ifndef ROOTLANG_PARSER_H
#define ROOTLANG_PARSER_H

#include "ast.h"
#include "token.h"

/*
 * Parse a token stream into a Program AST for a single source file.
 * `filename` is used for diagnostics only.
 */
Program rl_parse(TokenList *tokens, const char *filename);

#endif /* ROOTLANG_PARSER_H */
