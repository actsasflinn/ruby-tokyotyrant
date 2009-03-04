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


static VALUE mTokyoTyrant;
static VALUE cTable;

static VALUE eTokyoTyrantError;

static void cTable_free(TCRDB *db) {
  /* delete the object */
  tcrdbdel(db);
}

static VALUE cTable_close(VALUE self) {
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

static VALUE cTable_initialize(VALUE self, VALUE host, VALUE port) {
  int ecode;
  TCRDB *db;

  db = tcrdbnew();

  if(!tcrdbopen(db, StringValuePtr(host), FIX2INT(port))){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "open error: %s\n", tcrdberrmsg(ecode));
  }

  rb_iv_set(self, "@host", host);
  rb_iv_set(self, "@port", port);
  rb_iv_set(self, "@connection", Data_Wrap_Struct(rb_cObject, 0, cTable_free, db));

  return Qtrue;
}

static VALUE cTable_put(VALUE self, VALUE vpkey, VALUE vcols) {
  int ecode;
  TCMAP *cols;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(self, "@connection"), TCRDB, db);

  vpkey = StringValueEx(vpkey);
  Check_Type(vcols, T_HASH);
  cols = vhashtomap(vcols);

  if (!tcrdbtblput(db, RSTRING_PTR(vpkey), RSTRING_LEN(vpkey), cols)) {
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "put error: %s\n", tcrdberrmsg(ecode));
  }
  tcmapdel(cols);

  return Qtrue;
}

static VALUE cTable_get(VALUE self, VALUE vpkey){
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

static VALUE cTable_putkeep

void Init_tokyo_tyrant() {
  mTokyoTyrant = rb_define_module("TokyoTyrant");

  eTokyoTyrantError = rb_define_class("TokyoTyrantError", rb_eStandardError);

  cTable = rb_define_class_under(mTokyoTyrant, "Table", rb_cObject);

  rb_define_method(cTable, "initialize", cTable_initialize, 2);
  rb_define_method(cTable, "close", cTable_close, 0);
  rb_define_method(cTable, "put", cTable_put, 2);
  rb_define_alias(cTable, "[]=", "put");
  rb_define_method(cTable, "get", cTable_get, 1);
  rb_define_alias(cTable, "[]", "get");
}
