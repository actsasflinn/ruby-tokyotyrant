#include <tokyo_tyrant.h>

extern VALUE StringValueEx(VALUE vobj){
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

extern TCLIST *varytolist(VALUE vary){
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

extern VALUE listtovary(TCLIST *list){
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

extern TCMAP *vhashtomap(VALUE vhash){
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

extern VALUE maptovhash(TCMAP *map){
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

VALUE mTokyoTyrant;
VALUE eTokyoTyrantError;
VALUE cDB;
VALUE cTable;
VALUE cQuery;

void Init_tokyo_tyrant(){
  mTokyoTyrant = rb_define_module("TokyoTyrant");

  eTokyoTyrantError = rb_define_class("TokyoTyrantError", rb_eStandardError);
  cDB = rb_define_class_under(mTokyoTyrant, "DB", rb_cObject);
  cTable = rb_define_class_under(mTokyoTyrant, "Table", cDB);
  cQuery = rb_define_class_under(mTokyoTyrant, "Query", rb_cObject);

  init_db();
  init_table();
  init_query();
}
