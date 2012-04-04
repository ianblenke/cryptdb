extern "C" {

#include "postgres.h"
#include "fmgr.h"

// create function test_sum_add(bigint, bigint) returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function test_sum_final(bigint) returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate test_sum ( sfunc=test_sum_add, basetype=bigint, stype=bigint, finalfunc=test_sum_final, initcond='0' );

PG_MODULE_MAGIC;

Datum test_sum_add(PG_FUNCTION_ARGS);
Datum test_sum_final(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(test_sum_add);
PG_FUNCTION_INFO_V1(test_sum_final);

Datum test_sum_add(PG_FUNCTION_ARGS) {
  int64_t* a0 = (int64_t *) PG_GETARG_INT64(0);
  int64_t a1 = PG_GETARG_INT64(1);
  if (!a0) {
    a0 = new int64_t(0);
  }
  *a0 += a1;
  PG_RETURN_INT64((int64_t)a0);
}

Datum test_sum_final(PG_FUNCTION_ARGS) {
  int64_t* a0 = (int64_t *) PG_GETARG_INT64(0);
  if (!a0) PG_RETURN_INT64(0);
  int64_t r = *a0;
  delete a0;
  PG_RETURN_INT64(r);
}

}
