#include <tokyo_tyrant_bdb.h>

static VALUE cBDB_putlist(VALUE vself, VALUE vhash){
  VALUE vary;
  TCLIST *list, *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);
  Check_Type(vhash, T_HASH);

  list = vhashtoputlist(vhash);
  if ((result = tcrdbmisc(db, "putlist", 0, list)) != NULL){
    vary = listtovary(result);
    tclistdel(result);
  } else {
    vary = rb_ary_new();
  }
  tclistdel(list);

  return vary;
}

static VALUE cBDB_getlist(int argc, VALUE *argv, VALUE vself){
  VALUE vkeys, vvalue, vhash;
  TCLIST *list, *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);
  rb_scan_args(argc, argv, "*", &vkeys);

  // I really hope there is a better way to do this
  if (RARRAY_LEN(vkeys) == 1) {
    vvalue = rb_ary_entry(vkeys, 0);
    switch (TYPE(vvalue)){
      case T_STRING:
      case T_FIXNUM:
        break;
      case T_ARRAY:
        vkeys = vvalue;
        break;
      case T_OBJECT:
        vkeys = rb_convert_type(vvalue, T_ARRAY, "Array", "to_a");
        break;
    }
  }
  Check_Type(vkeys, T_ARRAY);

  list = varytolist(vkeys);
  result = tcrdbmisc(db, "getlist", RDBMONOULOG, list);
  tclistdel(list);
  vhash = listtovhash(result);
  tclistdel(result);
  return vhash;
}

static VALUE cBDB_each(VALUE vself){
  VALUE vrv;
  const char *kbuf, *vbuf;
  int ksiz, vsiz;
  TCLIST *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);
  vrv = Qnil;

  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");

  tcrdbiterinit(db);
  tcrdbmisc(db, "iterinit", RDBMONOULOG, tclistnew());
  while((result = tcrdbmisc(db, "iternext", RDBMONOULOG, tclistnew())) != NULL){
    if (tclistnum(result) == 2) {
      kbuf = tclistval(result, 0, &ksiz);
      vbuf = tclistval(result, 1, &vsiz);
      vrv = rb_yield_values(2, rb_str_new(kbuf, ksiz), rb_str_new(vbuf, vsiz));
    }
    tclistdel(result);
  }
  return vrv;
}

static VALUE cBDB_values(VALUE vself){
  VALUE vary;
  const char *vbuf;
  int vsiz;
  TCLIST *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vary = rb_ary_new();
  tcrdbiterinit(db);
  tcrdbmisc(db, "iterinit", RDBMONOULOG, tclistnew());
  while((result = tcrdbmisc(db, "iternext", RDBMONOULOG, tclistnew())) != NULL){
    if (tclistnum(result) == 2){
      vbuf = tclistval(result, 1, &vsiz);
      vary = rb_ary_push(vary, rb_str_new(vbuf, vsiz));
    }
    tclistdel(result);
  }

  return vary;
}

void init_bdb(){
  rb_define_method(cBDB, "putlist", cBDB_putlist, 1);
  rb_define_alias(cBDB, "mput", "putlist");
  rb_define_method(cBDB, "getlist", cBDB_getlist, -1);
  rb_define_alias(cBDB, "mget", "putlist");
  rb_define_method(cBDB, "each", cBDB_each, 0);
  rb_define_method(cBDB, "values", cBDB_values, 0);
}
