#include <tokyo_tyrant_db.h>

static VALUE cDB_put_method(VALUE vself, VALUE vkey, VALUE vstr, int method){
  bool res;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vkey = StringValueEx(vkey);
  vstr = StringValueEx(vstr);

  switch(method){
    case TTPUT:
      res = tcrdbput(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vstr), RSTRING_LEN(vstr));
      break;
    case TTPUTKEEP:
      res = tcrdbputkeep(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vstr), RSTRING_LEN(vstr));
      break;
    case TTPUTCAT:
      res = tcrdbputcat(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vstr), RSTRING_LEN(vstr));
      break;
    case TTPUTNR:
      res = tcrdbputnr(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vstr), RSTRING_LEN(vstr));
      break;
    default:
      res = false;
      break;
  }

  if(!res) mTokyoTyrant_exception(vself, NULL);

  return Qtrue;
}

static VALUE cDB_put(VALUE vself, VALUE vkey, VALUE vstr){
  return cDB_put_method(vself, vkey, vstr, TTPUT);
}

static VALUE cDB_mput(VALUE vself, VALUE vhash){
  VALUE vary = Qnil;
  TCLIST *list, *args;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  args = vhashtolist(vhash);
  if ((list = tcrdbmisc(db, "putlist", 0, args)) != NULL){
    vary = listtovary(list);
    tclistdel(list);
  }
  tclistdel(args);
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
  bool res;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vkey = StringValueEx(vkey);
  vstr = StringValueEx(vstr);

  res = tcrdbputshl(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vstr), RSTRING_LEN(vstr), FIXNUM_P(vwidth));

  if(!res) mTokyoTyrant_exception(vself, NULL);

  return Qtrue;
}

static VALUE cDB_get(VALUE vself, VALUE vkey){
  VALUE vval = Qnil;
  char *buf;
  int bsiz, ecode;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  // this is ugly
  vkey = StringValueEx(vkey);
  if((buf = tcrdbget(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), &bsiz)) == NULL){
    if ((ecode = tcrdbecode(db))) {
      if (ecode != TTENOREC) mTokyoTyrant_exception(vself, NULL);
    }
  } else {
    vval = StringRaw(buf, bsiz);
    tcfree(buf);
  }

  return vval;
}

static VALUE cDB_mget(int argc, VALUE *argv, VALUE vself){
  VALUE vkeys, vval;
  VALUE vhash = Qnil;
  TCMAP *recs;
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
      case T_STRUCT: // range is not a T_STRUCT instead of a T_OBJECT in ruby1.9?
      case T_OBJECT:
        vkeys = rb_convert_type(vval, T_ARRAY, "Array", "to_a");
        break;
    }
  }

  Check_Type(vkeys, T_ARRAY);

  recs = varytomap(vkeys);
  if(tcrdbget3(db, recs)){
    vhash = maptovhash(recs);
  }
  tcmapdel(recs);
  return vhash;
}

static VALUE cDB_vsiz(VALUE vself, VALUE vkey){
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vkey = StringValueEx(vkey);
  return INT2NUM(tcrdbvsiz(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey)));
}

static VALUE cDB_fetch(int argc, VALUE *argv, VALUE vself){
  VALUE vkey, vrv, vforce;
  rb_scan_args(argc, argv, "11", &vkey, &vforce);
  if(!rb_block_given_p()) rb_raise(rb_eArgError, "no block given");
  if(vforce == Qnil) vforce = Qfalse;

  if(vforce != Qfalse || (vrv = cDB_get(vself, vkey)) == Qnil){
    vrv = rb_yield(vkey);
    cDB_put(vself, vkey, vrv);
  }
  return vrv;
}

static VALUE cDB_each(VALUE vself){
  VALUE vrv;
  if(!rb_block_given_p()) rb_raise(rb_eArgError, "no block given");
  TCRDB *db = mTokyoTyrant_getdb(vself);
  vrv = Qnil;
  tcrdbiterinit(db);
  int ksiz;
  char *kbuf;

  while((kbuf = tcrdbiternext(db, &ksiz)) != NULL){
    VALUE vkey = rb_str_new(kbuf, ksiz);
    VALUE vval = cDB_get(vself, vkey);

    vrv = rb_yield_values(2, vkey, vval);
    tcfree(kbuf);
  }
  return vrv;
}

static VALUE cDB_each_value(VALUE vself){
  VALUE vrv;
  if(!rb_block_given_p()) rb_raise(rb_eArgError, "no block given");
  TCRDB *db = mTokyoTyrant_getdb(vself);
  vrv = Qnil;
  tcrdbiterinit(db);
  int ksiz;
  char *kbuf;

  while((kbuf = tcrdbiternext(db, &ksiz)) != NULL){
    VALUE vkey = rb_str_new(kbuf, ksiz);
    VALUE vval = cDB_get(vself, vkey);

    vrv = rb_yield_values(1, vval);
    tcfree(kbuf);
  }
  return vrv;
}

static VALUE cDB_values(VALUE vself){
  VALUE vary;
  TCRDB *db = mTokyoTyrant_getdb(vself);
  vary = rb_ary_new2(tcrdbrnum(db));
  tcrdbiterinit(db);
  int ksiz;
  char *kbuf;

  while((kbuf = tcrdbiternext(db, &ksiz)) != NULL){
    VALUE vkey = rb_str_new(kbuf, ksiz);
    VALUE vval = cDB_get(vself, vkey);

    rb_ary_push(vary, vval);
    tcfree(kbuf);
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
  rb_define_method(cDB, "putshl", cDB_putshl, 3);
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
