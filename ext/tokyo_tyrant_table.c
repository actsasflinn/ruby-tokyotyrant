#include <tokyo_tyrant_table.h>

static VALUE cTable_put_method(VALUE vself, VALUE vkey, VALUE vcols, int method){
  int ecode;
  bool res;
  TCMAP *cols;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

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

  if(!res){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "put error: %s\n", tcrdberrmsg(ecode));
  }
  tcmapdel(cols);

  return Qtrue;
}

static VALUE cTable_put(VALUE vself, VALUE vkey, VALUE vcols){
  return cTable_put_method(vself, vkey, vcols, TTPUT);
}

static VALUE cTable_mput(VALUE vself, VALUE vhash){
  int i, num, j;
  VALUE vary, vkeys, vkey, vval;
  TCRDB *db;
  TCLIST *list;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

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
    tcxstrdel(xstr);
  }
  list = tcrdbmisc(db, "putlist", 0, list);
  vary = listtovary(list);
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
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vkey = StringValueEx(vkey);

  return tcrdbtblout(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey)) ? Qtrue : Qfalse;
}

static VALUE cTable_get(VALUE vself, VALUE vkey){
  VALUE vcols;
  TCRDB *db;
  TCMAP *cols;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vkey = StringValueEx(vkey);

  if(!(cols = tcrdbtblget(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey)))) return Qnil;
  vcols = maptovhashsym(cols);
  tcmapdel(cols);
  return vcols;
}

static VALUE cTable_mget(int argc, VALUE *argv, VALUE vself){
  const char *kbuf, *vbuf;
  int ksiz, vsiz;
  VALUE vkeys, vhash, vcols, vvalue;
  TCRDB *db;
  TCMAP *recs, *cols;
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
  vhash = rb_hash_new();
  tcmapiterinit(recs);
  while((kbuf = tcmapiternext(recs, &ksiz)) != NULL){
    vbuf = tcmapiterval(kbuf, &vsiz);
    cols = tcstrsplit4(vbuf, vsiz);
    vcols = maptovhashsym(cols);
    tcmapdel(cols);
    rb_hash_aset(vhash, rb_str_new(kbuf, ksiz), vcols);
  }
  tcmapdel(recs);
  return vhash;
}

static VALUE cTable_setindex(VALUE vself, VALUE vname, VALUE vtype){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  Check_Type(vname, T_STRING);
  return tcrdbtblsetindex(db, RSTRING_PTR(vname), NUM2INT(vtype)) ? Qtrue : Qfalse;
}

static VALUE cTable_genuid(VALUE vself){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  return LL2NUM(tcrdbtblgenuid(db));
}

static VALUE cTable_fetch(int argc, VALUE *argv, VALUE vself){
  VALUE vkey, vdef, vval;
  TCRDB *db;
  TCMAP *cols;
  rb_scan_args(argc, argv, "11", &vkey, &vdef);
  vkey = StringValueEx(vkey);
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  if((cols = tcrdbtblget(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey))) != NULL){
    vval = maptovhashsym(cols);
    tcmapdel(cols);
  } else {
    vval = vdef;
  }
  return vval;
}

static VALUE cTable_each(VALUE vself){
  VALUE vrv;
  TCRDB *db;
  TCMAP *cols;
  char *kbuf;
  int ksiz;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vrv = Qnil;
  tcrdbiterinit(db);
  while((kbuf = tcrdbiternext(db, &ksiz)) != NULL){
    if((cols = tcrdbtblget(db, kbuf, ksiz)) != NULL){
      vrv = rb_yield_values(2, rb_str_new(kbuf, ksiz), maptovhashsym(cols));
      tcmapdel(cols);
    } else {
      vrv = rb_yield_values(2, rb_str_new(kbuf, ksiz), Qnil);
    }
    tcfree(kbuf);
  }
  return vrv;
}

static VALUE cTable_each_value(VALUE vself){
  VALUE vrv;
  TCRDB *db;
  TCMAP *cols;
  char *kbuf;
  int ksiz;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vrv = Qnil;
  tcrdbiterinit(db);
  while((kbuf = tcrdbiternext(db, &ksiz)) != NULL){
    if((cols = tcrdbtblget(db, kbuf, ksiz)) != NULL){
      vrv = rb_yield(maptovhashsym(cols));
      tcmapdel(cols);
    } else {
      vrv = rb_yield(Qnil);
    }
    tcfree(kbuf);
  }
  return vrv;
}

static VALUE cTable_values(VALUE vself){
  VALUE vary;
  TCRDB *db;
  TCMAP *cols;
  char *kxstr;
  int ksiz;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vary = rb_ary_new2(tcrdbrnum(db));
  tcrdbiterinit(db);
  while((kxstr = tcrdbiternext(db, &ksiz)) != NULL){
    cols = tcrdbtblget(db, kxstr, ksiz);
    rb_ary_push(vary, maptovhashsym(cols));
    tcmapdel(cols);
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

void init_table(){
  rb_define_method(cTable, "mput", cTable_mput, 1);
  rb_define_method(cTable, "put", cTable_put, 2);
  rb_define_alias(cTable, "[]=", "put");
  rb_define_method(cTable, "putkeep", cTable_putkeep, 2);
  rb_define_method(cTable, "putcat", cTable_putcat, 2);
  rb_define_method(cTable, "out", cTable_out, 1);
  rb_define_method(cTable, "get", cTable_get, 1);
  rb_define_method(cTable, "mget", cTable_mget, -1);
  rb_define_alias(cTable, "[]", "get");
  rb_define_method(cTable, "setindex", cTable_setindex, 2);
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
}
