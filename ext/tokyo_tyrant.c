#include <ruby.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <tcrdb.h>

#define NUMBUFSIZ      32

#if !defined(RSTRING_PTR)
#define RSTRING_PTR(TC_s) (RSTRING(TC_s)->ptr)
#endif
#if !defined(RSTRING_LEN)
#define RSTRING_LEN(TC_s) (RSTRING(TC_s)->len)
#endif
#if !defined(RARRAY_LEN)
#define RARRAY_LEN(TC_a) (RARRAY(TC_a)->len)
#endif

#define TPUT 0;
#define TPUTKEEP 1;
#define TPUTCAT 2;

static VALUE mTokyoTyrant;
static VALUE eTokyoTyrantError;

/* private function prototypes */
static VALUE StringValueEx(VALUE vobj);
static TCLIST *varytolist(VALUE vary);
static VALUE listtovary(TCLIST *list);
static TCMAP *vhashtomap(VALUE vhash);
static VALUE maptovhash(TCMAP *map);

static VALUE StringValueEx(VALUE vobj){
  char kbuf[NUMBUFSIZ];
  int ksiz;
  switch(TYPE(vobj)){
  case T_FIXNUM:
    ksiz = sprintf(kbuf, "%d", (int)FIX2INT(vobj));
    return rb_str_new(kbuf, ksiz);
  case T_BIGNUM:
    ksiz = sprintf(kbuf, "%lld", (long long)NUM2LL(vobj));
    return rb_str_new(kbuf, ksiz);
  case T_TRUE:
    ksiz = sprintf(kbuf, "true");
    return rb_str_new(kbuf, ksiz);
  case T_FALSE:
    ksiz = sprintf(kbuf, "false");
    return rb_str_new(kbuf, ksiz);
  case T_NIL:
    ksiz = sprintf(kbuf, "nil");
    return rb_str_new(kbuf, ksiz);
  }
  return StringValue(vobj);
}

static TCLIST *varytolist(VALUE vary){
  VALUE vval;
  TCLIST *list;
  int i, num;
  num = RARRAY_LEN(vary);
  list = tclistnew2(num);
  for(i = 0; i < num; i++){
    vval = rb_ary_entry(vary, i);
    vval = StringValueEx(vval);
    tclistpush(list, RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return list;
}

static VALUE listtovary(TCLIST *list){
  VALUE vary;
  const char *vbuf;
  int i, num, vsiz;
  num = tclistnum(list);
  vary = rb_ary_new2(num);
  for(i = 0; i < num; i++){
    vbuf = tclistval(list, i, &vsiz);
    rb_ary_push(vary, rb_str_new(vbuf, vsiz));
  }
  return vary;
}

static TCMAP *vhashtomap(VALUE vhash){
  VALUE vkeys, vkey, vval;
  TCMAP *map;
  int i, num;
  map = tcmapnew2(31);
  vkeys = rb_funcall(vhash, rb_intern("keys"), 0);
  num = RARRAY_LEN(vkeys);
  for(i = 0; i < num; i++){
    vkey = rb_ary_entry(vkeys, i);
    vval = rb_hash_aref(vhash, vkey);
    vkey = StringValueEx(vkey);
    vval = StringValueEx(vval);
    tcmapput(map, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return map;
}

static VALUE maptovhash(TCMAP *map){
  const char *kbuf, *vbuf;
  int ksiz, vsiz;
  VALUE vhash;
  vhash = rb_hash_new();
  tcmapiterinit(map);
  while((kbuf = tcmapiternext(map, &ksiz)) != NULL){
    vbuf = tcmapiterval(kbuf, &vsiz);
    rb_hash_aset(vhash, rb_str_new(kbuf, ksiz), rb_str_new(vbuf, vsiz));
  }
  return vhash;
}

static VALUE cDB;
static VALUE cDB_errmsg(VALUE ecode);
static VALUE cDB_initialize(VALUE self, VALUE host, VALUE port);
static void cDB_free(TCRDB *db);
static VALUE cDB_close(VALUE self);
static VALUE cDB_ecode(VALUE self);
static VALUE cDB_put(VALUE self, VALUE vpkey, VALUE vstr);
static VALUE cDB_putkeep(VALUE self, VALUE vpkey, VALUE vstr);
static VALUE cDB_putcat(VALUE self, VALUE vpkey, VALUE vstr);
static VALUE cDB_putshl(VALUE self, VALUE vpkey, VALUE vstr);
static VALUE cDB_putnr(VALUE self, VALUE vpkey, VALUE vstr);
static VALUE cDB_out(VALUE self, VALUE vpkey);
static VALUE cDB_get(VALUE self, VALUE keys);
static VALUE cDB_vsiz(VALUE self, VALUE vpkey);
static VALUE cDB_iterinit(VALUE self);
static VALUE cDB_iternext(VALUE self);
TCLIST cDB_fwmkeys(VALUE self, VALUE prefix, VALUE max);
int cDB_addint(VALUE self, const void *kbuf, int ksiz, int num);
double cDB_adddouble(VALUE self, const void *kbuf, int ksiz, double num);
void cDB_ext(VALUE self, VALUE name, int opts, const char *kstr, const char *vstr);
static VALUE cDB_sync(VALUE self);
static VALUE cDB_vanish(VALUE self);
static VALUE cDB_copy(VALUE self, VALUE path);
static VALUE cDB_restore(VALUE self, VALUE path, uint64_t ts);
static VALUE cDB_setmst(VALUE self, VALUE host, VALUE port);
uint64_t cDB_rnum(VALUE self);
uint64_t cDB_size(VALUE self);
char cDB_stat(VALUE self);
TCLIST cDB_misc(VALUE self, VALUE name, int opts, const TCLIST *args);

static void cDB_free(TCRDB *db) {
  /* delete the object */
  tcrdbdel(db);
}

static VALUE cDB_close(VALUE self) {
  int ecode;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(self, "@connection"), TCRDB, db);

  /* close the connection */
  if(!tcrdbclose(db)) {
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "close error: %s\n", tcrdberrmsg(ecode));
  }

  return Qtrue;
}

static VALUE cDB_initialize(VALUE self, VALUE host, VALUE port) {
  int ecode;
  TCRDB *db;

  db = tcrdbnew();

  if(!tcrdbopen(db, StringValuePtr(host), FIX2INT(port))){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "open error: %s\n", tcrdberrmsg(ecode));
  }

  rb_iv_set(self, "@host", host);
  rb_iv_set(self, "@port", port);
  rb_iv_set(self, "@connection", Data_Wrap_Struct(rb_cObject, 0, cDB_free, db));

  return Qtrue;
}

static VALUE cDB_vanish(VALUE self){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(self, "@connection"), TCRDB, db);
  return tcrdbvanish(db) ? Qtrue : Qfalse;
}


static VALUE cTable;

/* public function prototypes */
static VALUE cTable_put_method(VALUE self, VALUE vpkey, VALUE vcols, int method);
static VALUE cTable_put(VALUE self, VALUE vpkey, VALUE vcols);
static VALUE cTable_putkeep(VALUE self, VALUE vpkey, VALUE vcols);
static VALUE cTable_putcat(VALUE self, VALUE vpkey, VALUE vcols);
static VALUE cTable_out(VALUE self, VALUE vpkey);
static VALUE cTable_get(VALUE self, VALUE vpkey);
static VALUE cTable_setindex(VALUE self, VALUE vname, VALUE vtype);
static VALUE cTable_genuid(VALUE self);
void Init_tokyo_tyrant();

static VALUE cTable_put_method(VALUE self, VALUE vpkey, VALUE vcols, int method) {
  int ecode;
  bool res;
  TCMAP *cols;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(self, "@connection"), TCRDB, db);

  vpkey = StringValueEx(vpkey);
  Check_Type(vcols, T_HASH);
  cols = vhashtomap(vcols);

  // there's probably a more elegant way yo do this
  switch(method) {
    case 0 :
      res = tcrdbtblput(db, RSTRING_PTR(vpkey), RSTRING_LEN(vpkey), cols);
      break;
    case 1 :
      res = tcrdbtblputkeep(db, RSTRING_PTR(vpkey), RSTRING_LEN(vpkey), cols);
      break;
    case 2 :
      res = tcrdbtblputcat(db, RSTRING_PTR(vpkey), RSTRING_LEN(vpkey), cols);
      break;
    default :
      res = false;
      break;
  }

  if (!res) {
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "put error: %s\n", tcrdberrmsg(ecode));
  }
  tcmapdel(cols);

  return Qtrue;
}

static VALUE cTable_put(VALUE self, VALUE vpkey, VALUE vcols) {
  return cTable_put_method(self, vpkey, vcols, 0);
}

static VALUE cTable_putkeep(VALUE self, VALUE vpkey, VALUE vcols) {
  return cTable_put_method(self, vpkey, vcols, 1);
}

static VALUE cTable_putcat(VALUE self, VALUE vpkey, VALUE vcols) {
  return cTable_put_method(self, vpkey, vcols, 2);
}

static VALUE cTable_out(VALUE self, VALUE vpkey) {
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(self, "@connection"), TCRDB, db);
  vpkey = StringValueEx(vpkey);

  return tcrdbtblout(db, RSTRING_PTR(vpkey), RSTRING_LEN(vpkey));
}

static VALUE cTable_get(VALUE self, VALUE vpkey) {
  VALUE vcols;
  TCRDB *db;
  TCMAP *cols;
  Data_Get_Struct(rb_iv_get(self, "@connection"), TCRDB, db);
  vpkey = StringValueEx(vpkey);

  if(!(cols = tcrdbtblget(db, RSTRING_PTR(vpkey), RSTRING_LEN(vpkey)))) return Qnil;
  vcols = maptovhash(cols);
  tcmapdel(cols);
  return vcols;
}

static VALUE cTable_setindex(VALUE self, VALUE vname, VALUE vtype) {
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(self, "@connection"), TCRDB, db);
  Check_Type(vname, T_STRING);
  return tcrdbtblsetindex(db, RSTRING_PTR(vname), NUM2INT(vtype)) ? Qtrue : Qfalse;
}

static VALUE cTable_genuid(VALUE self) {
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(self, "@connection"), TCRDB, db);
  return LL2NUM(tcrdbtblgenuid(db));
}

void Init_tokyo_tyrant() {
  mTokyoTyrant = rb_define_module("TokyoTyrant");

  eTokyoTyrantError = rb_define_class("TokyoTyrantError", rb_eStandardError);
  cDB = rb_define_class_under(mTokyoTyrant, "DB", rb_cObject);
  cTable = rb_define_class_under(mTokyoTyrant, "Table", cDB);

  rb_define_const(cDB, "ESUCCESS", INT2NUM(TTESUCCESS));
  rb_define_const(cDB, "EINVALID", INT2NUM(TTEINVALID));
  rb_define_const(cDB, "ENOHOST", INT2NUM(TTENOHOST));
  rb_define_const(cDB, "EREFUSED", INT2NUM(TTEREFUSED));
  rb_define_const(cDB, "ESEND", INT2NUM(TTESEND));
  rb_define_const(cDB, "ERECV", INT2NUM(TTERECV));
  rb_define_const(cDB, "EKEEP", INT2NUM(TTEKEEP));
  rb_define_const(cDB, "ENOREC", INT2NUM(TTENOREC));
  rb_define_const(cDB, "EMISC", INT2NUM(TTEMISC));

  rb_define_const(cDB, "ITLEXICAL", INT2NUM(RDBITLEXICAL));
  rb_define_const(cDB, "ITDECIMAL", INT2NUM(RDBITDECIMAL));
  rb_define_const(cDB, "ITVOID", INT2NUM(RDBITVOID));
  rb_define_const(cDB, "ITKEEP", INT2NUM(RDBITKEEP));

  rb_define_method(cDB, "initialize", cDB_initialize, 2);
  rb_define_method(cDB, "close", cDB_close, 0);
/*
  rb_define_method(cDB, "errmsg", cDB_errmsg, 1);
  rb_define_method(cDB, "ecode", cDB_ecode, 0);
  rb_define_method(cDB, "put", cDB_put, 2);
  rb_define_alias(cDB, "[]=", "put");
  rb_define_method(cDB, "putkeep", cDB_putkeep, 2);
  rb_define_method(cDB, "putcat", cDB_putcat, 2);
  rb_define_method(cDB, "putshl", cDB_putshl, 2);
  rb_define_method(cDB, "putnr", cDB_putnr, 2);
  rb_define_method(cDB, "out", cDB_out, 1);
  rb_define_method(cDB, "get", cDB_get, 1);
  rb_define_alias(cDB, "[]", "get");
  rb_define_method(cDB, "vsiz", cDB_vsiz, 2);
  rb_define_method(cDB, "iterinit", cDB_iterinit, 0);
  rb_define_method(cDB, "iternext", cDB_iternext, 0);
  rb_define_method(cDB, "sync", cDB_sync, 0);
*/
  rb_define_method(cDB, "vanish", cDB_vanish, 0);
  rb_define_alias(cDB, "clear", "vanish");
/*
  rb_define_method(cDB, "copy", cDB_copy, 1);
  rb_define_method(cDB, "restore", cDB_restore, 2);
  rb_define_method(cDB, "setmst", cDB_setmst, 2);
*/

  rb_define_method(cTable, "put", cTable_put, 2);
  rb_define_alias(cTable, "[]=", "put");
  rb_define_method(cTable, "putkeep", cTable_putkeep, 2);
  rb_define_method(cTable, "putcat", cTable_putcat, 2);
  rb_define_method(cTable, "out", cTable_out, 1);
  rb_define_method(cTable, "get", cTable_get, 1);
  rb_define_alias(cTable, "[]", "get");
  rb_define_method(cTable, "setindex", cTable_setindex, 2);
  rb_define_method(cTable, "genuid", cTable_genuid, 0);
}
