#ifndef RUBY_TOKYOTYRANT
#define RUBY_TOKYOTYRANT

#include <ruby.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <tcrdb.h>
#include <tokyo_tyrant_module.h>
#include <tokyo_tyrant_db.h>
#include <tokyo_tyrant_table.h>
#include <tokyo_tyrant_query.h>

#define RDBVNDATA "@rdb"
#define RDBQRYVNDATA "@rdbquery"
#define NUMBUFSIZ 32
#define TTPUT      0
#define TTPUTKEEP  1
#define TTPUTCAT   2
#define TTPUTNR    3

#if !defined(RSTRING_PTR)
#define RSTRING_PTR(TC_s) (RSTRING(TC_s)->ptr)
#endif
#if !defined(RSTRING_LEN)
#define RSTRING_LEN(TC_s) (RSTRING(TC_s)->len)
#endif
#if !defined(RARRAY_LEN)
#define RARRAY_LEN(TC_a) (RARRAY(TC_a)->len)
#endif

extern VALUE mTokyoTyrant;

extern VALUE eTokyoTyrantError;
extern VALUE eTokyoTyrantErrorInvalid;
extern VALUE eTokyoTyrantErrorNoHost;
extern VALUE eTokyoTyrantErrorRefused;
extern VALUE eTokyoTyrantErrorSend;
extern VALUE eTokyoTyrantErrorReceive;
extern VALUE eTokyoTyrantErrorKeep;
extern VALUE eTokyoTyrantErrorNoRecord;
extern VALUE eTokyoTyrantErrorMisc;

extern VALUE cDB;
extern VALUE cTable;
extern VALUE cQuery;

extern VALUE StringRaw(const char *buf, int bsiz);
extern VALUE StringValueEx(VALUE vobj);
extern TCLIST *varytolist(VALUE vary);
extern VALUE listtovary(TCLIST *list);
extern TCMAP *vhashtomap(VALUE vhash);
extern VALUE maptovhash(TCMAP *map);
extern TCMAP *varytomap(VALUE vhash);
extern TCLIST *vhashtolist(VALUE vhash);
extern TCLIST *vhashtoputlist(VALUE vhash);
#endif
