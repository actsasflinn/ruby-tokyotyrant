#include <ruby.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <tcrdb.h>

#define RDBVNDATA "@rdb"
#define RDBQRYVNDATA "@rdbquery"
#define NUMBUFSIZ 32
#define TTPUT      0
#define TTPUTKEEP  1
#define TTPUTCAT   2
#define TTPUTNR    3

#if !defined(RSTRING_PTR)
#define RSTRING_PTR(TC_s) (RSTRING(TC_s)->ptr)
#endif
#if !defined(RSTRING_LEN)
#define RSTRING_LEN(TC_s) (RSTRING(TC_s)->len)
#endif
#if !defined(RARRAY_LEN)
#define RARRAY_LEN(TC_a) (RARRAY(TC_a)->len)
#endif


static VALUE mTokyoTyrant;
static VALUE eTokyoTyrantError;
static VALUE cDB;
static VALUE cTable;
static VALUE cQuery;

uint64_t cDB_size(VALUE vself);
char cDB_stat(VALUE vself);
TCLIST cDB_misc(VALUE vself, VALUE name, int opts, const TCLIST *args);

/* private function prototypes */
static VALUE StringValueEx(VALUE vobj);
static TCLIST *varytolist(VALUE vary);
static VALUE listtovary(TCLIST *list);
static TCMAP *vhashtomap(VALUE vhash);
static VALUE maptovhash(TCMAP *map);

static VALUE StringValueEx(VALUE vobj){
  char kbuf[NUMBUFSIZ];
  int ksiz;
  switch(TYPE(vobj)){
  case T_FIXNUM:
    ksiz = sprintf(kbuf, "%d", (int)FIX2INT(vobj));
    return rb_str_new(kbuf, ksiz);
  case T_BIGNUM:
    ksiz = sprintf(kbuf, "%lld", (long long)NUM2LL(vobj));
    return rb_str_new(kbuf, ksiz);
  case T_TRUE:
    ksiz = sprintf(kbuf, "true");
    return rb_str_new(kbuf, ksiz);
  case T_FALSE:
    ksiz = sprintf(kbuf, "false");
    return rb_str_new(kbuf, ksiz);
  case T_NIL:
    ksiz = sprintf(kbuf, "nil");
    return rb_str_new(kbuf, ksiz);
  }
  return StringValue(vobj);
}

static TCLIST *varytolist(VALUE vary){
  VALUE vval;
  TCLIST *list;
  int i, num;
  num = RARRAY_LEN(vary);
  list = tclistnew2(num);
  for(i = 0; i < num; i++){
    vval = rb_ary_entry(vary, i);
    vval = StringValueEx(vval);
    tclistpush(list, RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return list;
}

static VALUE listtovary(TCLIST *list){
  VALUE vary;
  const char *vbuf;
  int i, num, vsiz;
  num = tclistnum(list);
  vary = rb_ary_new2(num);
  for(i = 0; i < num; i++){
    vbuf = tclistval(list, i, &vsiz);
    rb_ary_push(vary, rb_str_new(vbuf, vsiz));
  }
  return vary;
}

static TCMAP *vhashtomap(VALUE vhash){
  VALUE vkeys, vkey, vval;
  TCMAP *map;
  int i, num;
  map = tcmapnew2(31);
  vkeys = rb_funcall(vhash, rb_intern("keys"), 0);
  num = RARRAY_LEN(vkeys);
  for(i = 0; i < num; i++){
    vkey = rb_ary_entry(vkeys, i);
    vval = rb_hash_aref(vhash, vkey);
    vkey = StringValueEx(vkey);
    vval = StringValueEx(vval);
    tcmapput(map, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vval), RSTRING_LEN(vval));
  }
  return map;
}

static VALUE maptovhash(TCMAP *map){
  const char *kbuf, *vbuf;
  int ksiz, vsiz;
  VALUE vhash;
  vhash = rb_hash_new();
  tcmapiterinit(map);
  while((kbuf = tcmapiternext(map, &ksiz)) != NULL){
    vbuf = tcmapiterval(kbuf, &vsiz);
    rb_hash_aset(vhash, rb_str_new(kbuf, ksiz), rb_str_new(vbuf, vsiz));
  }
  return vhash;
}

static void cDB_free(TCRDB *db){
  /* delete the object */
  tcrdbdel(db);
}

static VALUE cDB_close(VALUE vself){
  int ecode;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  /* close the connection */
  if(!tcrdbclose(db)){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "close error: %s\n", tcrdberrmsg(ecode));
  }

  return Qtrue;
}

static VALUE cDB_initialize(int argc, VALUE *argv, VALUE vself){
  VALUE host, port;
  int ecode;
  TCRDB *db;

  rb_scan_args(argc, argv, "11", &host, &port);
  if(NIL_P(host)) host = rb_str_new2("127.0.0.1");
  if(NIL_P(port)) port = INT2FIX(1978);

  db = tcrdbnew();

  if(!tcrdbopen(db, StringValuePtr(host), FIX2INT(port))){
    ecode = tcrdbecode(db);
    rb_raise(eTokyoTyrantError, "open error: %s\n", tcrdberrmsg(ecode));
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
    rb_raise(eTokyoTyrantError, "put error: %s\n", tcrdberrmsg(ecode));
  }

  return Qtrue;  
}

static VALUE cDB_put(VALUE vself, VALUE vkey, VALUE vstr){
  return cDB_put_method(vself, vkey, vstr, TTPUT);
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
    rb_raise(eTokyoTyrantError, "put error: %s\n", tcrdberrmsg(ecode));
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

static VALUE cDB_vsiz(VALUE vself, VALUE vkey){
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  vkey = StringValueEx(vkey);
  return INT2NUM(tcrdbvsiz2(db, RSTRING_PTR(vkey)));
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

static VALUE cDB_ext(int argc, VALUE *argv, VALUE vself){
  const char *res;
  VALUE vname, vkey, vvalue, vopts;
  TCRDB *db;
  Data_Get_Struct(rb_iv_get(vself, RDBVNDATA), TCRDB, db);

  rb_scan_args(argc, argv, "11", &vname, &vkey, &vvalue, &vopts);

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

/* TABLE METHODS */

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

/* Query Methods */

static int cQuery_procrec(const void *pkbuf, int pksiz, TCMAP *cols, void *opq){
  VALUE vpkey, vcols, vrv, vkeys, vkey, vval;
  int i, rv, num;
  vpkey = rb_str_new(pkbuf, pksiz);
  vcols = maptovhash(cols);
  vrv = rb_yield_values(2, vpkey, vcols);
  rv = (vrv == Qnil) ? 0 : NUM2INT(vrv);
  if(rv & TDBQPPUT){
    tcmapclear(cols);
    vkeys = rb_funcall(vcols, rb_intern("keys"), 0);
    num = RARRAY_LEN(vkeys);
    for(i = 0; i < num; i++){
      vkey = rb_ary_entry(vkeys, i);
      vval = rb_hash_aref(vcols, vkey);
      vkey = StringValueEx(vkey);
      vval = StringValueEx(vval);
      tcmapput(cols, RSTRING_PTR(vkey), RSTRING_LEN(vkey), RSTRING_PTR(vval), RSTRING_LEN(vval));
    }
  }
  return rv;
}

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

/*
static VALUE cQuery_proc(VALUE vself, VALUE vproc){
  VALUE vqry;
  RDBQRY *qry;
  if(rb_block_given_p() != Qtrue) rb_raise(rb_eArgError, "no block given");
  vqry = rb_iv_get(vself, RDBQRYVNDATA);
  Data_Get_Struct(vqry, RDBQRY, qry);
  return tcrdbqryproc(qry, (TDBQRYPROC)cQuery_procrec, NULL) ? Qtrue : Qfalse;
}
*/

static VALUE cQuery_hint(VALUE vself){
  VALUE vqry;
  RDBQRY *qry;
  vqry = rb_iv_get(vself, RDBQRYVNDATA);
  Data_Get_Struct(vqry, RDBQRY, qry);
  return rb_str_new2(tcrdbqryhint(qry));
}


void Init_tokyo_tyrant(){
  mTokyoTyrant = rb_define_module("TokyoTyrant");

  eTokyoTyrantError = rb_define_class("TokyoTyrantError", rb_eStandardError);
  cDB = rb_define_class_under(mTokyoTyrant, "DB", rb_cObject);
  cTable = rb_define_class_under(mTokyoTyrant, "Table", cDB);
  cQuery = rb_define_class_under(mTokyoTyrant, "Query", rb_cObject);

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
  rb_define_method(cDB, "put", cDB_put, 2);
  rb_define_alias(cDB, "[]=", "put");
  rb_define_method(cDB, "putkeep", cDB_putkeep, 2);
  rb_define_method(cDB, "putcat", cDB_putcat, 2);
  rb_define_method(cDB, "putshl", cDB_putshl, 2);
  rb_define_method(cDB, "putnr", cDB_putnr, 2);
  rb_define_method(cDB, "out", cDB_out, 1);
  rb_define_method(cDB, "get", cDB_get, 1);
  rb_define_alias(cDB, "[]", "get");
  rb_define_method(cDB, "vsiz", cDB_vsiz, 2);
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
  rb_define_method(cDB, "empty?", cDB_empty, 0);

  /* rubyisms
  rb_define_method(cDB, "each", cDB_each, 0);
  rb_define_method(cDB, "each_key", cDB_each_key, 0);
  rb_define_method(cDB, "each_value", cDB_each_value, 0);
  rb_define_method(cDB, "keys", cDB_keys, 0);
  rb_define_method(cDB, "values", cDB_values, 0);
  */

  rb_define_method(cTable, "put", cTable_put, 2);
  rb_define_alias(cTable, "[]=", "put");
  rb_define_method(cTable, "putkeep", cTable_putkeep, 2);
  rb_define_method(cTable, "putcat", cTable_putcat, 2);
  rb_define_method(cTable, "out", cTable_out, 1);
  rb_define_method(cTable, "get", cTable_get, 1);
  rb_define_alias(cTable, "[]", "get");
  rb_define_method(cTable, "setindex", cTable_setindex, 2);
  rb_define_method(cTable, "genuid", cTable_genuid, 0);


  rb_define_private_method(cQuery, "initialize", cQuery_initialize, 1);

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
/*
  rb_define_const(cQuery, "PPUT", INT2NUM(RDBQPPUT));
  rb_define_const(cQuery, "POUT", INT2NUM(RDBQPOUT));
  rb_define_const(cQuery, "PSTOP", INT2NUM(RDBQPSTOP));
*/

  rb_define_method(cQuery, "addcond", cQuery_addcond, 3);
  rb_define_method(cQuery, "setorder", cQuery_setorder, 2);
  rb_define_method(cQuery, "setmax", cQuery_setmax, 1);
  rb_define_method(cQuery, "search", cQuery_search, 0);
  rb_define_method(cQuery, "searchout", cQuery_searchout, 0);
//  rb_define_method(cQuery, "proc", cQuery_proc, 0);
  rb_define_method(cQuery, "hint", cQuery_hint, 0);
}
