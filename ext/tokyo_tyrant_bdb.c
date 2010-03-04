#include <tokyo_tyrant_bdb.h>

static VALUE cBDB_putlist(VALUE vself, VALUE vhash){
  VALUE vary;
  TCLIST *list, *result;
  TCRDB *db = mTokyoTyrant_getdb(vself);
  Check_Type(vhash, T_HASH);

  list = vhashtoputlist(vhash);
  if ((result = tcrdbmisc(db, "putlist", 0, list)) != NULL){
    vary = listtovary(result);
    tclistdel(result);
  } else {
    vary = rb_ary_new();
  }
  tclistdel(list);

  return vary;
}

static VALUE cDB_getlist(int argc, VALUE *argv, VALUE vself){
  VALUE vkeys, vvalue, vary, vhash, vkey, vval, vvals;
  TCLIST *list, *result;
  int i;
  TCRDB *db = mTokyoTyrant_getdb(vself);
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
  result = tcrdbmisc(db, "getlist", RDBMONOULOG, list);
  tclistdel(list);
  vary = listtovary(result);
  tclistdel(result);

  vhash = rb_hash_new();
  for(i = 0; i < RARRAY_LEN(vary); i += 2){
    vkey = rb_ary_entry(vary, i);
    vval = rb_ary_entry(vary, i + 1);
    vvals = rb_hash_aref(vhash, vkey);
    if (TYPE(vvals) == T_ARRAY){
      vvals = rb_ary_push(vvals, vval);
    } else {
      vvals = rb_ary_new();
      vvals = rb_ary_push(vvals, vval);
    }
    rb_hash_aset(vhash, vkey, vvals);
  }
  return vhash;
}

void init_bdb(){
  rb_define_method(cBDB, "putlist", cBDB_putlist, 1);
  rb_define_method(cDB, "getlist", cDB_getlist, -1);
}
