#ifndef RUBY_TOKYOTYRANT_MODULE
#define RUBY_TOKYOTYRANT_MODULE

#include <tokyo_tyrant.h>

extern TCRDB *mTokyoTyrant_getdb(VALUE vself);
extern void mTokyoTyrant_exception(VALUE vself);
void init_mod();

#endif
