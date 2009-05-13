#include <tokyo_tyrant_db.h>

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
  tclistdel(args);
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

static VALUE cDB_get(VALUE vself, VALUE vkey){
  VALUE vval;
  char *buf;
  int bsiz, ecode;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  // this is ugly
  vkey = StringValueEx(vkey);
  if(!(buf = tcrdbget(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), &bsiz))){
    if ((ecode = tcrdbecode(db))) {
      if (ecode != TTENOREC) {
        rb_raise(eTokyoTyrantError, "get error: %s", tcrdberrmsg(ecode));
      }
    }
    return Qnil;
  } else {
    vval = unpackTokyoValue(buf, bsiz);
  }

  tcfree(buf);
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
    tcfree(vxstr);
    tcfree(kxstr);
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
    tcfree(vxstr);
    tcfree(kxstr);
  }
  return vrv;
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
    tcfree(vxstr);
    tcfree(kxstr);
  }
  return vary;
}

void init_db(){
  rb_define_method(cDB, "mput", cDB_mput, 1);
  rb_define_alias(cDB, "lput", "mput");       // Rufus Compat
  rb_define_method(cDB, "put", cDB_put, 2);
  rb_define_alias(cDB, "[]=", "put");
  rb_define_method(cDB, "putkeep", cDB_putkeep, 2);
  rb_define_method(cDB, "putcat", cDB_putcat, 2);
  rb_define_method(cDB, "putshl", cDB_putshl, 2);
  rb_define_method(cDB, "putnr", cDB_putnr, 2);
  rb_define_method(cDB, "get", cDB_get, 1);
  rb_define_alias(cDB, "[]", "get");
  rb_define_method(cDB, "mget", cDB_mget, -1);
  rb_define_alias(cDB, "lget", "mget");       // Rufus Compat
  rb_define_method(cDB, "vsiz", cDB_vsiz, 1);
  /*
  rb_define_method(cDB, "check_value", cDB_check_value, 1);
  rb_define_alias(cDB, "has_value?", "check_value");
  rb_define_alias(cDB, "value?", "check_value");
  */

  rb_define_method(cDB, "fetch", cDB_fetch, -1);
  rb_define_method(cDB, "each", cDB_each, 0);
  rb_define_alias(cDB, "each_pair", "each");
  rb_define_method(cDB, "each_value", cDB_each_value, 0);
  rb_define_method(cDB, "values", cDB_values, 0);
}
