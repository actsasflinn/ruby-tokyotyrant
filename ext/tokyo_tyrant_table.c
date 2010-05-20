#include <tokyo_tyrant_table.h>

static VALUE cTable_put_method(VALUE vself, VALUE vkey, VALUE vcols, int method){
  bool res;
  TCMAP *cols;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vkey = StringValueEx(vkey);
  Check_Type(vcols, T_HASH);
  cols = vhashtomap(vcols);

  // there's probably a more elegant way yo do this
  switch(method){
    case TTPUT:
      res = tcrdbtblput(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), cols);
      break;
    case TTPUTKEEP:
      res = tcrdbtblputkeep(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), cols);
      break;
    case TTPUTCAT:
      res = tcrdbtblputcat(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey), cols);
      break;
    default :
      res = false;
      break;
  }

  if(!res) mTokyoTyrant_exception(vself, NULL);
  tcmapdel(cols);
  return Qtrue;
}

static VALUE cTable_put(VALUE vself, VALUE vkey, VALUE vcols){
  return cTable_put_method(vself, vkey, vcols, TTPUT);
}

static VALUE cTable_mput(VALUE vself, VALUE vhash){
  int i, num, j;
  VALUE vkeys, vkey, vval;
  VALUE vary = Qnil;
  TCLIST *list, *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vkeys = rb_funcall(vhash, rb_intern("keys"), 0);
  num = RARRAY_LEN(vkeys);
  list = tclistnew2(num * 2);
  for(i = 0; i < num; i++){
    vkey = rb_ary_entry(vkeys, i);
    vval = rb_hash_aref(vhash, vkey);

    vkey = StringValueEx(vkey);
    tclistpush2(list, RSTRING_PTR(vkey));

    TCLIST *cols = vhashtolist(vval);
    TCXSTR *xstr = tcxstrnew();

    for(j = 0; j < tclistnum(cols); j++){
      int rsiz;
      const char *rbuf = tclistval(cols, j, &rsiz);
      if (j > 0) tcxstrcat(xstr, "\0", 1);
      tcxstrcat(xstr, rbuf, rsiz);
    }
    tclistpush(list, tcxstrptr(xstr), tcxstrsize(xstr));
    tclistdel(cols);
    tcxstrdel(xstr);
  }

  if ((result = tcrdbmisc(db, "putlist", 0, list)) != NULL){
    vary = listtovary(result);
    tclistdel(result);
  }
  tclistdel(list);
  return vary;
}

static VALUE cTable_putkeep(VALUE vself, VALUE vkey, VALUE vcols){
  return cTable_put_method(vself, vkey, vcols, TTPUTKEEP);
}

static VALUE cTable_putcat(VALUE vself, VALUE vkey, VALUE vcols){
  return cTable_put_method(vself, vkey, vcols, TTPUTCAT);
}

static VALUE cTable_out(VALUE vself, VALUE vkey){
  TCRDB *db = mTokyoTyrant_getdb(vself);
  vkey = StringValueEx(vkey);

  return tcrdbtblout(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey)) ? Qtrue : Qfalse;
}

static VALUE cTable_get(VALUE vself, VALUE vkey){
  VALUE vcols = Qnil;
  int ecode;
  TCMAP *cols;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vkey = StringValueEx(vkey);
  if((cols = tcrdbtblget(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey))) == NULL){
    if ((ecode = tcrdbecode(db))) {
      if (ecode != TTENOREC) mTokyoTyrant_exception(vself, NULL);
    }
  } else {
    vcols = maptovhash(cols);
    tcmapdel(cols);
  }
  return vcols;
}

static VALUE cTable_mget(int argc, VALUE *argv, VALUE vself){
  const char *kbuf;
  int ksiz, vsiz;
  VALUE vkeys, vcols, vval;
  VALUE vhash = Qnil;
  TCMAP *recs, *cols;
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
    vhash = rb_hash_new();
    tcmapiterinit(recs);
    while((kbuf = tcmapiternext(recs, &ksiz)) != NULL){
      const char *vbuf = tcmapiterval(kbuf, &vsiz);
      cols = tcstrsplit4(vbuf, vsiz);
      vcols = maptovhash(cols);
      tcmapdel(cols);
      rb_hash_aset(vhash, StringRaw(kbuf, ksiz), vcols);
    }
  }
  tcmapdel(recs);
  return vhash;
}

static VALUE cTable_setindex(VALUE vself, VALUE vname, VALUE vtype){
  TCRDB *db = mTokyoTyrant_getdb(vself);
  vname = StringValueEx(vname);
  if (TYPE(vtype) == T_SYMBOL) vtype = rb_str_new2(rb_id2name(SYM2ID(vtype)));

  if (TYPE(vtype) == T_STRING){
    vtype = StringValueEx(vtype);
    vtype = tctdbstrtoindextype(RSTRING_PTR(vtype));
    vtype = INT2NUM(vtype);
  }

  return tcrdbtblsetindex(db, RSTRING_PTR(vname), NUM2INT(vtype)) ? Qtrue : Qfalse;
}

static VALUE cTable_genuid(VALUE vself){
  TCRDB *db = mTokyoTyrant_getdb(vself);
  return LL2NUM(tcrdbtblgenuid(db));
}

static VALUE cTable_fetch(int argc, VALUE *argv, VALUE vself){
  VALUE vkey, vrv, vforce;
  rb_scan_args(argc, argv, "11", &vkey, &vforce);
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  if(vforce == Qnil) vforce = Qfalse;
  vkey = StringValueEx(vkey);

  if(vforce != Qfalse ||
     (vrv = cTable_get(vself, vkey)) == Qnil){
    vrv = rb_yield(vkey);
    cTable_put(vself, vkey, vrv);
  }
  return vrv;
}

static VALUE cTable_each(VALUE vself){
  VALUE vrv = Qnil;
  char *kbuf;
  int ksiz;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  TCRDB *db = mTokyoTyrant_getdb(vself);

  tcrdbiterinit(db);
  while((kbuf = tcrdbiternext(db, &ksiz)) != NULL){
    VALUE vkey = rb_str_new(kbuf, ksiz);
    VALUE vval = cTable_get(vself, vkey);

    vrv = rb_yield_values(2, vkey, vval);
    tcfree(kbuf);
  }
  return vrv;
}

static VALUE cTable_each_value(VALUE vself){
  VALUE vrv = Qnil;
  char *kbuf;
  int ksiz;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  TCRDB *db = mTokyoTyrant_getdb(vself);

  tcrdbiterinit(db);
  while((kbuf = tcrdbiternext(db, &ksiz)) != NULL){
    VALUE vkey = rb_str_new(kbuf, ksiz);
    VALUE vval = cTable_get(vself, vkey);

    vrv = rb_yield_values(1, vval);
    tcfree(kbuf);
  }
  return vrv;
}

static VALUE cTable_values(VALUE vself){
  VALUE vary = rb_ary_new();;
  char *kbuf;
  int ksiz;
  TCRDB *db = mTokyoTyrant_getdb(vself);

  vary = rb_ary_new2(tcrdbrnum(db));
  tcrdbiterinit(db);
  while((kbuf = tcrdbiternext(db, &ksiz)) != NULL){
    VALUE vkey = rb_str_new(kbuf, ksiz);
    VALUE vval = cTable_get(vself, vkey);

    rb_ary_push(vary, vval);
    tcfree(kbuf);
  }
  return vary;
}

// Probably should dry these up
static VALUE cTable_prepare_query(VALUE vself){
  VALUE vqry;
  vqry = rb_class_new_instance(1, &vself, rb_path2class("TokyoTyrant::Query"));
  if(rb_block_given_p()) rb_yield_values(1, vqry);
  return vqry;
}

static VALUE cTable_query(VALUE vself){
  VALUE vqry, vary;
  vqry = rb_class_new_instance(1, &vself, rb_path2class("TokyoTyrant::Query"));
  if(rb_block_given_p()) {
    rb_yield_values(1, vqry);
    vary = rb_funcall(vqry, rb_intern("run"), 0);
    return vary;
  } else {
    return vqry;
  }
}

static VALUE cTable_find(VALUE vself){
  VALUE vqry, vary;
  vqry = rb_class_new_instance(1, &vself, rb_path2class("TokyoTyrant::Query"));
  if(rb_block_given_p()) rb_yield_values(1, vqry);
  vary = rb_funcall(vqry, rb_intern("get"), 0);
  return vary;
}

static VALUE cTable_search(int argc, VALUE *argv, VALUE vself){
  VALUE vqrys, vkeys, vtype;
  int qsiz, type, j;

  rb_scan_args(argc, argv, "1*", &vtype, &vqrys);

  qsiz = argc - 1;
  RDBQRY *qrys[qsiz];

  vtype = StringValueEx(vtype);
  type = tctdbstrtometasearcytype(RSTRING_PTR(vtype));

  for(j = 0; j < qsiz; j++){
    VALUE vqry = rb_iv_get(rb_ary_entry(vqrys, j), RDBQRYVNDATA);
    Data_Get_Struct(vqry, RDBQRY, qrys[j]);
  }
  TCLIST *res = tcrdbmetasearch(qrys, qsiz, type);
  vkeys = listtovary(res);
  tclistdel(res);

  return vkeys;
}

void init_table(){
  rb_define_method(cTable, "mput", cTable_mput, 1);
  rb_define_alias(cTable, "lput", "mput");                    // Rufus Compat
  rb_define_method(cTable, "put", cTable_put, 2);
  rb_define_alias(cTable, "[]=", "put");
  rb_define_method(cTable, "putkeep", cTable_putkeep, 2);
  rb_define_method(cTable, "putcat", cTable_putcat, 2);
  rb_define_method(cTable, "out", cTable_out, 1);
  rb_define_alias(cTable, "delete", "out");                   // Rufus Compat
  rb_define_method(cTable, "get", cTable_get, 1);
  rb_define_method(cTable, "mget", cTable_mget, -1);
  rb_define_alias(cTable, "lget", "mget");                    // Rufus Compat
  rb_define_alias(cTable, "[]", "get");
  rb_define_method(cTable, "set_index", cTable_setindex, 2);  // Rufus Compat
  rb_define_method(cTable, "genuid", cTable_genuid, 0);
  rb_define_alias(cTable, "generate_unique_id", "genuid");
  rb_define_method(cTable, "fetch", cTable_fetch, -1);
  rb_define_method(cTable, "each", cTable_each, 0);
  rb_define_alias(cTable, "each_pair", "each");
  rb_define_method(cTable, "each_value", cTable_each_value, 0);
  rb_define_method(cTable, "values", cTable_values, 0);
  rb_define_method(cTable, "prepare_query", cTable_prepare_query, 0);
  rb_define_method(cTable, "query", cTable_query, 0);
  rb_define_method(cTable, "find", cTable_find, 0);
  rb_define_method(cTable, "search", cTable_search, -1);
}
