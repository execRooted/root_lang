#ifndef ROOTLANG_RESOLVER_H
#define ROOTLANG_RESOLVER_H

#include "ast.h"

/*
 * Load `main_path` and, recursively, every module it pulls in with `use`.
 *
 * The result is a single flattened Program. Functions defined in an imported
 * module are tagged with their `module_alias` so later stages can mangle their
 * names uniquely (e.g. alias `std`, function `print` -> `std__print`). Blueprint
 * and choices declarations are shared globally.
 */
Program rl_resolve_program(const char *main_path);

#endif /* ROOTLANG_RESOLVER_H */
