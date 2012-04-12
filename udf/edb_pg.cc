#include <udf/edb_common.hh>

extern "C" {
#include "postgres.h"
#include "fmgr.h"
}

static unsigned char *
getba(PG_FUNCTION_ARGS, int i, unsigned int & len)
{
    bytea * eValue = PG_GETARG_BYTEA_P(i);

    len = VARSIZE(eValue) - VARHDRSZ;
    unsigned char * eValueBytes = new unsigned char[len];
    memcpy(eValueBytes, VARDATA(eValue), len);
    return eValueBytes;
}

static inline unsigned char *
getba_rdonly(PG_FUNCTION_ARGS, int i, unsigned int & len)
{
    bytea * eValue = PG_GETARG_BYTEA_P(i);
    len = VARSIZE(eValue) - VARHDRSZ;
    return (unsigned char *) VARDATA(eValue);
}

#define PG_PASS_FARGS fcinfo

extern "C" {

// create function test_sum_add(bigint, bigint) returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function test_sum_final(bigint) returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate test_sum ( sfunc=test_sum_add, basetype=bigint, stype=bigint, finalfunc=test_sum_final, initcond='0' );

PG_MODULE_MAGIC;

// create function bytea_cmp_less(bytea, bytea) returns bytea language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so' strict;
// create aggregate min ( sfunc=bytea_cmp_less, basetype=bytea, stype=bytea );
Datum bytea_cmp_less(PG_FUNCTION_ARGS);

Datum searchswp(PG_FUNCTION_ARGS);

Datum test_sum_add(PG_FUNCTION_ARGS);
Datum test_sum_final(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(bytea_cmp_less);
PG_FUNCTION_INFO_V1(searchswp);

PG_FUNCTION_INFO_V1(test_sum_add);
PG_FUNCTION_INFO_V1(test_sum_final);

Datum bytea_cmp_less(PG_FUNCTION_ARGS) {
  unsigned int l1, l2;
  unsigned char* b1 = getba_rdonly(PG_PASS_FARGS, 0, l1);
  unsigned char* b2 = getba_rdonly(PG_PASS_FARGS, 1, l2);

  int res = memcmp(b1, b2, l1 < l2 ? l1 : l2);
  unsigned char *p = (res < 0) ? b1 : b2;
  unsigned int l = (res < 0) ? l1 : l2;

  unsigned char* ret = (unsigned char *) palloc( l + VARHDRSZ );
  SET_VARSIZE(ret, l + VARHDRSZ);
  memcpy(VARDATA(ret), p, l);
  PG_RETURN_BYTEA_P(ret);
}

// create function searchSWP(bytea, bytea, bytea) returns bool language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
Datum searchswp(PG_FUNCTION_ARGS) {

  Token t;

  unsigned int ciphLen;
  unsigned char *ciph = getba(PG_PASS_FARGS, 0, ciphLen);

  unsigned int wordKeyLen;
  unsigned char *wordKey = getba(PG_PASS_FARGS, 1, wordKeyLen);

  t.ciph = Binary(ciphLen, ciph);
  t.wordKey = Binary(wordKeyLen, wordKey);

  unsigned int allciphLen;
  unsigned char * allciph = getba(PG_PASS_FARGS, 2, allciphLen);
  Binary overallciph(allciphLen, allciph);

  bool ret = search(t, overallciph);

  delete [] ciph;
  delete [] wordKey;
  delete [] allciph;

  PG_RETURN_BOOL(ret);
}

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
