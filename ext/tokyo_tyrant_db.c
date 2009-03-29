#include <tokyo_tyrant_db.h>

static void cDB_free(TCRDB *db){
  tcrdbdel(db);
}

static VALUE cDB_close(VALUE vself){
  int ecode;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  if(!tcrdbclose(db)){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "close error: %s", tcrdberrmsg(ecode));
  }

  return Qtrue;
}

static VALUE cDB_initialize(int argc, VALUE *argv, VALUE vself){
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
  rb_iv_set(vself, RDBVNDATA, Data_Wrap_Struct(rb_cObject, 0, cDB_free, db));

  return Qtrue;
}

static VALUE cDB_errmsg(int argc, VALUE *argv, VALUE vself){
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

static VALUE cDB_ecode(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return INT2NUM(tcrdbecode(db));
}

static VALUE cDB_put_method(VALUE vself, VALUE vkey, VALUE vstr, int method){
  int ecode;
  bool res;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  vstr = StringValueEx(vstr);

  switch(method){
    case TTPUT:
      res = tcrdbput2(db, RSTRING_PTR(vkey), RSTRING_PTR(vstr));
      break;
    case TTPUTKEEP:
      res = tcrdbputkeep2(db, RSTRING_PTR(vkey), RSTRING_PTR(vstr));
      break;
    case TTPUTCAT:
      res = tcrdbputcat2(db, RSTRING_PTR(vkey), RSTRING_PTR(vstr));
      break;
    case TTPUTNR:
      res = tcrdbputnr2(db, RSTRING_PTR(vkey), RSTRING_PTR(vstr));
      break;
    default:
      res = false;
      break;
  }

  if(!res){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "put error: %s", tcrdberrmsg(ecode));
  }

  return Qtrue;  
}

static VALUE cDB_put(VALUE vself, VALUE vkey, VALUE vstr){
  return cDB_put_method(vself, vkey, vstr, TTPUT);
}

static VALUE cDB_mput(VALUE vself, VALUE vhash){
  VALUE vary;
  TCRDB *db;
  TCLIST *list, *args;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  args = vhashtolist(vhash);
  list = tcrdbmisc(db, "putlist", 0, args);
  vary = listtovary(list);
  tclistdel(list);
  return vary;
}

static VALUE cDB_putkeep(VALUE vself, VALUE vkey, VALUE vstr){
  return cDB_put_method(vself, vkey, vstr, TTPUTKEEP);  
}

static VALUE cDB_putcat(VALUE vself, VALUE vkey, VALUE vstr){
  return cDB_put_method(vself, vkey, vstr, TTPUTCAT);
}

static VALUE cDB_putnr(VALUE vself, VALUE vkey, VALUE vstr){
  return cDB_put_method(vself, vkey, vstr, TTPUTNR);
}

static VALUE cDB_putshl(VALUE vself, VALUE vkey, VALUE vstr, VALUE vwidth){
  int ecode;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  vstr = StringValueEx(vstr);

  if(!tcrdbputshl2(db, RSTRING_PTR(vkey), RSTRING_PTR(vstr), FIXNUM_P(vwidth))){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "put error: %s", tcrdberrmsg(ecode));
  }

  return Qtrue;
}

static VALUE cDB_out(VALUE vself, VALUE vkey){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  return tcrdbout2(db, RSTRING_PTR(vkey)) ? Qtrue : Qfalse;
}

static VALUE cDB_get(VALUE vself, VALUE vkey){
  VALUE vval;
  char *vbuf;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  if(!(vbuf = tcrdbget2(db, RSTRING_PTR(vkey)))) return Qnil;
  vval = rb_str_new2(vbuf);
  tcfree(vbuf);
  return vval;
}

static VALUE cDB_mget(int argc, VALUE *argv, VALUE vself){
  VALUE vkeys, vhash, vvalue;
  TCRDB *db;
  TCMAP *recs;
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

  recs = varytomap(vkeys);
  if(!tcrdbget3(db, recs)) return Qnil;
  vhash = maptovhash(recs);
  tcmapdel(recs);
  return vhash;
}

static VALUE cDB_vsiz(VALUE vself, VALUE vkey){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  return INT2NUM(tcrdbvsiz2(db, RSTRING_PTR(vkey)));
}

static VALUE cDB_check(VALUE vself, VALUE vkey){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  return tcrdbvsiz2(db, RSTRING_PTR(vkey)) >= 0 ? Qtrue : Qfalse;
}

static VALUE cDB_iterinit(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbiterinit(db) ? Qtrue : Qfalse;
}

static VALUE cDB_iternext(VALUE vself){
  VALUE vval;
  char *vbuf;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  if(!(vbuf = tcrdbiternext2(db))) return Qnil;
  vval = rb_str_new2(vbuf);
  tcfree(vbuf);

  return vval;
}

static VALUE cDB_fwmkeys(int argc, VALUE *argv, VALUE vself){
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

static VALUE cDB_addint(VALUE vself, VALUE vkey, VALUE vnum){
  int num;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vkey = StringValueEx(vkey);

  num = tcrdbaddint(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), NUM2INT(vnum));
  return num == INT_MIN ? Qnil : INT2NUM(num);
}

static VALUE cDB_adddouble(VALUE vself, VALUE vkey, VALUE vnum){
  double num;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  num = tcrdbadddouble(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), NUM2DBL(vnum));
  return isnan(num) ? Qnil : rb_float_new(num);
}

// TODO: Give this more attention, it's untested and needs defaults for scan_args
static VALUE cDB_ext(int argc, VALUE *argv, VALUE vself){
  const char *res;
  VALUE vname, vkey, vvalue, vopts;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  rb_scan_args(argc, argv, "14", &vname, &vkey, &vvalue, &vopts);

  vname = StringValueEx(vname);
  vkey = StringValueEx(vkey);
  vvalue = StringValueEx(vvalue);
  res = tcrdbext2(db, RSTRING_PTR(vname), FIXNUM_P(vopts), RSTRING_PTR(vkey), RSTRING_PTR(vvalue));
  return rb_str_new2(res);
}

static VALUE cDB_sync(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbsync(db) ? Qtrue : Qfalse;
}

static VALUE cDB_vanish(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbvanish(db) ? Qtrue : Qfalse;
}

static VALUE cDB_copy(VALUE vself, VALUE path){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  Check_Type(path, T_STRING);
  return tcrdbcopy(db, RSTRING_PTR(path)) ? Qtrue : Qfalse;
}

static VALUE cDB_restore(VALUE vself, VALUE path, uint64_t ts){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  Check_Type(path, T_STRING);
  return tcrdbrestore(db, RSTRING_PTR(path), ts) ? Qtrue : Qfalse;
}

static VALUE cDB_setmst(VALUE vself, VALUE host, VALUE port){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbsetmst(db, StringValuePtr(host), FIX2INT(port)) ? Qtrue : Qfalse;
}

static VALUE cDB_rnum(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return LL2NUM(tcrdbrnum(db));
}

static VALUE cDB_empty(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return tcrdbrnum(db) < 1 ? Qtrue : Qfalse;
}

static VALUE cDB_size(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return LL2NUM(tcrdbsize(db));
}

static VALUE cDB_stat(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  return rb_str_new2(tcrdbstat(db));
}

static VALUE cDB_misc(int argc, VALUE *argv, VALUE vself){
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

static VALUE cDB_fetch(int argc, VALUE *argv, VALUE vself){
  VALUE vkey, vdef, vval;
  TCRDB *db;
  char *vbuf;
  int vsiz;
  rb_scan_args(argc, argv, "11", &vkey, &vdef);
  vkey = StringValueEx(vkey);
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  if((vbuf = tcrdbget(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), &vsiz)) != NULL){
    vval = rb_str_new(vbuf, vsiz);
    tcfree(vbuf);
  } else {
    vval = vdef;
  }
  return vval;
}

static VALUE cDB_each(VALUE vself){
  VALUE vrv;
  TCRDB *db;
  char *kxstr, *vxstr;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vrv = Qnil;
  tcrdbiterinit(db);
  while((kxstr = tcrdbiternext2(db)) != NULL){
    vxstr = tcrdbget2(db, kxstr);
    vrv = rb_yield_values(2, rb_str_new2(kxstr), rb_str_new2(vxstr));
  }
  return vrv;
}

static VALUE cDB_each_key(VALUE vself){
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

static VALUE cDB_each_value(VALUE vself){
  VALUE vrv;
  TCRDB *db;
  char *kxstr, *vxstr;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vrv = Qnil;
  tcrdbiterinit(db);
  while((kxstr = tcrdbiternext2(db)) != NULL){
    vxstr = tcrdbget2(db, kxstr);
    vrv = rb_yield_values(1, rb_str_new2(vxstr));
  }
  return vrv;
}

static VALUE cDB_keys(VALUE vself){
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

static VALUE cDB_values(VALUE vself){
  VALUE vary;
  TCRDB *db;
  char *kxstr, *vxstr;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vary = rb_ary_new2(tcrdbrnum(db));
  tcrdbiterinit(db);
  while((kxstr = tcrdbiternext2(db)) != NULL){
    vxstr = tcrdbget2(db, kxstr);
    rb_ary_push(vary, rb_str_new2(vxstr));
  }
  return vary;
}

void init_db(){
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

  rb_define_private_method(cDB, "initialize", cDB_initialize, -1);
  rb_define_method(cDB, "close", cDB_close, 0);
  rb_define_method(cDB, "errmsg", cDB_errmsg, -1);
  rb_define_method(cDB, "ecode", cDB_ecode, 0);
  rb_define_method(cDB, "mput", cDB_mput, 1);
  rb_define_method(cDB, "put", cDB_put, 2);
  rb_define_alias(cDB, "[]=", "put");
  rb_define_method(cDB, "putkeep", cDB_putkeep, 2);
  rb_define_method(cDB, "putcat", cDB_putcat, 2);
  rb_define_method(cDB, "putshl", cDB_putshl, 2);
  rb_define_method(cDB, "putnr", cDB_putnr, 2);
  rb_define_method(cDB, "out", cDB_out, 1);
  rb_define_method(cDB, "get", cDB_get, 1);
  rb_define_alias(cDB, "[]", "get");
  rb_define_method(cDB, "mget", cDB_mget, -1);
  rb_define_method(cDB, "vsiz", cDB_vsiz, 1);
  rb_define_method(cDB, "check", cDB_check, 1);
  rb_define_alias(cDB, "has_key?", "check");
  rb_define_alias(cDB, "key?", "check");
  /*
  rb_define_method(cDB, "check_value", cDB_check_value, 1);
  rb_define_alias(cDB, "has_value?", "check_value");
  rb_define_alias(cDB, "value?", "check_value");
  */
  rb_define_alias(cDB, "include?", "check");
  rb_define_alias(cDB, "member?", "check");
  rb_define_method(cDB, "iterinit", cDB_iterinit, 0);
  rb_define_method(cDB, "iternext", cDB_iternext, 0);
  rb_define_method(cDB, "fwmkeys", cDB_fwmkeys, -1);
  rb_define_method(cDB, "addint", cDB_addint, 2);
  rb_define_method(cDB, "adddouble", cDB_adddouble, 2);
  rb_define_method(cDB, "ext", cDB_ext, -1);
  rb_define_method(cDB, "sync", cDB_sync, 0);
  rb_define_method(cDB, "vanish", cDB_vanish, 0);
  rb_define_alias(cDB, "clear", "vanish");
  rb_define_method(cDB, "copy", cDB_copy, 1);
  rb_define_method(cDB, "restore", cDB_restore, 2);
  rb_define_method(cDB, "setmst", cDB_setmst, 2);
  rb_define_method(cDB, "rnum", cDB_rnum, 0);
  rb_define_alias(cDB, "count", "rnum");
  rb_define_method(cDB, "empty?", cDB_empty, 0);
  rb_define_method(cDB, "size", cDB_size, 0);
  rb_define_alias(cDB, "length", "size");
  rb_define_method(cDB, "stat", cDB_stat, 0);
  rb_define_method(cDB, "misc", cDB_misc, -1);
  rb_define_method(cDB, "fetch", cDB_fetch, -1);
  rb_define_method(cDB, "each", cDB_each, 0);
  rb_define_alias(cDB, "each_pair", "each");
  rb_define_method(cDB, "each_key", cDB_each_key, 0);
  rb_define_method(cDB, "each_value", cDB_each_value, 0);
  rb_define_method(cDB, "keys", cDB_keys, 0);
  rb_define_method(cDB, "values", cDB_values, 0);
}
