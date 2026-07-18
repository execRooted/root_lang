#ifndef ROOTLANG_CODEGEN_H
#define ROOTLANG_CODEGEN_H

#include "ast.h"

/*
 * Lower a fully resolved Program into portable C99 source text.
 *
 * The generated translation unit includes the root_lang runtime header and
 * relies on the runtime library for I/O, text handling and memory helpers. The
 * caller owns the returned buffer and must free it.
 */
char *rl_generate_c(Program *program);

#endif /* ROOTLANG_CODEGEN_H */
