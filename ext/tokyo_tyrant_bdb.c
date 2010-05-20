#include <tokyo_tyrant_bdb.h>

static VALUE cBDB_put_method(VALUE vself, VALUE vkey, VALUE vval, char *command, bool bang){
  VALUE vres = Qfalse;
  TCLIST *list, *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vkey = StringValueEx(vkey);
  vval = StringValueEx(vval);

  list = tclistnew2(2);
  tclistpush(list, RSTRING_PTR(vkey), RSTRING_LEN(vkey));
  tclistpush(list, RSTRING_PTR(vval), RSTRING_LEN(vval));
  if ((result = tcrdbmisc(db, command, 0, list)) != NULL){
    if (tclistnum(result) == 0){
      vres = Qtrue;
    } else {
      vres = listtovary(result);
    }
    tclistdel(result);
  } else {
    if (bang) mTokyoTyrant_exception(vself, NULL);
  }
  tclistdel(list);
  return vres;
}

static VALUE cBDB_putlist(VALUE vself, VALUE vhash){
  VALUE vary = rb_ary_new();
  TCLIST *list, *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);
  Check_Type(vhash, T_HASH);

  list = vhashtoputlist(vhash);
  if ((result = tcrdbmisc(db, "putlist", 0, list)) != NULL){
    vary = listtovary(result);
    tclistdel(result);
  }
  tclistdel(list);

  return vary;
}

static VALUE cBDB_getlist(int argc, VALUE *argv, VALUE vself){
  VALUE vkeys, vval;
  VALUE vhash = rb_hash_new();
  TCLIST *list, *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);
  rb_scan_args(argc, argv, "*", &vkeys);

  // I really hope there is a better way to do this
  if (RARRAY_LEN(vkeys) == 1) {
    vval = rb_ary_entry(vkeys, 0);
    switch (TYPE(vval)){
      case T_STRING:
      case T_FIXNUM:
        break;
      case T_ARRAY:
        vkeys = vval;
        break;
      case T_OBJECT:
        vkeys = rb_convert_type(vval, T_ARRAY, "Array", "to_a");
        break;
    }
  }
  Check_Type(vkeys, T_ARRAY);

  list = varytolist(vkeys);
  if ((result = tcrdbmisc(db, "getlist", RDBMONOULOG, list)) != NULL){
    vhash = listtovhash(result);
    tclistdel(result);
  }
  tclistdel(list);
  return vhash;
}

static VALUE cBDB_each(VALUE vself){
  VALUE vrv = Qnil;
  const char *kbuf, *vbuf;
  int ksiz, vsiz;
  TCLIST *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);

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
  VALUE vary = rb_ary_new();
  const char *vbuf;
  int vsiz;
  TCLIST *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);

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

static VALUE cBDB_putdup(VALUE vself, VALUE vkey, VALUE vval){
  return cBDB_put_method(vself, vkey, vval, "putdup", false);
}

static VALUE cBDB_putdup_bang(VALUE vself, VALUE vkey, VALUE vval){
  return cBDB_put_method(vself, vkey, vval, "putdup", true);
}

void init_bdb(){
  rb_define_method(cBDB, "putlist", cBDB_putlist, 1);
  rb_define_alias(cBDB, "mput", "putlist");
  rb_define_method(cBDB, "getlist", cBDB_getlist, -1);
  rb_define_alias(cBDB, "mget", "putlist");
  rb_define_method(cBDB, "each", cBDB_each, 0);
  rb_define_method(cBDB, "values", cBDB_values, 0);
  rb_define_method(cBDB, "putdup", cBDB_putdup, 2);
  rb_define_method(cBDB, "putdup!", cBDB_putdup_bang, 2);
}
