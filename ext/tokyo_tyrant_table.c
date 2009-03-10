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

  return tcrdbtblout(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey));
}

static VALUE cTable_get(VALUE vself, VALUE vkey){
  VALUE vcols;
  TCRDB *db;
  TCMAP *cols;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);
  vkey = StringValueEx(vkey);

  if(!(cols = tcrdbtblget(db, RSTRING_PTR(vkey), RSTRING_LEN(vkey)))) return Qnil;
  vcols = maptovhash(cols);
  tcmapdel(cols);
  return vcols;
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

void init_table(){
  rb_define_method(cTable, "put", cTable_put, 2);
  rb_define_alias(cTable, "[]=", "put");
  rb_define_method(cTable, "putkeep", cTable_putkeep, 2);
  rb_define_method(cTable, "putcat", cTable_putcat, 2);
  rb_define_method(cTable, "out", cTable_out, 1);
  rb_define_method(cTable, "get", cTable_get, 1);
  rb_define_alias(cTable, "[]", "get");
  rb_define_method(cTable, "setindex", cTable_setindex, 2);
  rb_define_method(cTable, "genuid", cTable_genuid, 0);
}
