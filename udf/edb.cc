/*
 * There are three or four incompatible memory allocators:
 *
 *   palloc / pfree (Postgres-specific)
 *   malloc / free
 *   new / delete
 *   new[] / delete[]
 *
 * The Postgres versions of the UDFs likely do not keep track of which
 * allocator is used in each case.  They might not free memory at all
 * in some cases, and might free memory with a different allocator than
 * the one used to initially allocate it.  Beware.
 *
 * The MySQL versions of the UDFs are more likely to get this right.
 */

#define DEBUG 1

#include <udf/edb_common.hh>

using namespace std;
using namespace NTL;
using namespace tbb;

// templated code must go here, before the extern "C"

static ofstream trace_file("/tmp/trace.txt");

template <typename T>
struct mysql_item_result_type {};

template <>
struct mysql_item_result_type<uint64_t> {
  static const Item_result value = INT_RESULT;
  static inline uint64_t extract(UDF_ARGS* args, size_t pos) {
    return *((long long *) args->args[pos]);
  }
};

template <>
struct mysql_item_result_type<double> {
  static const Item_result value = REAL_RESULT;
  static inline double extract(UDF_ARGS* args, size_t pos) {
    return *((double *) args->args[pos]);
  }
};

struct AggRowColPack {
  //static const size_t AggSize = 1256 * 2 / 8;
  //static const size_t RowsPerBlock = 3;

  //static const size_t BufferSize = 80000000;
  static const size_t BufferSize = 1024;

  static const size_t NumBlocksInternalBuffer = 524288;
  //static const size_t NumBlocksInternalBuffer = 4194304;
};

struct agg_group_key {
  struct elem {
    Item_result type;
    int64_t i;
    double d;
    string s;
  };

  vector<elem> elems;

  static void coerce_types(
      UDF_ARGS* args,
      size_t off, size_t len) {
    // coerce decimal keys into real
    assert(args->arg_count >= (off + len));
    for (size_t i = off; i < off + len; i++) {
      if (args->arg_type[i] == DECIMAL_RESULT) {
        args->arg_type[i] = REAL_RESULT;
      }
    }
  }

  agg_group_key() { assert(elems.size() == 0); }

  agg_group_key(UDF_ARGS* args, size_t off, size_t len) {
    elems.resize(len);
    assert(args->arg_count >= (off + len));
    assert(elems.size() == len);
    for (size_t i = 0, j = off; j < off + len; i++, j++) {
      switch (args->arg_type[j]) {
      case INT_RESULT:
        elems[i].i = *((long long *)args->args[j]);
        break;
      case REAL_RESULT:
        elems[i].d = *((double *)args->args[j]);
        break;
      case STRING_RESULT:
        elems[i].s = string(args->args[j], (size_t) args->lengths[j]);
        break;
      default: assert(false);
      }
      elems[i].type = args->arg_type[j];
    }
  }

  // key is framed as:
  // [type (1-byte) | (int64_t (8 bytes) | double (8 bytes) | [len + len bytes])]+
  uint8_t* write_key(uint8_t* buf) const {
    for (size_t i = 0; i < elems.size(); i++) {
      const elem& e = elems[i];
      *buf++ = (uint8_t) e.type;
      switch (e.type) {
      case INT_RESULT: {
        int64_t* p = (int64_t *)buf; *p = e.i;
        buf += sizeof(int64_t);
        break;
      }
      case REAL_RESULT: {
        double* p = (double *)buf; *p = e.d;
        buf += sizeof(double);
        break;
      }
      case STRING_RESULT: {
        uint32_t* p = (uint32_t *)buf; *p = e.s.size();
        buf += sizeof(uint32_t);
        memcpy(buf, e.s.data(), (uint32_t)e.s.size());
        buf += (uint32_t)e.s.size();
        break;
      }
      default: assert(false);
      }
    }
    return buf;
  }

  size_t size_needed() const {
    size_t r = 0;
    for (size_t i = 0; i < elems.size(); i++) {
      const elem& e = elems[i];
      r += sizeof(uint8_t);
      switch (e.type) {
      case INT_RESULT: {
        r += sizeof(int64_t);
        break;
      }
      case REAL_RESULT: {
        r += sizeof(double);
        break;
      }
      case STRING_RESULT: {
        r += sizeof(uint32_t);
        r += (uint32_t)e.s.size();
        break;
      }
      default: assert(false);
      }
    }
    return r;
  }
};

namespace {
  ostream& operator<<(ostream& o, const agg_group_key& k) {
    o << "[";
    for (size_t i = 0; i < k.elems.size(); i++) {
      const agg_group_key::elem& e = k.elems[i];
      switch (e.type) {
      case INT_RESULT:    o << e.i << " (int)"; break;
      case REAL_RESULT:   o << e.d << " (dbl)"; break;
      case STRING_RESULT: o << marshallBinary(e.s) << " (str)"; break;
      default: assert(false);
      }
      if (i != k.elems.size() - 1) {
        o << ", ";
      }
    }
    o << "]";
    return o;
  }
}

static bool operator<(const agg_group_key& a, const agg_group_key& b) {
  assert(a.elems.size() == b.elems.size());
  for (size_t i = 0; i < a.elems.size(); i++) {
    const agg_group_key::elem& lhs = a.elems[i];
    const agg_group_key::elem& rhs = b.elems[i];
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
    case INT_RESULT:
      if (lhs.i < rhs.i) return true;
      if (lhs.i > rhs.i) return false;
      break;
    case REAL_RESULT:
      if (lhs.d < rhs.d) return true;
      if (lhs.d > rhs.d) return false;
      break;
    case STRING_RESULT:
      if (lhs.s < rhs.s) return true;
      if (lhs.s > rhs.s) return false;
      break;
    default: assert(false);
    }
  }
  return false;
}

template <typename T>
struct sum_hash_agg_impl {
  struct state {
      // key -> (count, sum)
      std::map<agg_group_key, std::pair<uint64_t, T> > aggs;
      void *rbuf;
  };

  static my_bool
  init(UDF_INIT *initid, UDF_ARGS *args, char *message)
  {
      state *as = new state;
      as->rbuf = NULL;
      initid->ptr = (char *) as;

      // input argument is:
      // first N-1 args => key, last arg => summation argument

      if (args->arg_count <= 1) {
        strcpy(message, "need > 1 arg");
        return 1;
      }

      agg_group_key::coerce_types(args, 0, args->arg_count - 1);

      args->arg_type[args->arg_count - 1] = mysql_item_result_type<T>::value;
      return 0;
  }

  static void
  deinit(UDF_INIT *initid)
  {
      state *as = (state *) initid->ptr;
      free(as->rbuf);
      delete as;
  }

  static void
  clear(UDF_INIT *initid, char *is_null, char *error)
  {
      state *as = (state *) initid->ptr;
      as->aggs.clear();
  }

  static void print_agg_map(state* as) {
    for (auto it = as->aggs.begin(); it != as->aggs.end(); ++it) {
      trace_file << it->first << " : " << it->second.first << endl;
    }
    trace_file.flush();
  }

  static my_bool
  add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
  {
      state *as = (state *) initid->ptr;

      agg_group_key key(args, 0, args->arg_count - 1);

      typename std::map<agg_group_key, std::pair<uint64_t, T> >::iterator it =
          as->aggs.find(key);

      T val = mysql_item_result_type<T>::extract(args, args->arg_count - 1);

      if (UNLIKELY(it == as->aggs.end())) {
          std::pair<uint64_t, T>& p = as->aggs[key]; // creates on demand
          assert(as->aggs.find(key) != as->aggs.end());
          p.first  = 1;
          p.second = val;
      } else {
          it->second.first++;
          it->second.second += val;
      }
      return true;
  }

  static char *
  agg(UDF_INIT *initid, UDF_ARGS *args, char *result,
      unsigned long *length, char *is_null, char *error)
  {
      state *as = (state *) initid->ptr;
      *length = (sizeof(uint64_t) + sizeof(T)) * as->aggs.size();

      typedef
        typename std::map<agg_group_key, std::pair<uint64_t, T> >::iterator
        iter;
      for (iter it = as->aggs.begin(); it != as->aggs.end(); ++it) {
        *length += it->first.size_needed();
      }

      as->rbuf = malloc(*length);
      assert(as->rbuf); // TODO: handle OOM
      uint8_t* ptr = (uint8_t *) as->rbuf;

      for (iter it = as->aggs.begin(); it != as->aggs.end(); ++it) {
          ptr = it->first.write_key(ptr);
          uint64_t *iptr = (uint64_t *) ptr;
          *iptr = it->second.first;
          ptr += sizeof(uint64_t);
          T *dptr = (T *) ptr;
          *dptr = it->second.second;
          ptr += sizeof(T);
      }
      return (char *) as->rbuf;
  }
};

extern "C" {
#if MYSQL_S

typedef unsigned long long ulonglong;
typedef long long longlong;
#include <mysql.h>
#include <ctype.h>

my_bool  decrypt_int_sem_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
ulonglong decrypt_int_sem(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
                         char *error);

my_bool  decrypt_int_det_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
ulonglong decrypt_int_det(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
                         char *error);

my_bool  decrypt_text_sem_init(UDF_INIT *initid, UDF_ARGS *args,
                               char *message);
void     decrypt_text_sem_deinit(UDF_INIT *initid);
char *   decrypt_text_sem(UDF_INIT *initid, UDF_ARGS *args, char *result,
                          unsigned long *length, char *is_null, char *error);

my_bool  encrypt_int_det_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
ulonglong encrypt_int_det(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
                         char *error);

my_bool  decrypt_text_det_init(UDF_INIT *initid, UDF_ARGS *args,
                               char *message);
void     decrypt_text_det_deinit(UDF_INIT *initid);
char *   decrypt_text_det(UDF_INIT *initid, UDF_ARGS *args, char *result,
                          unsigned long *length, char *is_null, char *error);

my_bool  search_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
ulonglong search(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

my_bool  agg_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg_deinit(UDF_INIT *initid);
void     agg_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

my_bool  sum_hash_agg_int_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     sum_hash_agg_int_deinit(UDF_INIT *initid);
void     sum_hash_agg_int_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  sum_hash_agg_int_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   sum_hash_agg_int(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

my_bool  sum_hash_agg_double_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     sum_hash_agg_double_deinit(UDF_INIT *initid);
void     sum_hash_agg_double_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  sum_hash_agg_double_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   sum_hash_agg_double(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

// TODO: fixup to be more general
my_bool  agg_char2_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg_char2_deinit(UDF_INIT *initid);
void     agg_char2_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg_char2_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg_char2(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

my_bool  agg_hash_agg_row_col_pack_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg_hash_agg_row_col_pack_deinit(UDF_INIT *initid);
void     agg_hash_agg_row_col_pack_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg_hash_agg_row_col_pack_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg_hash_agg_row_col_pack(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

void     func_add_set_deinit(UDF_INIT *initid);
char *   func_add_set(UDF_INIT *initid, UDF_ARGS *args, char *result,
                      unsigned long *length, char *is_null, char *error);

my_bool  searchSWP_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     searchSWP_deinit(UDF_INIT *initid);
ulonglong searchSWP(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
                   char *error);

#else /* Postgres */

#include "postgres.h"                   /* general Postgres declarations */
#include "utils/array.h"
#include "executor/executor.h"  /* for GetAttributeByName() */

PG_MODULE_MAGIC;

Datum decrypt_int_sem(PG_FUNCTION_ARGS);
Datum decrypt_int_det(PG_FUNCTION_ARGS);
Datum decrypt_text_sem(PG_FUNCTION_ARGS);
Datum decrypt_text_det(PG_FUNCTION_ARGS);
Datum search(PG_FUNCTION_ARGS);
Datum func_add(PG_FUNCTION_ARGS);
Datum func_add_final(PG_FUNCTION_ARGS);
Datum func_add_set(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(decrypt_int_sem);
PG_FUNCTION_INFO_V1(decrypt_int_det);
PG_FUNCTION_INFO_V1(decrypt_text_sem);
PG_FUNCTION_INFO_V1(decrypt_text_det);
PG_FUNCTION_INFO_V1(search);
PG_FUNCTION_INFO_V1(func_add);
PG_FUNCTION_INFO_V1(func_add_final);
PG_FUNCTION_INFO_V1(func_add_set);

#endif
}

#if MYSQL_S
#define ARGS args

static uint64_t
getui(UDF_ARGS * args, int i)
{
    return (uint64_t) (*((longlong *) args->args[i]));
}

static unsigned char
getb(UDF_ARGS * args, int i)
{
    return (unsigned char)(*((longlong *) args->args[i]));
}

static unsigned char *
getba(UDF_ARGS * args, int i, uint64_t &len)
{
    len = args->lengths[i];
    return (unsigned char*) (args->args[i]);
}

#else

#define ARGS PG_FUNCTION_ARGS

static uint64_t
getui(ARGS, int i)
{
    return PG_GETARG_INT64(i);
}

static unsigned char
getb(ARGS, int i)
{
    return (unsigned char)PG_GETARG_INT32(i);
}

static unsigned char *
getba(ARGS, int i, unsigned int & len)
{
    bytea * eValue = PG_GETARG_BYTEA_P(i);

    len = VARSIZE(eValue) - VARHDRSZ;
    unsigned char * eValueBytes = new unsigned char[len];
    memcpy(eValueBytes, VARDATA(eValue), len);
    return eValueBytes;
}

#endif

extern "C" {

#if MYSQL_S
my_bool
decrypt_int_sem_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return 0;
}

ulonglong
decrypt_int_sem(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
#else /*postgres*/
Datum
decrypt_int_sem(PG_FUNCTION_ARGS)
#endif
{
    uint64_t eValue = getui(ARGS, 0);

    string key;
    key.resize(AES_KEY_BYTES);
    int offset = 1;

    for (unsigned int i = 0; i < AES_KEY_BYTES; i++)
        key[i] = getb(ARGS, offset+i);

    uint64_t salt = getui(args, offset + AES_KEY_BYTES);

    AES_KEY *aesKey = get_key_SEM(key);
    uint64_t value = decrypt_SEM(eValue, aesKey, salt);
    delete aesKey;

#if MYSQL_S
    return (ulonglong) value;
#else /* postgres */
    PG_RETURN_INT64(value);
#endif
}

#if MYSQL_S
my_bool
decrypt_int_det_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return 0;
}

ulonglong
decrypt_int_det(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
#else /* postgres */
Datum
decrypt_int_det(PG_FUNCTION_ARGS)
#endif
{
    uint64_t eValue = getui(ARGS, 0);

    string key;
    key.resize(AES_KEY_BYTES);
    int offset = 1;

    for (unsigned int i = 0; i < AES_KEY_BYTES; i++)
        key[i] = getb(ARGS, offset+i);

    blowfish bf(key);
    uint64_t value = bf.decrypt(eValue);

#if MYSQL_S
    return (ulonglong) value;
#else /* postgres */
    PG_RETURN_INT64(value);
#endif

}

#if MYSQL_S
my_bool
encrypt_int_det_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return 0;
}

ulonglong
encrypt_int_det(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
#else /* postgres */
Datum
decrypt_int_det(PG_FUNCTION_ARGS)
#endif
{
    uint64_t eValue = getui(ARGS, 0);

    string key;
    key.resize(AES_KEY_BYTES);
    int offset = 1;

    for (unsigned int i = 0; i < AES_KEY_BYTES; i++)
        key[i] = getb(ARGS, offset+i);

    blowfish bf(key);
    uint64_t value = bf.encrypt(eValue);

#if MYSQL_S
    return (ulonglong) value;
#else /* postgres */
    PG_RETURN_INT64(value);
#endif

}

#if MYSQL_S
my_bool
decrypt_text_sem_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return 0;
}

void
decrypt_text_sem_deinit(UDF_INIT *initid)
{
    /*
     * in mysql-server/sql/item_func.cc, udf_handler::fix_fields
     * initializes initid.ptr=0 for us.
     */
    if (initid->ptr)
        delete[] initid->ptr;
}

char *
decrypt_text_sem(UDF_INIT *initid, UDF_ARGS *args,
                 char *result, unsigned long *length,
                 char *is_null, char *error)
#else /* postgres */
Datum
decrypt_text_sem(PG_FUNCTION_ARGS)
#endif
{
    uint64_t eValueLen;
    unsigned char *eValueBytes = getba(args, 0, eValueLen);

    string key;
    key.resize(AES_KEY_BYTES);
    int offset = 1;

    for (unsigned int i = 0; i < AES_KEY_BYTES; i++) {
        key[i] = getb(ARGS,offset+i);
    }

     uint64_t salt = getui(ARGS, offset + AES_KEY_BYTES);

    AES_KEY *aesKey = get_AES_dec_key(key);
    string value = decrypt_SEM(eValueBytes, eValueLen, aesKey, salt);
    delete aesKey;

#if MYSQL_S
    unsigned char * res = new unsigned char[value.length()];
    initid->ptr = (char *) res;
    memcpy(res, value.data(), value.length());
    *length =  value.length();
    return (char*) initid->ptr;
#else
    bytea * res = (bytea *) palloc(eValueLen+VARHDRSZ);
    SET_VARSIZE(res, eValueLen+VARHDRSZ);
    memcpy(VARDATA(res), value, eValueLen);
    PG_RETURN_BYTEA_P(res);
#endif

}

#if MYSQL_S
my_bool
decrypt_text_det_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return 0;
}

void
decrypt_text_det_deinit(UDF_INIT *initid)
{
    /*
     * in mysql-server/sql/item_func.cc, udf_handler::fix_fields
     * initializes initid.ptr=0 for us.
     */
    if (initid->ptr)
        delete[] initid->ptr;
}

char *
decrypt_text_det(UDF_INIT *initid, UDF_ARGS *args,
                 char *result, unsigned long *length,
                 char *is_null, char *error)
#else /* postgres */
Datum
decrypt_text_det(PG_FUNCTION_ARGS)
#endif
{
    uint64_t eValueLen;
    unsigned char *eValueBytes = getba(args, 0, eValueLen);

    string key;
    key.resize(AES_KEY_BYTES);
    int offset = 1;

    for (unsigned int i = 0; i < AES_KEY_BYTES; i++) {
        key[i] = getb(ARGS,offset+i);
    }

    AES_KEY * aesKey = get_AES_dec_key(key);
    string value = decrypt_AES_CMC(string((char *)eValueBytes, (unsigned int)eValueLen), aesKey, false);
    delete aesKey;

#if MYSQL_S
    unsigned char * res = new unsigned char[value.length()];
    initid->ptr = (char *) res;
    memcpy(res, value.data(), value.length());
    *length =  value.length();
    return (char*) initid->ptr;
#else
    bytea * res = (bytea *) palloc(eValueLen+VARHDRSZ);
    SET_VARSIZE(res, eValueLen+VARHDRSZ);
    memcpy(VARDATA(res), value, eValueLen);
    PG_RETURN_BYTEA_P(res);
#endif

}

/*
 * given field of the form:   len1 word1 len2 word2 len3 word3 ...,
 * where each len is the length of the following "word",
 * search for word which is of the form len word_body where len is
 * the length of the word body
 */
#if MYSQL_S
my_bool
search_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{

    return 0;
}

ulonglong
search(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
#else
Datum
search(PG_FUNCTION_ARGS)
#endif
{
    uint64_t wordLen;
    char * word = (char *)getba(ARGS, 0, wordLen);
    if (wordLen != (unsigned int)word[0]) {
        cerr << "ERR: wordLen is not equal to fist byte of word!!! ";
    }
    word = word + 1;     // +1 skips over the length field
    //cerr << "given expr to search for has " << wordLen << " length \n";

    uint64_t fieldLen;
    char *field = (char *) getba(ARGS, 1, fieldLen);

    //cerr << "searching for "; myPrint((unsigned char *)word, wordLen); cerr
    // << " in field "; myPrint((unsigned char *)field, fieldLen); cerr <<
    // "\n";

    unsigned int i = 0;
    while (i < fieldLen) {
        unsigned int currLen = (unsigned int)field[i];
        if (currLen != wordLen) {
            i = i + currLen+1;
            continue;
        }

        //need to compare
        unsigned int j;
        for (j = 0; j < currLen; j++) {
            if (field[i+j+1] != word[j]) {
                break;
            }
        }
        if (j == currLen) {
#if MYSQL_S
            return 1;
#else
            PG_RETURN_BOOL(true);
#endif
        }
        i = i + currLen + 1;
    }

#if MYSQL_S
    return 0;
#else
    PG_RETURN_BOOL(true);
#endif
}

#if MYSQL_S

//TODO: write a version of search for postgres

my_bool
searchSWP_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{

    Token * t = new Token();

    uint64_t ciphLen;
    unsigned char *ciph = getba(args, 0, ciphLen);

    uint64_t wordKeyLen;
    unsigned char *wordKey = getba(args, 1, wordKeyLen);

    t->ciph = Binary((unsigned int) ciphLen, ciph);
    t->wordKey = Binary((unsigned int)wordKeyLen, wordKey);

    initid->ptr = (char *) t;

    return 0;
}

void
searchSWP_deinit(UDF_INIT *initid)
{
    Token *t = (Token *) initid->ptr;
    delete t;
}

ulonglong
searchSWP(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    uint64_t allciphLen;
    unsigned char * allciph = getba(ARGS, 2, allciphLen);
    Binary overallciph = Binary((unsigned int)allciphLen, allciph);

    Token * t = (Token *) initid->ptr;

    return search(*t, overallciph);
}

#endif

#if MYSQL_S

// agg()

struct agg_state {
    ZZ sum;
    ZZ n2;
    bool n2_set;
    void *rbuf;
};

my_bool
agg_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    TRACE();
    agg_state *as = new agg_state();
    as->rbuf = malloc(CryptoManager::Paillier_len_bytes);
    as->n2_set = 0;
    initid->ptr = (char *) as;
    return 0;
}

void
agg_deinit(UDF_INIT *initid)
{
    agg_state *as = (agg_state *) initid->ptr;
    free(as->rbuf);
    delete as;
}

void
agg_clear(UDF_INIT *initid, char *is_null, char *error)
{
    agg_state *as = (agg_state *) initid->ptr;
    as->sum = to_ZZ(1);
}

//args will be element to add, constant N2
my_bool
agg_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    // means we don't add
    if (args->args[0] == NULL) return true;

    agg_state *as = (agg_state *) initid->ptr;
    if (!as->n2_set) {
        ZZFromBytesFast(as->n2, (const uint8_t *) args->args[1],
                        args->lengths[1]);
        as->n2_set = 1;
    }

    ZZ e;
    ZZFromBytesFast(e, (const uint8_t *) args->args[0], args->lengths[0]);

    MulMod(as->sum, as->sum, e, as->n2);
    return true;
}

char *
agg(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    agg_state *as = (agg_state *) initid->ptr;
    BytesFromZZ((uint8_t *) as->rbuf, as->sum,
                CryptoManager::Paillier_len_bytes);
    *length = CryptoManager::Paillier_len_bytes;
    return (char *) as->rbuf;
}

my_bool
sum_hash_agg_int_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
  return sum_hash_agg_impl<uint64_t>::init(initid, args, message);
}

void
sum_hash_agg_int_deinit(UDF_INIT *initid)
{
  sum_hash_agg_impl<uint64_t>::deinit(initid);
}

void
sum_hash_agg_int_clear(UDF_INIT *initid, char *is_null, char *error)
{
  sum_hash_agg_impl<uint64_t>::clear(initid, is_null, error);
}

my_bool
sum_hash_agg_int_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
  return sum_hash_agg_impl<uint64_t>::add(initid, args, is_null, error);
}

char *
sum_hash_agg_int(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
  return sum_hash_agg_impl<uint64_t>::agg(initid, args, result, length, is_null, error);
}

my_bool
sum_hash_agg_double_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
  return sum_hash_agg_impl<double>::init(initid, args, message);
}

void
sum_hash_agg_double_deinit(UDF_INIT *initid)
{
  sum_hash_agg_impl<double>::deinit(initid);
}

void
sum_hash_agg_double_clear(UDF_INIT *initid, char *is_null, char *error)
{
  sum_hash_agg_impl<double>::clear(initid, is_null, error);
}

my_bool
sum_hash_agg_double_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
  return sum_hash_agg_impl<double>::add(initid, args, is_null, error);
}

char *
sum_hash_agg_double(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
  return sum_hash_agg_impl<double>::agg(initid, args, result, length, is_null, error);
}

struct agg_char2_state {
    struct worker_msg {
      bool shutdown;
      uint16_t key;
      mpz_t value;
    };

    struct worker_state {
      map<uint16_t, mpz_wrapper> aggs;
      concurrent_bounded_queue<worker_msg> q;
      agg_char2_state* as;
    };

    std::map<uint16_t, std::pair<uint64_t, mpz_wrapper> > aggs;
    mpz_t n2_mp;

    // parallel stuff
    size_t num_threads; /* 0 means don't run in || */

    std::vector< pthread_t > workers;
    std::vector< worker_state > worker_states;
    size_t workerCnt;

    void *rbuf;

    static void* worker_main(void* p) {
      worker_state* ctx = (worker_state *) p;
      worker_msg m;
      while (true) {
        ctx->q.pop(m);
        if (UNLIKELY(m.shutdown)) break;
        else {
          auto it = ctx->aggs.find(m.key);
          mpz_wrapper* v = NULL;
          if (UNLIKELY(it == ctx->aggs.end())) {
            v = &ctx->aggs[m.key];
            mpz_init_set_ui(v->mp, 1);
          } else {
            v = &it->second;
          }
          mpz_mul(v->mp, v->mp, m.value);
          mpz_mod(v->mp, v->mp, ctx->as->n2_mp);
          mpz_clear(m.value);
        }
      }
      return NULL;
    }
};

my_bool
agg_char2_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if (args->arg_count != 5) {
      strcpy(message, "need to provide 5 args");
      return 1;
    }

    agg_char2_state *as = new agg_char2_state;
    as->rbuf = NULL;

    if (args->args[3] == NULL) {
        strcpy(message, "need to provide PK");
        return 1;
    }

    if (args->args[4] == NULL) {
        strcpy(message, "need to provide parallelism flag");
        return 1;
    }

    mpz_init2(as->n2_mp, args->lengths[3] * 8);
    MPZFromBytes(as->n2_mp, (const uint8_t *) args->args[3],
                 args->lengths[3]);

    as->num_threads = *((long long *) args->args[4]);

    if (as->num_threads) {
      as->workers.resize(as->num_threads);
      as->worker_states.resize(as->num_threads);
      for (size_t i = 0; i < as->num_threads; i++) {
        as->worker_states[i].as  = as;
        pthread_create(&as->workers[i], NULL, agg_char2_state::worker_main, &as->worker_states[i]);
      }
      as->workerCnt = 0;
    }

    initid->ptr = (char *) as;
    return 0;
}

void
agg_char2_deinit(UDF_INIT *initid)
{
    agg_char2_state *as = (agg_char2_state *) initid->ptr;
    free(as->rbuf);
    delete as;
}

void
agg_char2_clear(UDF_INIT *initid, char *is_null, char *error)
{
  // TODO: free the aggs
    agg_char2_state *as = (agg_char2_state *) initid->ptr;
    as->aggs.clear();
}

my_bool
agg_char2_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    agg_char2_state *as = (agg_char2_state *) initid->ptr;

    long long p0, p1;
    p0 = *((long long *) args->args[0]);
    p1 = *((long long *) args->args[1]);

    //assert(p0 >= 0 && p0 <= 0xFF);
    //assert(p1 >= 0 && p1 <= 0xFF);

    uint16_t key = (uint8_t(p0) << 8) | uint8_t(p1);

    std::map<uint16_t, std::pair<uint64_t, mpz_wrapper> >::iterator it =
        as->aggs.find(key);
    mpz_wrapper* sum = NULL;
    if (UNLIKELY(it == as->aggs.end())) {
        std::pair<uint64_t, mpz_wrapper>& p = as->aggs[key]; // creates on demand
        p.first  = 1;
        mpz_init_set_ui(p.second.mp, 1);
        sum = &p.second;
    } else {
        it->second.first++;
        sum = &it->second.second;
    }

    agg_char2_state::worker_msg m;
    m.shutdown = false;
    mpz_init2(m.value, args->lengths[2] * 8);
    MPZFromBytes(m.value, (const uint8_t *) args->args[2],
                 args->lengths[2]);

    if (as->num_threads) {
      // queue job
      m.key = key;
      as->worker_states[ as->workerCnt++ % as->num_threads ].q.push(m);
    } else {
      // execute job
      mpz_mul(sum->mp, sum->mp, m.value);
      mpz_mod(sum->mp, sum->mp, as->n2_mp);
      mpz_clear(m.value);
    }

    return true;
}

char *
agg_char2(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    agg_char2_state *as = (agg_char2_state *) initid->ptr;

    if (as->num_threads) {
      agg_char2_state::worker_msg m;
      m.shutdown = true;

      for (size_t i = 0; i < as->num_threads; i++) {
        // signal all threads to stop
        as->worker_states[i].q.push(m);
      }

      for (size_t i = 0; i < as->num_threads; i++) {
        // wait until computation is done
        pthread_join(as->workers[i], NULL);
      }

      for (size_t i = 0; i < as->num_threads; i++) {
        // merge (aggregate) results from workers into main results
        agg_char2_state::worker_state& s = as->worker_states[i];
        for (auto it = s.aggs.begin(); it != s.aggs.end(); ++it) {
          mpz_wrapper& sum = as->aggs[it->first].second;
          mpz_mul(sum.mp, sum.mp, it->second.mp);
          mpz_mod(sum.mp, sum.mp, as->n2_mp);
          mpz_clear(it->second.mp);
        }
      }
    }

    *length = (2 + sizeof(uint64_t) + CryptoManager::Paillier_len_bytes) * as->aggs.size();
    as->rbuf = malloc(*length);
    assert(as->rbuf); // TODO: handle OOM
    uint8_t* ptr = (uint8_t *) as->rbuf;
    for (std::map<uint16_t, std::pair<uint64_t, mpz_wrapper> >::iterator
            it = as->aggs.begin();
         it != as->aggs.end(); ++it) {
        *ptr++ = (uint8_t) ((it->first >> 8) & 0xFF);
        *ptr++ = (uint8_t) (it->first & 0xFF);
        uint64_t *iptr = (uint64_t *) ptr;
        *iptr = it->second.first;
        ptr += sizeof(uint64_t);
        BytesFromMPZ(ptr, it->second.second.mp,
                     CryptoManager::Paillier_len_bytes);
        ptr += CryptoManager::Paillier_len_bytes;
    }
    return (char *) as->rbuf;
}

struct worker_msg {
  bool shutdown;
  struct ent {
    agg_group_key group_key;
    size_t group_id;
    size_t idx;
  };
  vector<ent> ents;
  mpz_t value; // worker must free
};

// forward decl
struct agg_hash_agg_row_pack_state;

struct worker_state {
  std::map<
    agg_group_key,
    std::vector< std::vector< mpz_wrapper > > > group_map;
  concurrent_bounded_queue<worker_msg> q;
  agg_hash_agg_row_pack_state* as;
};

struct agg_hash_agg_row_pack_state {
    struct per_group_state {
        std::vector< std::vector< mpz_wrapper > > running_sums_mp;
        uint64_t count;
    };

    size_t num_group_fields;

    typedef std::map<agg_group_key, per_group_state> group_map;
    group_map aggs;

    mpz_t n2_mp;

    void *rbuf;

    // agg info
    size_t agg_size; // size of each aggregate in bytes
    size_t rows_per_agg; // number of rows packed into each aggregrate

    size_t num_groups;

    // parallel data structures
    size_t num_threads; /* 0 means don't run in || */
    std::vector< pthread_t > workers;
    std::vector< worker_state > worker_states;
    size_t workerCnt;

    // internal element buffer
    typedef std::map<
      agg_group_key,
      std::vector< std::pair< uint64_t, uint64_t > > > element_buffer_map;
    element_buffer_map element_buffer;

    // internal state
    ssize_t block_id; // the block ID that is in block_buf. is also
                      // the last read block out of fp (so fp points to
                      // block_id + 1). if -1, then block_buf is not
                      // meaningful

    // last block buffer
    char* block_buf;
    size_t block_size;

    // internal buffer
    char* internal_buffer;
    ssize_t internal_block_id;

    // file pointer
    FILE* fp;

    // debug file
    std::ofstream debug_stream;

    // stats
    struct stats {
      stats() : seeks(0), mults(0), all_pos(0), masks_sum(0), masks_cnt(0) {}
      uint32_t seeks;
      uint32_t mults;
      uint32_t all_pos;

      uint32_t masks_sum;
      uint32_t masks_cnt;
    };

    stats st;
};

static void* worker_main(void* p) {
  worker_state* ctx = (worker_state *) p;
  worker_msg m;
  while (true) {
    ctx->q.pop(m);
    if (UNLIKELY(m.shutdown)) break;
    else {
      for (vector<worker_msg::ent>::iterator ent_it = m.ents.begin();
           ent_it != m.ents.end(); ++ent_it) {
        auto it = ctx->group_map.find(ent_it->group_key);
        vector< vector< mpz_wrapper > >* v = NULL;
        if (UNLIKELY(it == ctx->group_map.end())) {
          // init it
          v = &ctx->group_map[ent_it->group_key]; // create
          v->resize(ctx->as->num_groups);
          for (size_t i = 0; i < ctx->as->num_groups; i++) {
            InitMPZRunningSums(ctx->as->rows_per_agg, v->operator[](i));
          }
        } else {
          v = &it->second;
        }
        vector<mpz_wrapper>& vr = v->operator[](ent_it->group_id);
        mpz_mul(vr[ent_it->idx].mp, vr[ent_it->idx].mp, m.value);
        mpz_mod(vr[ent_it->idx].mp, vr[ent_it->idx].mp, ctx->as->n2_mp);
      }
      mpz_clear(m.value);
    }
  }
  return NULL;
}

#define EXTRACT_INT(pos) \
  *((long long *)args->args[(pos)])

#define EXTRACT_DBL(pos) \
  *((double *)args->args[(pos)])

#define EXTRACT_STR(pos) \
  string(args->args[(pos)], (size_t) args->lengths[(pos)])

my_bool
agg_hash_agg_row_col_pack_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{

    // input args:
    //
    // num_group_key_elems,
    // ...,
    // row_id,
    // public_key,
    // filename,
    // use_parallelism,
    // agg_size,
    // rows_per_agg,
    // group_0,
    // group_1,
    // ...,
    // group_N

#define BAIL(msg) \
  do { \
    strcpy(message, (msg)); \
    return 1; \
  } while (0)

#define EXPECT_CONST(pos, msg) \
  do { \
    if (args->arg_count <= (pos)) { \
      BAIL(msg); \
    } \
    if (args->args[(pos)] == NULL) { \
      BAIL(msg); \
    } \
  } while (0)

#define EXPECT_CONST_TYPE(pos, type, msg) \
  do { \
    if (args->arg_count <= (pos)) { \
      BAIL(msg); \
    } \
    if (args->args[(pos)] == NULL || \
        args->arg_type[(pos)] != (type)) { \
      BAIL(msg); \
    } \
  } while (0)

    EXPECT_CONST_TYPE(0, INT_RESULT, "num_group_fields");

    agg_hash_agg_row_pack_state *as = new agg_hash_agg_row_pack_state;
    as->num_group_fields = EXTRACT_INT(0);
    as->rbuf = NULL;

    agg_group_key::coerce_types(args, 1, as->num_group_fields);

    EXPECT_CONST(as->num_group_fields + 2 , "primary_key");
    mpz_init(as->n2_mp);
    MPZFromBytes(as->n2_mp,
                 (const uint8_t *) args->args[as->num_group_fields + 2],
                 args->lengths[as->num_group_fields + 2]);

    EXPECT_CONST_TYPE(as->num_group_fields + 3, STRING_RESULT, "filename");

    // TODO: fix security
    string name = EXTRACT_STR(as->num_group_fields + 3);
    as->fp = fopen(name.c_str(), "rb");
    assert(as->fp);

    EXPECT_CONST_TYPE(as->num_group_fields + 4, INT_RESULT, "parallelism_flag");
    assert(EXTRACT_INT(as->num_group_fields + 4) >= 0);
    as->num_threads = EXTRACT_INT(as->num_group_fields + 4);

    EXPECT_CONST_TYPE(as->num_group_fields + 5, INT_RESULT, "agg_size");
    as->agg_size = EXTRACT_INT(as->num_group_fields + 5);

    EXPECT_CONST_TYPE(as->num_group_fields + 6, INT_RESULT, "rows_per_agg");
    as->rows_per_agg = EXTRACT_INT(as->num_group_fields + 6);

    assert(as->agg_size > 0);
    assert(as->rows_per_agg > 0);

    as->num_groups =
      (ssize_t)args->arg_count -
      (ssize_t)(as->num_group_fields + 7);
    assert(as->num_groups > 0);

    if (as->num_threads) {
      // init the worker threads
      as->workers.resize(as->num_threads);
      as->worker_states.resize(as->num_threads);
      for (size_t i = 0; i < as->num_threads; i++) {
        as->worker_states[i].as  = as;
        pthread_create(&as->workers[i], NULL, worker_main, &as->worker_states[i]);
      }
      as->workerCnt = 0; // for RR-ing work to all workers
    }

    // load the first block
    as->internal_buffer =
      (char *) malloc( as->agg_size * AggRowColPack::NumBlocksInternalBuffer );
    assert(as->internal_buffer);
    as->internal_block_id = 0;
    size_t n ATTR_UNUSED =
      fread(as->internal_buffer, 1,
            as->agg_size * AggRowColPack::NumBlocksInternalBuffer,
            as->fp);

    as->block_id = 0;
    as->block_size = as->agg_size;
    as->block_buf = as->internal_buffer;

    as->debug_stream.open("/tmp/debug.txt");
    assert(as->debug_stream.good());

    as->debug_stream << "agg_size (bytes) = " << as->agg_size << endl;
    as->debug_stream << "rows_per_agg = " << as->rows_per_agg << endl;
    as->debug_stream << "num_groups = " << as->num_groups << endl;

    initid->ptr = (char *) as;
    return 0;
}

void
agg_hash_agg_row_col_pack_deinit(UDF_INIT *initid)
{
    agg_hash_agg_row_pack_state *as = (agg_hash_agg_row_pack_state *) initid->ptr;
    mpz_clear(as->n2_mp);
    free(as->rbuf);
    free(as->internal_buffer);
    fclose(as->fp);
    as->debug_stream.flush();
    delete as;
}

void
agg_hash_agg_row_col_pack_clear(UDF_INIT *initid, char *is_null, char *error)
{
    agg_hash_agg_row_pack_state *as = (agg_hash_agg_row_pack_state *) initid->ptr;
    as->aggs.clear();
    as->block_id = 0;
    as->block_buf = as->internal_buffer;
    as->internal_block_id = 0;
    fseek(as->fp, 0, SEEK_SET);
    // reload the first block
    size_t n ATTR_UNUSED =
      fread(as->internal_buffer, 1,
            as->agg_size * AggRowColPack::NumBlocksInternalBuffer,
            as->fp);
}

static void flush_group_buffers(agg_hash_agg_row_pack_state *as) {

    //as->debug_stream << "flush_group_buffers: starting" << endl;
    //as->debug_stream.flush();
    //timer t;

    typedef
      map
       <
        uint64_t /*block_id*/,
        vector< vector< uint32_t > > /* masks (per user group) */
       >
      control_group;

    // value is ordered list based on the pair's first element
    typedef vector
      <
        pair
        <
          agg_group_key /*group_id*/,
          control_group
        >
      > allocation_map;
    allocation_map alloc_map;

    alloc_map.resize(as->element_buffer.size());

    size_t eit_pos = 0;
    for (agg_hash_agg_row_pack_state::element_buffer_map::iterator
            eit = as->element_buffer.begin();
         eit != as->element_buffer.end(); ++eit, ++eit_pos) {
      // lookup agg group
      agg_hash_agg_row_pack_state::group_map::iterator it =
        as->aggs.find(eit->first);
      agg_hash_agg_row_pack_state::per_group_state* s = NULL;
      if (UNLIKELY(it == as->aggs.end())) {
          s = &as->aggs[eit->first]; // creates on demand
          s->running_sums_mp.resize(as->num_groups);
          for (size_t i = 0; i < as->num_groups; i++) {
            InitMPZRunningSums(as->rows_per_agg, s->running_sums_mp[i]);
          }
          s->count = 0;
      } else {
          s = &it->second;
      }
      s->count += eit->second.size();

      alloc_map[eit_pos].first = eit->first;
      control_group &m = alloc_map[eit_pos].second;

      //as->debug_stream
      //  << "going over " << eit->second.size() << " buffered entries for group: " << eit->first << endl;
      //timer t0;
      for (vector< pair< uint64_t, uint64_t > >::iterator it = eit->second.begin();
           it != eit->second.end(); ++it) {
        uint64_t row_id = it->first;
        uint64_t mask   = it->second;
        //assert(mask != 0); // otherwise would not be here...

        // compute block ID from row ID
        uint64_t block_id = row_id / as->rows_per_agg;
        // compute block offset
        uint64_t block_offset = row_id % as->rows_per_agg;

        vector< vector< uint32_t > >&v = m[block_id]; // create on demand
        if (v.empty()) v.resize(as->num_groups);

        for (size_t i = 0; i < as->num_groups; i++) {
          if (!(mask & (0x1 << i))) continue;
          vector<uint32_t>& masks = v[i];
          if (masks.empty()) {
            masks.push_back(0x1 << block_offset);
          } else {
            // search for empty spot w/ linear scan
            bool found = false;
            for (vector<uint32_t>::iterator it = masks.begin();
                 it != masks.end(); ++it) {
              if (!((*it) & (0x1 << block_offset))) {
                *it |= 0x1 << block_offset;
                found = true;
                break;
              }
            }
            if (!found) masks.push_back(0x1 << block_offset);
          }
        }
      }
      //as->debug_stream << "finished iteration in " << t0.lap() << " usec" << endl;
      eit->second.clear(); // clear buffer
    }

    typedef vector
      <
        pair< bool /*done?*/, control_group::iterator /* iterator to current group (if not done) */ >
      > group_done_vec; // keeps track of which groups are done
    group_done_vec group_done;
    group_done.resize(alloc_map.size());

    size_t pos_first_non_empty = 0;
    size_t alloc_map_pos = 0;
    for (allocation_map::iterator it = alloc_map.begin();
         it != alloc_map.end(); ++it, ++alloc_map_pos) {
      assert(!it->second.empty());
      group_done[alloc_map_pos].first  = false;
      group_done[alloc_map_pos].second = it->second.begin();
    }

    //as->debug_stream << "flush_group_buffers: allocation took: " << t.lap() << " usec. IO reads starting" << endl;
    //as->debug_stream.flush();

    while (pos_first_non_empty < alloc_map.size()) {
      // find min block id for all groups
      uint64_t minSoFar = group_done[pos_first_non_empty].second->first; // block_id
      for (size_t i = pos_first_non_empty + 1; i < group_done.size(); i++) {
        pair< bool, control_group::iterator > &group = group_done[i];
        if (!group.first /* not done */ &&
            group.second->first /* block_id */ < minSoFar) {
          minSoFar = group.second->first;
        }
      }

      //// read in block (minSoFar)
      //if (as->block_id > ssize_t(minSoFar)) {
      //  // seek backwards
      //  as->block_id = ssize_t(minSoFar) - 1;
      //  fseek(as->fp, minSoFar * as->block_size, SEEK_SET);
      //  as->st.seeks++;
      //}
      assert(as->block_id <= ssize_t(minSoFar)); // TODO: support seeking

      // assert that the block_id is always in the range of the internal
      // buffer
      assert( as->internal_block_id <= as->block_id &&
              as->block_id < (as->internal_block_id + AggRowColPack::NumBlocksInternalBuffer) );

      // read the internal buffer forward until the range covers minSoFar
      while ( (as->internal_block_id + AggRowColPack::NumBlocksInternalBuffer) <= ssize_t(minSoFar) ) {
        size_t bread =
          fread(as->internal_buffer, 1,
                as->block_size * AggRowColPack::NumBlocksInternalBuffer,
                as->fp);
        assert(bread % as->agg_size == 0); // TODO: deal w/ bad files
        as->internal_block_id += AggRowColPack::NumBlocksInternalBuffer;
      }

      // set block_id/block_buf accordingly
      as->block_id = minSoFar;
      as->block_buf = as->internal_buffer + as->block_size * ( as->block_id - as->internal_block_id );

      // service the requests
      worker_msg m;
      m.shutdown = false;

      // init the mpz_t
      mpz_init2(m.value, as->agg_size * 8);
      MPZFromBytes(m.value, (const uint8_t *) as->block_buf, as->agg_size);
      //assert(((long)m.value->_mp_alloc * sizeof(mp_limb_t)) >= as->agg_size);

      for (size_t i = pos_first_non_empty; i < group_done.size(); i++) {
        pair< bool, control_group::iterator > &group = group_done[i];
        if (group.first /* done */ ||
            group.second->first /* block_id */ != minSoFar) continue;

        agg_group_key& group_key = alloc_map[i].first;
        agg_hash_agg_row_pack_state::per_group_state* s = NULL;
        if (!as->num_threads) s = &as->aggs[group_key];

        // push all these multiplications into the queue
        size_t group_id_idx = 0;
        for (vector< vector< uint32_t > >::iterator it = group.second->second.begin();
             it != group.second->second.end(); ++it, ++group_id_idx) {

          vector<mpz_wrapper>* ss = NULL;
          if (!as->num_threads) ss = &s->running_sums_mp[group_id_idx];

          for (vector<uint32_t>::iterator it0 = it->begin();
               it0 != it->end(); ++it0) {

            if (as->num_threads) {
              worker_msg::ent e;
              e.group_key = group_key;
              e.group_id  = group_id_idx;
              e.idx       = (*it0) - 1;
              m.ents.push_back(e);
            } else {
              vector<mpz_wrapper>& vr = *ss;
              size_t idx = (*it0) - 1;
              mpz_mul(vr[idx].mp, vr[idx].mp, m.value);
              mpz_mod(vr[idx].mp, vr[idx].mp, as->n2_mp);
            }

            // stats
            as->st.mults++;
            //static const uint32_t AllPosHigh = (0x1 << as->rows_per_agg) - 1;
            //if (group.second.front().mask == AllPosHigh) as->st.all_pos++;
          }
        }

        // advance this group

        group.second++;
        group.first = group.second == alloc_map[i].second.end();
      }

      if (as->num_threads) {
        as->worker_states[ as->workerCnt++ % as->num_threads ].q.push(m);
      } else {
        // clear the memory
        mpz_clear(m.value);
      }

      // find first non-empty
      for (group_done_vec::iterator it = group_done.begin() + pos_first_non_empty;
           it != group_done.end(); ++it) {
        if (it->first /*done*/) pos_first_non_empty++;
        else break;
      }
    }

    //as->debug_stream << "flush_group_buffers: IO reads finished: " << t.lap() << endl;
    //as->debug_stream.flush();
}

my_bool
agg_hash_agg_row_col_pack_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    agg_hash_agg_row_pack_state *as = (agg_hash_agg_row_pack_state *) initid->ptr;

    agg_group_key key(args, 1, as->num_group_fields);
    uint64_t row_id = EXTRACT_INT(as->num_group_fields + 1);

    // insert into buffer
    agg_hash_agg_row_pack_state::element_buffer_map::iterator it =
      as->element_buffer.find(key);
    vector< pair<uint64_t, uint64_t> >* elem_buffer = NULL;
    if (UNLIKELY(it == as->element_buffer.end())) {
      elem_buffer = &as->element_buffer[key];
      elem_buffer->reserve(AggRowColPack::BufferSize);
    } else {
      elem_buffer = &it->second;
    }

    // create bitmap
    uint64_t m = 0;
    for (size_t i = 0; i < as->num_groups; i++) {
      long long emit = EXTRACT_INT(as->num_group_fields + 7 + i);
      if (emit) m |= (0x1 << i);
    }

    //as->debug_stream << "element (row_id, groups) = " << row_id << ", " << m << endl;

    if (m) elem_buffer->push_back(make_pair(row_id, m));

    // if per group buffer is full, then flush
    if (UNLIKELY(elem_buffer->size() >= AggRowColPack::BufferSize)) {
      flush_group_buffers(as);
    }

    return true;
}

char *
agg_hash_agg_row_col_pack(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    agg_hash_agg_row_pack_state *as = (agg_hash_agg_row_pack_state *) initid->ptr;

    // flush all buffers
    flush_group_buffers(as);

    if (as->num_threads) {
      // wait until workers are done
      worker_msg m;
      m.shutdown = true;

      for (size_t i = 0; i < as->num_threads; i++) {
        // signal all threads to stop
        as->worker_states[i].q.push(m);
      }

      for (size_t i = 0; i < as->num_threads; i++) {
        // wait until computation is done
        pthread_join(as->workers[i], NULL);
      }

      for (size_t i = 0; i < as->num_threads; i++) {
        // merge (aggregate) results from workers into main results
        worker_state& s = as->worker_states[i];
        for (map< agg_group_key, vector< vector<mpz_wrapper> > >::iterator
              it = s.group_map.begin();
             it != s.group_map.end(); ++it) {
          assert( it->second.size() == as->num_groups );
          for (size_t group_id = 0; group_id < as->num_groups; group_id++) {
            vector<mpz_wrapper>& v =
              as->aggs[it->first].running_sums_mp[group_id];
            assert( v.size() == it->second[group_id].size() );
            for (size_t i = 0; i < v.size(); i++) {
              mpz_mul(v[i].mp, v[i].mp, it->second[group_id][i].mp);
              mpz_mod(v[i].mp, v[i].mp, as->n2_mp);
            }
          }
        }
      }
    }

    // calculate length
    map< agg_group_key, vector< size_t > > non_zero_m;
    *length = 0;
    for (agg_hash_agg_row_pack_state::group_map::iterator it = as->aggs.begin();
         it != as->aggs.end(); ++it) {

      as->debug_stream << "final group: " << it->first << endl;

      *length += it->first.size_needed(); /* group key */
      *length += sizeof(uint64_t); /* number of data items in this group (ie count(*)) */

      vector<size_t>& nn = non_zero_m[it->first];
      nn.resize(as->num_groups);
      for (size_t i = 0; i < as->num_groups; i++) {
        *length += sizeof(uint32_t); /* number of (non-zero) configurations */
        size_t non_zero = 0;
        size_t s = it->second.running_sums_mp[i].size();
        assert(s == ((0x1 << as->rows_per_agg) - 1));
        for (size_t idx = 0; idx < s; idx++) {
          if (mpz_cmp_ui(it->second.running_sums_mp[i][idx].mp, 1)) non_zero++;
        }
        // all non_zero aggs (mask plus actual agg)
        *length += non_zero * (sizeof(uint32_t) + as->agg_size);
        nn[i] = non_zero;
      }
    }

    //as->debug_stream << "seeks = " << as->st.seeks << endl;
    as->debug_stream << "run_in_parallel = " << (as->num_threads ? "yes" : "no") << endl;
    as->debug_stream << "length = " << *length << endl;
    as->debug_stream << "mults = " << as->st.mults << endl;
    as->debug_stream << "all_pos = " << as->st.all_pos << endl;
    as->debug_stream << "avg_masks_size = "
      << (double(as->st.masks_sum)/double(as->st.masks_cnt)) << endl;

    as->rbuf = malloc(*length);
    assert(as->rbuf); // TODO: handle OOM

    uint8_t* ptr = (uint8_t *) as->rbuf;

    for (agg_hash_agg_row_pack_state::group_map::iterator it = as->aggs.begin();
         it != as->aggs.end(); ++it) {
        ptr = it->first.write_key(ptr);

        uint64_t *iptr = (uint64_t *) ptr;
        *iptr = it->second.count;
        ptr += sizeof(uint64_t);

        vector<size_t>& nn = non_zero_m[it->first];
        for (size_t i = 0; i < as->num_groups; i++) {
          uint32_t *u32p = (uint32_t *) ptr;
          *u32p = nn[i];
          ptr += sizeof(uint32_t);

          size_t s = it->second.running_sums_mp[i].size();
          for (size_t idx = 0; idx < s; idx++) {
              if (!mpz_cmp_ui(it->second.running_sums_mp[i][idx].mp, 1)) {
                continue; // test usefullness
              }

              uint32_t *iptr = (uint32_t *) ptr;
              *iptr = (idx + 1);
              ptr += sizeof(uint32_t);

              BytesFromMPZ(ptr, it->second.running_sums_mp[i][idx].mp, as->agg_size);
              ptr += as->agg_size;
          }
        }
    }

    if (as->num_threads) {
      // need to clear worker states
      for (size_t i = 0; i < as->num_threads; i++) {
        worker_state& s = as->worker_states[i];
        for (map<agg_group_key, vector< vector<mpz_wrapper> > >::iterator
              it = s.group_map.begin();
             it != s.group_map.end(); ++it) {
          for (vector< vector<mpz_wrapper> >::iterator it0 = it->second.begin();
               it0 != it->second.end(); ++it0) {
            for (vector<mpz_wrapper>::iterator it1 = it0->begin();
                 it1 != it0->end(); ++it1) {
              mpz_clear(it1->mp);
            }
          }
        }
      }
    }

    return (char *) as->rbuf;
}

#else

Datum
func_add(PG_FUNCTION_ARGS)
{
    int lenN2, lenB;
    unsigned char * bytesN2;
    unsigned char * bytesA;
    unsigned char * bytesB;

    bytea * input = PG_GETARG_BYTEA_P(0);
    lenN2 = (VARSIZE(input)- VARHDRSZ)/2;
    //cerr << "lenN2 " << lenN2 << "\n";
    bytesA = (unsigned char *)VARDATA(input);
    bytesN2 = bytesA+lenN2;

    bytea * inputB = PG_GETARG_BYTEA_P(1);
    lenB = VARSIZE(inputB) - VARHDRSZ;
    //cerr << "lenB " << lenB << "\n";
    bytesB = (unsigned char *)VARDATA(inputB);

    if (lenB != lenN2) {
        cerr << "error: lenB != lenN2 \n";
        cerr << "lenB is " << lenB << " lenN2 is " << lenN2 << "\n";
        PG_RETURN_BYTEA_P(NULL);
    }

    if (DEBUG) { cerr << stringToByteInts(string(bytesA, lenN2)) << "\n"; }

    unsigned char * bytesRes = homomorphicAdd(bytesA, bytesB, bytesN2, lenN2);
    //cerr << "product "; myPrint(bytesRes, lenN2); cerr << " ";

    memcpy(VARDATA(input), bytesRes, lenN2);
    PG_RETURN_BYTEA_P(input);
}

Datum
func_add_final(PG_FUNCTION_ARGS)
{
    bytea * input = PG_GETARG_BYTEA_P(0);
    int lenN2 = (VARSIZE(input) - VARHDRSZ) / 2;

    bytea * res = (bytea *) palloc(lenN2 + VARHDRSZ);

    SET_VARSIZE(res, lenN2+VARHDRSZ);
    memcpy(VARDATA(res), VARDATA(input), lenN2);
    PG_RETURN_BYTEA_P(res);
}

#endif

// for update with increment
#if MYSQL_S
void
func_add_set_deinit(UDF_INIT *initid)
{
    if (initid->ptr)
        free(initid->ptr);
}

char *
func_add_set(UDF_INIT *initid, UDF_ARGS *args,
             char *result, unsigned long *length,
             char *is_null, char *error)
{
    if (initid->ptr)
        free(initid->ptr);

    uint64_t n2len = args->lengths[2];
    ZZ field, val, n2;
    ZZFromBytesFast(field, (const uint8_t *) args->args[0], args->lengths[0]);
    ZZFromBytesFast(val, (const uint8_t *) args->args[1], args->lengths[1]);
    ZZFromBytesFast(n2, (const uint8_t *) args->args[2], args->lengths[2]);

    ZZ res;
    MulMod(res, field, val, n2);

    void *rbuf = malloc((size_t)n2len);
    initid->ptr = (char *) rbuf;
    BytesFromZZ((uint8_t *) rbuf, res, (size_t)n2len);

    *length = (long unsigned int) n2len;
    return initid->ptr;
}

#else

Datum
func_add_set(PG_FUNCTION_ARGS)
{
    unsigned char * val;
    unsigned char * N2;
    unsigned char * field;
    unsigned int valLen, fieldLen, N2Len;

    field = getba(ARGS, 0, fieldLen);
    val = getba(ARGS, 1, valLen);
    N2 = getba(ARGS, 2, N2Len);

    myassert(fieldLen == N2Len, "length of the field differs from N2 len");
    myassert(valLen == N2Len, "length of val differs from N2 len");

    unsigned char * res = homomorphicAdd(field, val, N2, N2Len);

    bytea * resBytea = (bytea *) palloc(N2Len + VARHDRSZ);
    SET_VARSIZE(resBytea, N2Len + VARHDRSZ);
    memcpy(VARDATA(resBytea), res, N2Len);
    PG_RETURN_BYTEA_P(resBytea);
}

#endif

} /* extern "C" */
