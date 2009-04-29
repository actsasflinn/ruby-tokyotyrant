#include <tokyo_tyrant_db.h>

static void mTokyoTyrant_free(TCRDB *db){
  tcrdbdel(db);
}

static VALUE mTokyoTyrant_close(VALUE vself){
  int ecode;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  if(!tcrdbclose(db)){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "close error: %s", tcrdberrmsg(ecode));
  }
  return Qtrue;
}

static VALUE mTokyoTyrant_initialize(int argc, VALUE *argv, VALUE vself){
  VALUE host, port;
  int ecode;
  TCRDB *db;

  rb_scan_args(argc, argv, "02", &host, &port);
  if(NIL_P(host)) host = rb_str_new2("127.0.0.1");
  if(NIL_P(port)) port = INT2FIX(1978);

  db = tcrdbnew();

  if(!tcrdbopen(db, StringValuePtr(host), FIX2INT(port))){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "open error: %s", tcrdberrmsg(ecode));
  }

  rb_iv_set(vself, "@host", host);
  rb_iv_set(vself, "@port", port);
  rb_iv_set(vself, RDBVNDATA, Data_Wrap_Struct(rb_cObject, 0, mTokyoTyrant_free, db));

  return Qtrue;
}

static VALUE mTokyoTyrant_errmsg(int argc, VALUE *argv, VALUE vself){
  VALUE vecode;
  const char *msg;
  int ecode;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  rb_scan_args(argc, argv, "01", &vecode);

  ecode = (vecode == Qnil) ? tcrdbecode(db) : NUM2INT(vecode);
  msg = tcrdberrmsg(ecode);
  return rb_str_new2(msg);
}

static VALUE mTokyoTyrant_ecode(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return INT2NUM(tcrdbecode(db));
}

static VALUE mTokyoTyrant_out(VALUE vself, VALUE vkey){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  return tcrdbout2(db, RSTRING_PTR(vkey)) ? Qtrue : Qfalse;
}

// TODO: merge out and mout?
static VALUE mTokyoTyrant_outlist(int argc, VALUE *argv, VALUE vself){
  VALUE vkeys, vary, vvalue;
  TCRDB *db;
  TCLIST *list, *result;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
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
  result = tcrdbmisc(db, "outlist", 0, list);
  tclistdel(list);
  vary = listtovary(result);
  tclistdel(result);
  return vary;
}

static VALUE mTokyoTyrant_check(VALUE vself, VALUE vkey){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  return tcrdbvsiz2(db, RSTRING_PTR(vkey)) >= 0 ? Qtrue : Qfalse;
}

static VALUE mTokyoTyrant_iterinit(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbiterinit(db) ? Qtrue : Qfalse;
}

static VALUE mTokyoTyrant_iternext(VALUE vself){
  VALUE vval;
  char *vbuf;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  if(!(vbuf = tcrdbiternext2(db))) return Qnil;
  vval = rb_str_new2(vbuf);
  tcfree(vbuf);

  return vval;
}

static VALUE mTokyoTyrant_fwmkeys(int argc, VALUE *argv, VALUE vself){
  VALUE vprefix, vmax, vary;
  TCLIST *keys;
  int max;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  rb_scan_args(argc, argv, "11", &vprefix, &vmax);

  vprefix = StringValueEx(vprefix);
  max = (vmax == Qnil) ? -1 : NUM2INT(vmax);
  keys = tcrdbfwmkeys(db, RSTRING_PTR(vprefix), RSTRING_LEN(vprefix), max);
  vary = listtovary(keys);
  tclistdel(keys);
  return vary;
}

static VALUE mTokyoTyrant_keys(VALUE vself){
  /*
  VALUE vary;
  TCRDB *db;
  char *kxstr;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vary = rb_ary_new2(tcrdbrnum(db));
  tcrdbiterinit(db);
  while((kxstr = tcrdbiternext2(db)) != NULL){
    rb_ary_push(vary, rb_str_new2(kxstr));
  }
  return vary;
  */

  // Using forward matching keys with an empty string is 100x faster than iternext+get
  VALUE vary;
  TCLIST *keys;
  TCRDB *db;
  char *prefix;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  prefix = "";
  keys = tcrdbfwmkeys2(db, prefix, -1);
  vary = listtovary(keys);
  tclistdel(keys);
  return vary;
}

static VALUE mTokyoTyrant_add_int(VALUE vself, VALUE vkey, VALUE vnum){
  int num;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vkey = StringValueEx(vkey);

  num = tcrdbaddint(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), NUM2INT(vnum));
  return num == INT_MIN ? Qnil : INT2NUM(num);
}

static VALUE mTokyoTyrant_get_int(VALUE vself, VALUE vkey){
  return mTokyoTyrant_add_int(vself, vkey, INT2NUM(0));
}

static VALUE mTokyoTyrant_add_double(VALUE vself, VALUE vkey, VALUE vnum){
  double num;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  num = tcrdbadddouble(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), NUM2DBL(vnum));
  return isnan(num) ? Qnil : rb_float_new(num);
}

static VALUE mTokyoTyrant_get_double(VALUE vself, VALUE vkey){
  return mTokyoTyrant_add_double(vself, vkey, INT2NUM(0));
}

static VALUE mTokyoTyrant_sync(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbsync(db) ? Qtrue : Qfalse;
}

static VALUE mTokyoTyrant_vanish(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbvanish(db) ? Qtrue : Qfalse;
}

static VALUE mTokyoTyrant_copy(VALUE vself, VALUE path){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  Check_Type(path, T_STRING);
  return tcrdbcopy(db, RSTRING_PTR(path)) ? Qtrue : Qfalse;
}

static VALUE mTokyoTyrant_restore(VALUE vself, VALUE path, uint64_t ts){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  Check_Type(path, T_STRING);
  return tcrdbrestore(db, RSTRING_PTR(path), ts) ? Qtrue : Qfalse;
}

static VALUE mTokyoTyrant_setmst(VALUE vself, VALUE host, VALUE port){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbsetmst(db, StringValuePtr(host), FIX2INT(port)) ? Qtrue : Qfalse;
}

static VALUE mTokyoTyrant_rnum(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return LL2NUM(tcrdbrnum(db));
}

static VALUE mTokyoTyrant_empty(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbrnum(db) < 1 ? Qtrue : Qfalse;
}

static VALUE mTokyoTyrant_size(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return LL2NUM(tcrdbsize(db));
}

static VALUE mTokyoTyrant_stat(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return rb_str_new2(tcrdbstat(db));
}

static VALUE mTokyoTyrant_misc(int argc, VALUE *argv, VALUE vself){
  VALUE vname, vopts, vargs;
  TCRDB *db;
  TCLIST *list, *args;
  VALUE vary;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  rb_scan_args(argc, argv, "13", &vname, &vopts, &vargs);

  args = varytolist(vargs);
  vname = StringValueEx(vname);

  list = tcrdbmisc(db, RSTRING_PTR(vname), NUM2INT(vopts), args);
  vary = listtovary(list);
  tclistdel(list);
  return vary;
}

static VALUE mTokyoTyrant_each_key(VALUE vself){
  VALUE vrv;
  TCRDB *db;
  char *kxstr;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vrv = Qnil;
  tcrdbiterinit(db);
  while((kxstr = tcrdbiternext2(db)) != NULL){
    vrv = rb_yield_values(1, rb_str_new2(kxstr));
  }
  return vrv;
}

void init_mod(){
  rb_define_const(mTokyoTyrant, "ESUCCESS", INT2NUM(TTESUCCESS));
  rb_define_const(mTokyoTyrant, "EINVALID", INT2NUM(TTEINVALID));
  rb_define_const(mTokyoTyrant, "ENOHOST", INT2NUM(TTENOHOST));
  rb_define_const(mTokyoTyrant, "EREFUSED", INT2NUM(TTEREFUSED));
  rb_define_const(mTokyoTyrant, "ESEND", INT2NUM(TTESEND));
  rb_define_const(mTokyoTyrant, "ERECV", INT2NUM(TTERECV));
  rb_define_const(mTokyoTyrant, "EKEEP", INT2NUM(TTEKEEP));
  rb_define_const(mTokyoTyrant, "ENOREC", INT2NUM(TTENOREC));
  rb_define_const(mTokyoTyrant, "EMISC", INT2NUM(TTEMISC));

  rb_define_const(mTokyoTyrant, "ITLEXICAL", INT2NUM(RDBITLEXICAL));
  rb_define_const(mTokyoTyrant, "ITDECIMAL", INT2NUM(RDBITDECIMAL));
  rb_define_const(mTokyoTyrant, "ITVOID", INT2NUM(RDBITVOID));
  rb_define_const(mTokyoTyrant, "ITKEEP", INT2NUM(RDBITKEEP));

  rb_define_private_method(mTokyoTyrant, "initialize", mTokyoTyrant_initialize, -1);
  rb_define_method(mTokyoTyrant, "close", mTokyoTyrant_close, 0);
  rb_define_method(mTokyoTyrant, "errmsg", mTokyoTyrant_errmsg, -1);
  rb_define_method(mTokyoTyrant, "ecode", mTokyoTyrant_ecode, 0);
  rb_define_method(mTokyoTyrant, "out", mTokyoTyrant_out, 1);
  rb_define_alias(mTokyoTyrant, "delete", "out");                 // Rufus Compat
  rb_define_method(mTokyoTyrant, "outlist", mTokyoTyrant_outlist, -1);
  rb_define_alias(mTokyoTyrant, "mdelete", "outlist");
  rb_define_alias(mTokyoTyrant, "ldelete", "outlist");            // Rufus Compat
  rb_define_method(mTokyoTyrant, "check", mTokyoTyrant_check, 1);
  rb_define_alias(mTokyoTyrant, "has_key?", "check");
  rb_define_alias(mTokyoTyrant, "key?", "check");
  rb_define_alias(mTokyoTyrant, "include?", "check");
  rb_define_alias(mTokyoTyrant, "member?", "check");
  rb_define_method(mTokyoTyrant, "iterinit", mTokyoTyrant_iterinit, 0);
  rb_define_method(mTokyoTyrant, "iternext", mTokyoTyrant_iternext, 0);
  rb_define_method(mTokyoTyrant, "fwmkeys", mTokyoTyrant_fwmkeys, -1);
  rb_define_method(mTokyoTyrant, "keys", mTokyoTyrant_keys, 0);
  rb_define_method(mTokyoTyrant, "add_int", mTokyoTyrant_add_int, 2);
  rb_define_method(mTokyoTyrant, "get_int", mTokyoTyrant_get_int, 1);
  rb_define_method(mTokyoTyrant, "add_double", mTokyoTyrant_add_double, 2);
  rb_define_method(mTokyoTyrant, "get_double", mTokyoTyrant_get_double, 1);
  rb_define_method(mTokyoTyrant, "sync", mTokyoTyrant_sync, 0);
  rb_define_method(mTokyoTyrant, "vanish", mTokyoTyrant_vanish, 0);
  rb_define_alias(mTokyoTyrant, "clear", "vanish");
  rb_define_method(mTokyoTyrant, "copy", mTokyoTyrant_copy, 1);
  rb_define_method(mTokyoTyrant, "restore", mTokyoTyrant_restore, 2);
  rb_define_method(mTokyoTyrant, "setmst", mTokyoTyrant_setmst, 2);
  rb_define_method(mTokyoTyrant, "rnum", mTokyoTyrant_rnum, 0);
  rb_define_alias(mTokyoTyrant, "count", "rnum");
  rb_define_method(mTokyoTyrant, "empty?", mTokyoTyrant_empty, 0);
  rb_define_alias(mTokyoTyrant, "size", "rnum");                    // Rufus Compat
  rb_define_method(mTokyoTyrant, "db_size", mTokyoTyrant_size, 0);  // Rufus Compat
  rb_define_alias(mTokyoTyrant, "length", "size");
  rb_define_method(mTokyoTyrant, "stat", mTokyoTyrant_stat, 0);
  rb_define_method(mTokyoTyrant, "misc", mTokyoTyrant_misc, -1);
  rb_define_method(mTokyoTyrant, "each_key", mTokyoTyrant_each_key, 0);
}