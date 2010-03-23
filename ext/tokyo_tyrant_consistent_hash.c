#include <tokyo_tyrant_consistent_hash.h>

static void cConstistentHash_free(TCCHIDX *idx){
  tcchidxdel(idx);
}

static VALUE cConstistentHash_initialize(VALUE self, VALUE dbs){
  TCCHIDX *idx;

  Check_Type(dbs, T_ARRAY);
  idx = tcchidxnew(RARRAY_LEN(dbs));

  rb_iv_set(self, "@dbs", dbs);
  rb_iv_set(self, "@idx", Data_Wrap_Struct(rb_cObject, 0, cConstistentHash_free, idx));

  return Qtrue;
}

static VALUE cConstistentHash_db_for_key(VALUE self, VALUE key){
  VALUE dbs;
  TCCHIDX *idx;
  int hash;

  Data_Get_Struct(rb_iv_get(self, "@idx"), TCCHIDX, idx);
  hash = tcchidxhash(idx, RSTRING_PTR(key), RSTRING_LEN(key));
  dbs = rb_iv_get(self, "@dbs");

  return rb_ary_entry(dbs, hash);
}

void init_consistent_hash(){
  rb_define_private_method(cConstistentHash, "initialize", cConstistentHash_initialize, 1);
  rb_define_method(cConstistentHash, "db_for_key", cConstistentHash_db_for_key, 1);
}
