#include <tokyo_tyrant_query.h>

static VALUE cQuery_initialize(VALUE vself, VALUE vrdb){
  VALUE vqry;
  TCRDB *db;
  RDBQRY *qry;
  Check_Type(vrdb, T_OBJECT);
  vrdb = rb_iv_get(vrdb, RDBVNDATA);
  Data_Get_Struct(vrdb, TCRDB, db);
  qry = tcrdbqrynew(db);
  vqry = Data_Wrap_Struct(rb_cObject, 0, tcrdbqrydel, qry);
  rb_iv_set(vself, RDBQRYVNDATA, vqry);
  rb_iv_set(vself, RDBVNDATA, vrdb);
  return Qnil;
}

static VALUE cQuery_addcond(VALUE vself, VALUE vname, VALUE vop, VALUE vexpr){
  VALUE vqry;
  RDBQRY *qry;
  vname = StringValueEx(vname);
  vexpr = StringValueEx(vexpr);
  vqry = rb_iv_get(vself, RDBQRYVNDATA);
  Data_Get_Struct(vqry, RDBQRY, qry);
  tcrdbqryaddcond(qry, RSTRING_PTR(vname), NUM2INT(vop), RSTRING_PTR(vexpr));
  return Qnil;
}

static VALUE cQuery_setorder(VALUE vself, VALUE vname, VALUE vtype){
  VALUE vqry;
  RDBQRY *qry;
  vname = StringValueEx(vname);
  vqry = rb_iv_get(vself, RDBQRYVNDATA);
  Data_Get_Struct(vqry, RDBQRY, qry);
  tcrdbqrysetorder(qry, RSTRING_PTR(vname), NUM2INT(vtype));
  return Qnil;
}

static VALUE cQuery_setmax(VALUE vself, VALUE vmax){
  VALUE vqry;
  RDBQRY *qry;
  vqry = rb_iv_get(vself, RDBQRYVNDATA);
  Data_Get_Struct(vqry, RDBQRY, qry);
  tcrdbqrysetmax(qry, NUM2INT(vmax));
  return Qnil;
}

static VALUE cQuery_search(VALUE vself){
  VALUE vqry, vary;
  RDBQRY *qry;
  TCLIST *res;
  vqry = rb_iv_get(vself, RDBQRYVNDATA);
  Data_Get_Struct(vqry, RDBQRY, qry);
  res = tcrdbqrysearch(qry);
  vary = listtovary(res);
  tclistdel(res);
  return vary;
}

static VALUE cQuery_searchout(VALUE vself){
  VALUE vqry;
  RDBQRY *qry;
  vqry = rb_iv_get(vself, RDBQRYVNDATA);
  Data_Get_Struct(vqry, RDBQRY, qry);
  return tcrdbqrysearchout(qry) ? Qtrue : Qfalse;
}

void init_query(){
  rb_define_const(cQuery, "CSTREQ", INT2NUM(RDBQCSTREQ));
  rb_define_const(cQuery, "CSTRINC", INT2NUM(RDBQCSTRINC));
  rb_define_const(cQuery, "CSTRBW", INT2NUM(RDBQCSTRBW));
  rb_define_const(cQuery, "CSTREW", INT2NUM(RDBQCSTREW));
  rb_define_const(cQuery, "CSTRAND", INT2NUM(RDBQCSTRAND));
  rb_define_const(cQuery, "CSTROR", INT2NUM(RDBQCSTROR));
  rb_define_const(cQuery, "CSTROREQ", INT2NUM(RDBQCSTROREQ));
  rb_define_const(cQuery, "CSTRRX", INT2NUM(RDBQCSTRRX));
  rb_define_const(cQuery, "CNUMEQ", INT2NUM(RDBQCNUMEQ));
  rb_define_const(cQuery, "CNUMGT", INT2NUM(RDBQCNUMGT));
  rb_define_const(cQuery, "CNUMGE", INT2NUM(RDBQCNUMGE));
  rb_define_const(cQuery, "CNUMLT", INT2NUM(RDBQCNUMLT));
  rb_define_const(cQuery, "CNUMLE", INT2NUM(RDBQCNUMLE));
  rb_define_const(cQuery, "CNUMBT", INT2NUM(RDBQCNUMBT));
  rb_define_const(cQuery, "CNUMOREQ", INT2NUM(RDBQCNUMOREQ));
  rb_define_const(cQuery, "CNEGATE", INT2NUM(RDBQCNEGATE));
  rb_define_const(cQuery, "CNOIDX", INT2NUM(RDBQCNOIDX));
  rb_define_const(cQuery, "OSTRASC", INT2NUM(RDBQOSTRASC));
  rb_define_const(cQuery, "OSTRDESC", INT2NUM(RDBQOSTRDESC));
  rb_define_const(cQuery, "ONUMASC", INT2NUM(RDBQONUMASC));
  rb_define_const(cQuery, "ONUMDESC", INT2NUM(RDBQONUMDESC));

  rb_define_private_method(cQuery, "initialize", cQuery_initialize, 1);
  rb_define_method(cQuery, "addcond", cQuery_addcond, 3);
  rb_define_alias(cQuery, "condition", "addcond");
  rb_define_method(cQuery, "setorder", cQuery_setorder, 2);
  rb_define_alias(cQuery, "order_by", "setorder");
  rb_define_method(cQuery, "setmax", cQuery_setmax, 1);
  rb_define_alias(cQuery, "limit", "setmax");
  rb_define_method(cQuery, "search", cQuery_search, 0);
  rb_define_method(cQuery, "searchout", cQuery_searchout, 0);
}
