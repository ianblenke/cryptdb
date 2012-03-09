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

#include <crypto-old/CryptoManager.hh> /* various functions for EDB */
#include <util/params.hh>
#include <util/util.hh>
#include <sys/time.h>
#include <time.h>

#include <map>

#include <gmp.h>
#include <pthread.h>
#include <tbb/concurrent_queue.h>
#include <cassert>

using namespace std;
using namespace NTL;
using namespace tbb;

#define LIKELY(pred)   __builtin_expect(!!(pred), true)
#define UNLIKELY(pred) __builtin_expect(!!(pred), false)

#define TRACE()
#define SANITY(x) assert(x)

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

my_bool  sum_char2_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     sum_char2_deinit(UDF_INIT *initid);
void     sum_char2_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  sum_char2_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   sum_char2(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

my_bool  agg_char2_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg_char2_deinit(UDF_INIT *initid);
void     agg_char2_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg_char2_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg_char2(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

my_bool  agg_char2_row_pack_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg_char2_row_pack_deinit(UDF_INIT *initid);
void     agg_char2_row_pack_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg_char2_row_pack_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg_char2_row_pack(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

my_bool  agg_char2_row_col_pack_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg_char2_row_col_pack_deinit(UDF_INIT *initid);
void     agg_char2_row_col_pack_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg_char2_row_col_pack_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg_char2_row_col_pack(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

my_bool  agg8_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg8_deinit(UDF_INIT *initid);
void     agg8_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg8_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg8(UDF_INIT *initid, UDF_ARGS *args, char *result,
             unsigned long *length, char *is_null, char *error);

my_bool  agg_profile_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg_profile_deinit(UDF_INIT *initid);
void     agg_profile_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg_profile_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg_profile(UDF_INIT *initid, UDF_ARGS *args, char *result,
                     unsigned long *length, char *is_null, char *error);

my_bool  agg_par_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
void     agg_par_deinit(UDF_INIT *initid);
void     agg_par_clear(UDF_INIT *initid, char *is_null, char *error);
my_bool  agg_par_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
char *   agg_par(UDF_INIT *initid, UDF_ARGS *args, char *result,
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

static void __attribute__((unused))
log(string s)
{
    /* Writes to the server's error log */
    fprintf(stderr, "%s\n", s.c_str());
}

static AES_KEY *
get_key_SEM(const string &key)
{
    return CryptoManager::get_key_SEM(key);
}
/*
static AES_KEY *
get_key_DET(const string &key)
{
    return CryptoManager::get_key_DET(key);
}
*/
static uint64_t
decrypt_SEM(uint64_t value, AES_KEY * aesKey, uint64_t salt)
{
    return CryptoManager::decrypt_SEM(value, aesKey, salt);
}
/*
static uint64_t
decrypt_DET(uint64_t ciph, AES_KEY* aesKey)
{
    return CryptoManager::decrypt_DET(ciph, aesKey);
}

static uint64_t
encrypt_DET(uint64_t plaintext, AES_KEY * aesKey)
{
    return CryptoManager::encrypt_DET(plaintext, aesKey);
}
*/
static string
decrypt_SEM(unsigned char *eValueBytes, uint64_t eValueLen,
            AES_KEY * aesKey, uint64_t salt)
{
    string c((char *) eValueBytes, (unsigned int) eValueLen);
    return CryptoManager::decrypt_SEM(c, aesKey, salt);
}
/*
static string
decrypt_DET(unsigned char *eValueBytes, uint64_t eValueLen, AES_KEY * key)
{
    string c((char *) eValueBytes, (unsigned int) eValueLen);
    return CryptoManager::decrypt_DET(c, key);
}
*/
static bool
search(const Token & token, const Binary & overall_ciph)
{

   return CryptoManager::searchExists(token, overall_ciph);

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
        ZZFromBytes(as->n2, (const uint8_t *) args->args[1],
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

struct sum_char2_state {
    std::map<uint16_t, std::pair<uint64_t, double> > aggs;
    void *rbuf;
};

my_bool
sum_char2_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    sum_char2_state *as = new sum_char2_state;
    as->rbuf = NULL;
    initid->ptr = (char *) as;
    args->arg_type[2] = REAL_RESULT; // coerce DECIMAL to REAL
    return 0;
}

void
sum_char2_deinit(UDF_INIT *initid)
{
    sum_char2_state *as = (sum_char2_state *) initid->ptr;
    free(as->rbuf);
    delete as;
}

void
sum_char2_clear(UDF_INIT *initid, char *is_null, char *error)
{
    sum_char2_state *as = (sum_char2_state *) initid->ptr;
    as->aggs.clear();
}

my_bool
sum_char2_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    sum_char2_state *as = (sum_char2_state *) initid->ptr;

    long long p0, p1;
    p0 = *((long long *) args->args[0]);
    p1 = *((long long *) args->args[1]);

    //assert(p0 >= 0 && p0 <= 0xFF);
    //assert(p1 >= 0 && p1 <= 0xFF);

    uint16_t key = (uint8_t(p0) << 8) | uint8_t(p1);

    std::map<uint16_t, std::pair<uint64_t, double> >::iterator it =
        as->aggs.find(key);

    double real_val = *((double*) args->args[2]);

    if (UNLIKELY(it == as->aggs.end())) {
        std::pair<uint64_t, double>& p = as->aggs[key]; // creates on demand
        p.first  = 1;
        p.second = real_val;
    } else {
        it->second.first++;
        it->second.second += real_val;
    }
    return true;
}

char *
sum_char2(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    sum_char2_state *as = (sum_char2_state *) initid->ptr;
    *length = (2 + sizeof(uint64_t) + sizeof(double)) * as->aggs.size();
    as->rbuf = malloc(*length);
    assert(as->rbuf); // TODO: handle OOM
    uint8_t* ptr = (uint8_t *) as->rbuf;
    for (std::map<uint16_t, std::pair<uint64_t, double> >::iterator it = as->aggs.begin();
         it != as->aggs.end(); ++it) {
        *ptr++ = (uint8_t) ((it->first >> 8) & 0xFF);
        *ptr++ = (uint8_t) (it->first & 0xFF);
        uint64_t *iptr = (uint64_t *) ptr;
        *iptr = it->second.first;
        ptr += sizeof(uint64_t);
        double *dptr = (double *)ptr;
        *dptr = it->second.second;
        ptr += sizeof(double);
    }
    return (char *) as->rbuf;
}

struct agg_char2_state {
    std::map<uint16_t, std::pair<uint64_t, ZZ> > aggs;
    ZZ n2;
    //bool n2_set;
    void *rbuf;
};

my_bool
agg_char2_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    agg_char2_state *as = new agg_char2_state;
    as->rbuf = NULL;

    if (args->args[3] == NULL) {
        strcpy(message, "need to provide PK");
        return 1;
    }

    ZZFromBytes(as->n2, (const uint8_t *) args->args[3],
                args->lengths[3]);
    //as->n2_set = 1;

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
    agg_char2_state *as = (agg_char2_state *) initid->ptr;
    as->aggs.clear();
}

my_bool
agg_char2_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    agg_char2_state *as = (agg_char2_state *) initid->ptr;
    //if (!as->n2_set) {
    //    ZZFromBytes(as->n2, (const uint8_t *) args->args[3],
    //                args->lengths[3]);
    //    as->n2_set = 1;
    //}

    long long p0, p1;
    p0 = *((long long *) args->args[0]);
    p1 = *((long long *) args->args[1]);

    //assert(p0 >= 0 && p0 <= 0xFF);
    //assert(p1 >= 0 && p1 <= 0xFF);

    uint16_t key = (uint8_t(p0) << 8) | uint8_t(p1);

    std::map<uint16_t, std::pair<uint64_t, ZZ> >::iterator it =
        as->aggs.find(key);
    ZZ* sum = NULL;
    if (UNLIKELY(it == as->aggs.end())) {
        std::pair<uint64_t, ZZ>& p = as->aggs[key]; // creates on demand
        p.first  = 1;
        p.second = to_ZZ(1);
        sum = &p.second;

        //assert(as->aggs[key].first == 1);
        //assert(as->aggs[key].second == to_ZZ(1));
    } else {
        it->second.first++;
        sum = &it->second.second;
    }
    //assert(sum);

    ZZ e;
    ZZFromBytes(e, (const uint8_t *) args->args[2], args->lengths[2]);

    MulMod(*sum, *sum, e, as->n2);
    return true;
}

char *
agg_char2(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    agg_char2_state *as = (agg_char2_state *) initid->ptr;
    *length = (2 + sizeof(uint64_t) + CryptoManager::Paillier_len_bytes) * as->aggs.size();
    as->rbuf = malloc(*length);
    assert(as->rbuf); // TODO: handle OOM
    uint8_t* ptr = (uint8_t *) as->rbuf;
    for (std::map<uint16_t, std::pair<uint64_t, ZZ> >::iterator it = as->aggs.begin();
         it != as->aggs.end(); ++it) {
        *ptr++ = (uint8_t) ((it->first >> 8) & 0xFF);
        *ptr++ = (uint8_t) (it->first & 0xFF);
        uint64_t *iptr = (uint64_t *) ptr;
        *iptr = it->second.first;
        ptr += sizeof(uint64_t);
        BytesFromZZ(ptr, it->second.second,
                    CryptoManager::Paillier_len_bytes);
        ptr += CryptoManager::Paillier_len_bytes;
    }
    return (char *) as->rbuf;
}

struct agg_char2_row_pack_state {
    struct per_group_state {
        std::vector< std::vector<ZZ> > running_sums;
        uint64_t count;
    };
    typedef std::map<uint16_t, per_group_state> group_map;
    group_map aggs;
    uint32_t fields_mask;
    ZZ n2;
    void *rbuf;

    // internal element buffer
    typedef std::map< uint16_t, std::vector< uint64_t > > element_buffer_map;
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

static size_t NumBitsHigh(uint32_t t) {
    size_t cnt = 0;
    size_t s = sizeof(t) * 8;
    while (s--) {
        cnt += t & 0x1;
        t >>= 1;
    }
    return cnt;
}

struct AggRowPack {
  static const size_t AggSize = 256;
  static const size_t RowsPerBlock = 12;
};

my_bool
agg_char2_row_pack_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if (args->arg_count != 5) {
        strcpy(message, "need to provide 5 args");
        return 1;
    }

    agg_char2_row_pack_state *as = new agg_char2_row_pack_state;
    as->rbuf = NULL;

    if (args->args[3] == NULL) {
        strcpy(message, "need to indicate which fields are of interest");
        return 1;
    }

    as->fields_mask = *((long long *) args->args[3]);
    if (as->fields_mask > 0x1F /* hardcode 5 fields for now */) {
        sprintf(message, "fields out of range: %d", as->fields_mask);
        return 1;
    }

    if (args->args[4] == NULL) {
        strcpy(message, "need to provide PK");
        return 1;
    }

    ZZFromBytes(as->n2, (const uint8_t *) args->args[4],
                args->lengths[4]);

    as->block_id = -1;

    as->fp = fopen("/tmp/tpch-0.05/lineitem_enc/data", "rb");
    assert(as->fp);

    as->block_size = AggRowPack::AggSize * 5; // hardcode 5 for now
    as->block_buf = (char *) malloc(as->block_size);

    //as->debug_stream.open("/tmp/debug.txt");
    //assert(as->debug_stream.good());

    initid->ptr = (char *) as;
    return 0;
}

void
agg_char2_row_pack_deinit(UDF_INIT *initid)
{
    agg_char2_row_pack_state *as = (agg_char2_row_pack_state *) initid->ptr;
    free(as->rbuf);
    free(as->block_buf);
    fclose(as->fp);
    //as->debug_stream.flush();
    delete as;
}

void
agg_char2_row_pack_clear(UDF_INIT *initid, char *is_null, char *error)
{
    agg_char2_row_pack_state *as = (agg_char2_row_pack_state *) initid->ptr;
    as->aggs.clear();
    as->block_id = -1;
    fseek(as->fp, 0, SEEK_SET);
}

my_bool
agg_char2_row_pack_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    agg_char2_row_pack_state *as = (agg_char2_row_pack_state *) initid->ptr;

    long long p0, p1, row_id;
    p0 = *((long long *) args->args[0]);
    p1 = *((long long *) args->args[1]);

    uint16_t key = (uint8_t(p0) << 8) | uint8_t(p1);

    agg_char2_row_pack_state::group_map::iterator it = as->aggs.find(key);
    agg_char2_row_pack_state::per_group_state* s = NULL;
    if (UNLIKELY(it == as->aggs.end())) {
        s = &as->aggs[key]; // creates on demand
        vector<ZZ> cpy;
        cpy.resize(AggRowPack::RowsPerBlock, to_ZZ(1));
        s->running_sums.resize(NumBitsHigh(as->fields_mask), cpy);
        s->count = 1;
        //as->debug_stream <<
        //  "create new group " << p0 << " " << p1 << ": "
        //  << "fields_mask: " << as->fields_mask
        //  << " num_bits_high: " << NumBitsHigh(as->fields_mask) << endl;
    } else {
        s = &it->second;
        s->count++;
    }

    // compute block ID from row ID
    row_id = *((long long *) args->args[2]);
    size_t block_id = row_id / AggRowPack::RowsPerBlock;

    assert((ssize_t)block_id >= as->block_id); // ONLY support scanning forward for now!

    // read in block
    while (as->block_id < (ssize_t)block_id) {
        // scan forward
        size_t bread = fread(as->block_buf, 1, as->block_size, as->fp);
        assert(bread == as->block_size); // TODO: handle bad files
        as->block_id++;
        // TODO: seek if we are really far away instead?
        //as->debug_stream << "read block " << as->block_id << endl;
    }

    // compute block offset
    size_t block_offset = row_id % AggRowPack::RowsPerBlock;

    //as->debug_stream << row_id << ": " << block_id << " " << block_offset << endl;

    // do sum for each field of interest
    size_t idx = 0;
    for (size_t i = 0; i < 5; i++) { // TODO: don't hardcode 5
        if (as->fields_mask & (0x1 << i)) {
            //as->debug_stream << "  idx: " << idx << endl;
            ZZ &sum = s->running_sums[idx][block_offset];
            ZZ e;
            ZZFromBytes(
                e,
                (const uint8_t *) as->block_buf + (i * AggRowPack::AggSize),
                AggRowPack::AggSize);
            MulMod(sum, sum, e, as->n2);
            idx++;
        }
    }
    return true;
}

char *
agg_char2_row_pack(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    agg_char2_row_pack_state *as = (agg_char2_row_pack_state *) initid->ptr;
    *length =
        (2 + sizeof(uint64_t) +
         NumBitsHigh(as->fields_mask) * AggRowPack::RowsPerBlock *
         CryptoManager::Paillier_len_bytes) * as->aggs.size();

    as->rbuf = malloc(*length);
    assert(as->rbuf); // TODO: handle OOM

    uint8_t* ptr = (uint8_t *) as->rbuf;

    for (agg_char2_row_pack_state::group_map::iterator it = as->aggs.begin();
         it != as->aggs.end(); ++it) {
        //as->debug_stream << "group :" << ((it->first >> 8) & 0xFF) << " " << (it->first & 0xFF) << endl;
        *ptr++ = (uint8_t) ((it->first >> 8) & 0xFF);
        *ptr++ = (uint8_t) (it->first & 0xFF);
        uint64_t *iptr = (uint64_t *) ptr;
        *iptr = it->second.count;
        ptr += sizeof(uint64_t);

        for (vector< vector<ZZ> >::iterator iit =
                it->second.running_sums.begin();
             iit != it->second.running_sums.end(); ++iit) {
            for (vector<ZZ>::iterator zzt = iit->begin();
                 zzt != iit->end(); ++zzt) {
                BytesFromZZ(ptr, *zzt,
                            CryptoManager::Paillier_len_bytes);
                //as->debug_stream << marshallBinary(string((char*)ptr, 256U)) << endl;
                ptr += CryptoManager::Paillier_len_bytes;
            }
        }

    }
    return (char *) as->rbuf;
}

struct AggRowColPack {
  static const size_t AggSize = 1256 * 2 / 8;
  static const size_t RowsPerBlock = 3;
  static const size_t BufferSize = 1024;

  static const size_t NumBlocksInternalBuffer = 524288;
  //static const size_t NumBlocksInternalBuffer = 4194304;
};

my_bool
agg_char2_row_col_pack_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if (args->arg_count != 5) {
        strcpy(message, "need to provide 5 args");
        return 1;
    }

    agg_char2_row_pack_state *as = new agg_char2_row_pack_state;
    as->rbuf = NULL;

    if (args->args[3] == NULL) {
        strcpy(message, "need to provide PK");
        return 1;
    }

    if (args->args[4] == NULL) {
        strcpy(message, "need to provide name");
        return 1;
    }
    // TODO: fix security
    string name =
      "/tmp/" + string(args->args[4], args->lengths[4]) +
      "/lineitem_enc/row_col_pack/data";

    ZZFromBytes(as->n2, (const uint8_t *) args->args[3],
                args->lengths[3]);

    as->fp = fopen(name.c_str(), "rb");
    assert(as->fp);

    // load the first block
    as->internal_buffer =
      (char *) malloc( AggRowColPack::AggSize * AggRowColPack::NumBlocksInternalBuffer );
    assert(as->internal_buffer);
    as->internal_block_id = 0;
    fread(as->internal_buffer, 1,
          AggRowColPack::AggSize * AggRowColPack::NumBlocksInternalBuffer,
          as->fp);

    as->block_id = 0;
    as->block_size = AggRowColPack::AggSize;
    as->block_buf = as->internal_buffer;

    as->debug_stream.open("/tmp/debug.txt");
    assert(as->debug_stream.good());

    initid->ptr = (char *) as;
    return 0;
}

void
agg_char2_row_col_pack_deinit(UDF_INIT *initid)
{
    agg_char2_row_pack_state *as = (agg_char2_row_pack_state *) initid->ptr;
    free(as->rbuf);
    free(as->internal_buffer);
    fclose(as->fp);
    as->debug_stream.flush();
    delete as;
}

void
agg_char2_row_col_pack_clear(UDF_INIT *initid, char *is_null, char *error)
{
    agg_char2_row_pack_state *as = (agg_char2_row_pack_state *) initid->ptr;
    as->aggs.clear();
    as->block_id = 0;
    as->block_buf = as->internal_buffer;
    as->internal_block_id = 0;
    fseek(as->fp, 0, SEEK_SET);
    // reload the first block
    fread(as->internal_buffer, 1,
          AggRowColPack::AggSize * AggRowColPack::NumBlocksInternalBuffer,
          as->fp);
}

static void flush_group_buffers(agg_char2_row_pack_state *as) {

    // value is ordered list based on the pair's first element
    typedef vector< pair< uint16_t, list< pair< uint64_t, uint32_t > > > >
            allocation_map;
    allocation_map alloc_map;
    alloc_map.reserve(as->element_buffer.size());
    for (agg_char2_row_pack_state::element_buffer_map::iterator
            eit = as->element_buffer.begin();
         eit != as->element_buffer.end(); ++eit) {
      // lookup agg group
      agg_char2_row_pack_state::group_map::iterator it =
        as->aggs.find(eit->first);
      agg_char2_row_pack_state::per_group_state* s = NULL;
      if (UNLIKELY(it == as->aggs.end())) {
          s = &as->aggs[eit->first]; // creates on demand
          vector<ZZ> cpy;
          // the index is...
          cpy.resize((0x1 << AggRowColPack::RowsPerBlock) - 1, to_ZZ(1));
          s->running_sums.resize(1, cpy);
          s->count = 0;
      } else {
          s = &it->second;
      }
      s->count += eit->second.size();

      map< uint64_t, vector< uint32_t > > m;
      for (vector<uint64_t>::iterator it = eit->second.begin();
           it != eit->second.end(); ++it) {
        uint64_t row_id = *it;

        // compute block ID from row ID
        uint64_t block_id = row_id / AggRowColPack::RowsPerBlock;
        // compute block offset
        uint64_t block_offset = row_id % AggRowColPack::RowsPerBlock;

        vector<uint32_t>& masks = m[block_id];
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
      eit->second.clear(); // clear buffer

      alloc_map.push_back(make_pair(eit->first, list< pair< uint64_t, uint32_t > >()));
      list< pair< uint64_t, uint32_t > >& elems = alloc_map.back().second;
      for (map< uint64_t, vector< uint32_t > >::iterator it = m.begin();
           it != m.end(); ++it) {
        for (vector<uint32_t>::iterator iit = it->second.begin();
             iit != it->second.end(); ++iit) {
          elems.push_back(make_pair(it->first, *iit));
        }
        as->st.masks_sum += it->second.size();
        as->st.masks_cnt++;
      }
    }

    size_t pos_first_non_empty = 0;
    for (allocation_map::iterator it = alloc_map.begin();
         it != alloc_map.end(); ++it) {
      if (it->second.empty()) pos_first_non_empty++;
      else break;
    }

    while (pos_first_non_empty < alloc_map.size()) {
      // find min block id for all groups
      uint64_t minSoFar = alloc_map[pos_first_non_empty].second.front().first;
      for (size_t i = pos_first_non_empty + 1; i < alloc_map.size(); i++) {
        pair< uint16_t, list< pair< uint64_t, uint32_t > > > &group =
          alloc_map[i];
        if (!group.second.empty() && group.second.front().first < minSoFar) {
          minSoFar = group.second.front().first;
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
        assert(bread % AggRowColPack::AggSize == 0); // TODO: deal w/ bad files
        as->internal_block_id += AggRowColPack::NumBlocksInternalBuffer;
      }

      // set block_id/block_buf accordingly
      as->block_id = minSoFar;
      as->block_buf = as->internal_buffer + as->block_size * ( as->block_id - as->internal_block_id );

      // service the requests
      for (size_t i = pos_first_non_empty; i < alloc_map.size(); i++) {
        pair< uint16_t, list< pair< uint64_t, uint32_t > > > &group =
          alloc_map[i];
        if (group.second.empty() ||
            group.second.front().first != minSoFar) continue;
        agg_char2_row_pack_state::per_group_state& s = as->aggs[group.first];

        // use group.second.front().second - 1 as an index into the running
        // sums vector

        ZZ &sum = s.running_sums[0][group.second.front().second - 1];
        ZZ e;
        ZZFromBytes(
            e,
            (const uint8_t *) as->block_buf,
            AggRowColPack::AggSize);
        MulMod(sum, sum, e, as->n2);

        // stats
        as->st.mults++;
        static const uint32_t AllPosHigh = (0x1 << AggRowColPack::RowsPerBlock) - 1;
        if (group.second.front().second == AllPosHigh) as->st.all_pos++;

        group.second.pop_front();
      }

      // find first non-empty
      pos_first_non_empty = 0;
      for (allocation_map::iterator it = alloc_map.begin();
           it != alloc_map.end(); ++it) {
        if (it->second.empty()) pos_first_non_empty++;
        else break;
      }
    }
}

my_bool
agg_char2_row_col_pack_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    agg_char2_row_pack_state *as = (agg_char2_row_pack_state *) initid->ptr;

    long long p0, p1, row_id;
    p0     = *((long long *) args->args[0]);
    p1     = *((long long *) args->args[1]);
    row_id = *((long long *) args->args[2]);

    uint16_t key = (uint8_t(p0) << 8) | uint8_t(p1);

    // insert into buffer
    agg_char2_row_pack_state::element_buffer_map::iterator it =
      as->element_buffer.find(key);
    vector<uint64_t>* elem_buffer = NULL;
    if (UNLIKELY(it == as->element_buffer.end())) {
      elem_buffer = &as->element_buffer[key];
      elem_buffer->reserve(AggRowColPack::BufferSize);
    } else {
      elem_buffer = &it->second;
    }
    elem_buffer->push_back(row_id);

    // if per group buffer is full, then flush
    if (UNLIKELY(elem_buffer->size() >= AggRowColPack::BufferSize)) {
      flush_group_buffers(as);
    }

    return true;
}

char *
agg_char2_row_col_pack(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    agg_char2_row_pack_state *as = (agg_char2_row_pack_state *) initid->ptr;

    // flush all buffers
    flush_group_buffers(as);

    *length =
        (2 /* group key */ +
         sizeof(uint64_t) /* group count */ +
         sizeof(uint32_t) /* num of agg configs to come */ +
         ((0x1 << AggRowColPack::RowsPerBlock) - 1) *
         (sizeof(uint32_t) /* mask of interest */ +
         AggRowColPack::AggSize)) * as->aggs.size();

    //as->debug_stream << "seeks = " << as->st.seeks << endl;
    as->debug_stream << "length = " << *length << endl;
    as->debug_stream << "mults = " << as->st.mults << endl;
    as->debug_stream << "all_pos = " << as->st.all_pos << endl;
    as->debug_stream << "avg_masks_size = "
      << (double(as->st.masks_sum)/double(as->st.masks_cnt)) << endl;

    as->rbuf = malloc(*length);
    assert(as->rbuf); // TODO: handle OOM

    uint8_t* ptr = (uint8_t *) as->rbuf;

    for (agg_char2_row_pack_state::group_map::iterator it = as->aggs.begin();
         it != as->aggs.end(); ++it) {
        *ptr++ = (uint8_t) ((it->first >> 8) & 0xFF);
        *ptr++ = (uint8_t) (it->first & 0xFF);

        uint64_t *iptr = (uint64_t *) ptr;
        *iptr = it->second.count;
        ptr += sizeof(uint64_t);

        uint32_t *u32p = (uint32_t *) ptr;
        *u32p = (0x1 << AggRowColPack::RowsPerBlock) - 1;
        ptr += sizeof(uint32_t);

        size_t s = it->second.running_sums[0].size();
        assert(s == ((0x1 << AggRowColPack::RowsPerBlock) - 1));
        for (size_t idx = 0; idx < s; idx++) {
            uint32_t *iptr = (uint32_t *) ptr;
            *iptr = (idx + 1);
            ptr += sizeof(uint32_t);

            BytesFromZZ(ptr, it->second.running_sums[0][idx], AggRowColPack::AggSize);
            ptr += AggRowColPack::AggSize;
        }
    }
    return (char *) as->rbuf;
}

// agg8()

my_bool
agg8_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return agg_init(initid, args, message);
}

void
agg8_deinit(UDF_INIT *initid)
{
    agg_deinit(initid);
}

void
agg8_clear(UDF_INIT *initid, char *is_null, char *error)
{
    agg_clear(initid, is_null, error);
}

//args will be element to add, constant N2
my_bool
agg8_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    // means we don't add
    if (args->args[0] == NULL) return true;
    SANITY(args->arg_count == (8 + 1));

    uint8_t buf[CryptoManager::Paillier_len_bytes];

    // copy args into buf
    for (size_t i = 0; i < 8; i++) {
        // each individual buffer is 32-bytes (8 * 32 = 256 = Paillier_len_bytes)
        SANITY(args->lengths[i] == 32);
        memcpy(buf + (i * 32), args->args[i], 32);
    }

    agg_state *as = (agg_state *) initid->ptr;
    if (!as->n2_set) {
        ZZFromBytes(as->n2, (const uint8_t *) args->args[8],
                    args->lengths[8]);
        as->n2_set = 1;
    }

    ZZ e;
    ZZFromBytes(e, (const uint8_t *) buf, sizeof(buf));

    MulMod(as->sum, as->sum, e, as->n2);
    return true;
}

char *
agg8(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    return agg(initid, args, result, length, is_null, error);
}

// agg_profile()

struct agg_profile_state {
  struct agg_state agg;
  uint64_t start;
  uint64_t realstart;
};

static uint64_t cur_usec() {
    //struct timeval tv;
    //gettimeofday(&tv, 0);
    //return ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return ((uint64_t)t.tv_sec) * 1000000 + (t.tv_nsec / 1000);
}

my_bool
agg_profile_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    agg_profile_state *as = new agg_profile_state();
    as->realstart = as->start = cur_usec();
    as->agg.rbuf = malloc(sizeof(uint64_t) + CryptoManager::Paillier_len_bytes);
    initid->ptr = (char *) as;
    return 0;
}

void
agg_profile_deinit(UDF_INIT *initid)
{
    agg_profile_state *as = (agg_profile_state *) initid->ptr;
    uint64_t now = cur_usec();
    uint64_t diff = now - as->realstart;
    cout << "total: " << diff << endl;
    free(as->agg.rbuf);
    delete as;
}

void
agg_profile_clear(UDF_INIT *initid, char *is_null, char *error)
{
    agg_profile_state *as = (agg_profile_state *) initid->ptr;
    as->start = cur_usec();
    as->agg.sum = to_ZZ(1);
    as->agg.n2_set = 0;
}

//args will be element to add, constant N2
my_bool
agg_profile_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    agg_profile_state *as = (agg_profile_state *) initid->ptr;
    if (!as->agg.n2_set) {
        ZZFromBytes(as->agg.n2, (const uint8_t *) args->args[1],
                    args->lengths[1]);
        as->agg.n2_set = 1;
    }

    ZZ e;
    ZZFromBytes(e, (const uint8_t *) args->args[0], args->lengths[0]);

    MulMod(as->agg.sum, as->agg.sum, e, as->agg.n2);
    return true;
}

char *
agg_profile(UDF_INIT *initid, UDF_ARGS *args, char *result,
            unsigned long *length, char *is_null, char *error)
{
    agg_profile_state *as = (agg_profile_state *) initid->ptr;
    uint64_t now = cur_usec();
    uint64_t *p = (uint64_t*) as->agg.rbuf;
    cout << (now - as->start) << endl;
    *p = now - as->start;

    BytesFromZZ(((uint8_t*)as->agg.rbuf) + sizeof(uint64_t), as->agg.sum,
                CryptoManager::Paillier_len_bytes);
    *length = sizeof(uint64_t) + CryptoManager::Paillier_len_bytes;
    return (char *) as->agg.rbuf;
}

// --------------------------------------------------

struct memory_slab {
public:
  static const size_t NSlots = 32;
  static const size_t SlabSize = 256 * NSlots;

  memory_slab() : stream_end(false), is_raw(false), num_valid(0) {
    memset(ptrs, 0, sizeof(ptrs));
    memset(sizes, 0, sizeof(sizes));
  }

  bool stream_end;
  bool is_raw;

  uint8_t * ptrs[NSlots];
  size_t sizes[NSlots];
  size_t num_valid; // between 0-NSlots inclusive

  inline bool needs_init() const { return ptrs[0] == NULL; }
  inline void init() {
    ptrs[0] = (uint8_t*) malloc(SlabSize);
  }
  inline uint8_t * init_raw(size_t bytes) {
    ptrs[0] = (uint8_t*) malloc(bytes);
    is_raw = true;
    sizes[0] = bytes;
    num_valid = 1;
    return ptrs[0];
  }

  inline void free() {
    ::free(ptrs[0]);
  }

  inline uint8_t * tail() {
    SANITY(!is_raw);
    SANITY(!needs_init());
    if (num_valid == 0) return ptrs[0];
    return ptrs[num_valid - 1] + sizes[num_valid - 1];
  }

  inline const uint8_t * const_tail() const {
    SANITY(!is_raw);
    SANITY(!needs_init());
    if (num_valid == 0) return ptrs[0];
    return ptrs[num_valid - 1] + sizes[num_valid - 1];
  }

  inline uint8_t * alloc(size_t bytes) {
    SANITY(!is_raw);
    SANITY(!needs_init());
    SANITY(!slots_full());
    SANITY(remaining() >= bytes);
    uint8_t * end = tail();
    num_valid++;
    ptrs[num_valid - 1] = end;
    sizes[num_valid - 1] = bytes;
    return end;
  }

  inline bool slots_full() const {
    SANITY(!is_raw);
    return num_valid == NSlots;
  }

  inline size_t remaining() const {
    SANITY(!is_raw);
    if (num_valid == 0) return SlabSize;
    SANITY(1 <= num_valid && num_valid <= NSlots);
    const uint8_t * end = const_tail();
    return SlabSize - (end - ptrs[0]);
  }
};

static inline memory_slab mem_slab_end() {
  memory_slab m;
  m.stream_end = true;
  return m;
}

struct agg_par_state;

struct worker_ctx {
public:
  worker_ctx(agg_par_state *as) : as(as) { mpz_init_set_ui(sum, 1); }
  ~worker_ctx() { mpz_clear(sum); }
  agg_par_state *as;
  mpz_t sum;
  concurrent_bounded_queue<memory_slab> q;
};

struct agg_par_state {
public:
  agg_par_state() { mpz_init(mod); }
  ~agg_par_state() { mpz_clear(mod); }

  bool mod_set;
  mpz_t mod;

  bool threads_inited;
  vector<pthread_t> thds;
  vector<worker_ctx*> ctxs;

  memory_slab cur_slab;
  uint32_t ctr;

  void *rbuf;

  inline worker_ctx* next_ctx() {
    return ctxs[ctr++ % ctxs.size()];
  }
};

static void *thd_main(void *ptr) {
  worker_ctx *ctx = (worker_ctx *)ptr;

  mpz_t p;
  mpz_init(p);

  memory_slab slab;
  while (true) {
    ctx->q.pop(slab);
    if (slab.stream_end) {
      break;
    } else {
      for (size_t i = 0; i < slab.num_valid; i++) {
        mpz_import(p, slab.sizes[i], -1, sizeof(uint8_t), 0, 0, slab.ptrs[i]);
        mpz_mul(ctx->sum, ctx->sum, p);
        mpz_mod(ctx->sum, ctx->sum, ctx->as->mod);
      }
      slab.free();
    }
  }

  mpz_clear(p);
  return NULL;
}

my_bool
agg_par_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    TRACE();
    agg_par_state *as = new agg_par_state();
    as->rbuf = malloc(CryptoManager::Paillier_len_bytes);
    as->mod_set = 0;
    as->threads_inited = false;
    initid->ptr = (char *) as;
    return 0;
}

void
agg_par_deinit(UDF_INIT *initid)
{
    TRACE();
    agg_par_state *as = (agg_par_state *) initid->ptr;
    free(as->rbuf);
    delete as;
}

void
agg_par_clear(UDF_INIT *initid, char *is_null, char *error)
{
    TRACE();
    agg_par_state *as = (agg_par_state *) initid->ptr;
    SANITY(!as->threads_inited);
    as->ctr = 0;
}

static const size_t nthreads = 16;

//args will be element to add, constant N2
my_bool
agg_par_add(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    TRACE();
    agg_par_state *as = (agg_par_state *) initid->ptr;
    if (UNLIKELY(!as->mod_set)) {
        mpz_import(as->mod, args->lengths[1], -1, sizeof(uint8_t), 0, 0, args->args[1]);
        as->mod_set = 1;
    }

    if (UNLIKELY(!as->threads_inited)) {
      for (size_t i = 0; i < nthreads; i++) {
        worker_ctx *ctx = new worker_ctx(as);
        pthread_t t;
        pthread_create(&t, NULL, thd_main, ctx);
        as->thds.push_back(t);
        as->ctxs.push_back(ctx);
      }
      as->threads_inited = true;
    }

    size_t count = args->lengths[0];
    const uint8_t *buffer = (const uint8_t*) args->args[0];
    if (UNLIKELY(count > memory_slab::SlabSize)) {
      // won't fit in entire slab
      memory_slab special;
      uint8_t *p = special.init_raw(count);
      memcpy(p, buffer, count);
      as->next_ctx()->q.push(special);
    } else {
      if (count > as->cur_slab.remaining() /* won't fit */ ||
          as->cur_slab.slots_full() /* no more slots */) {
        // need new mem slab
        SANITY(!as->cur_slab.needs_init());
        SANITY(as->cur_slab.remaining() != memory_slab::SlabSize);
        as->next_ctx()->q.push(as->cur_slab);
        as->cur_slab = memory_slab(); // reset
      }

      SANITY(count <= as->cur_slab.remaining() &&
             !as->cur_slab.slots_full());

      if (as->cur_slab.needs_init()) as->cur_slab.init();
      uint8_t *p = as->cur_slab.alloc(count);
      memcpy(p, buffer, count);
    }

    return true;
}

char *
agg_par(UDF_INIT *initid, UDF_ARGS *args, char *result,
    unsigned long *length, char *is_null, char *error)
{
    TRACE();

    agg_par_state *as = (agg_par_state *) initid->ptr;
    SANITY(as->threads_inited);

    // send outstanding slab
    if (as->cur_slab.num_valid > 0) {
      SANITY(!as->cur_slab.needs_init());
      as->next_ctx()->q.push(as->cur_slab);
      as->cur_slab = memory_slab();
    }

    // send signal to stop worker threads + join
    mpz_t sum;
    mpz_init_set_si(sum, 1);
    for (size_t i = 0; i < as->ctxs.size(); i++) {
      as->ctxs[i]->q.push(mem_slab_end());
      pthread_join(as->thds[i], NULL);
      mpz_mul(sum, sum, as->ctxs[i]->sum);
      mpz_mod(sum, sum, as->mod);
    }

    // cleanup thread stuff
    for (size_t i = 0; i < as->ctxs.size(); i++) {
      delete as->ctxs[i];
    }
    as->thds.clear();
    as->ctxs.clear();
    as->threads_inited = false;

    size_t count;
    mpz_export(as->rbuf, &count, -1, CryptoManager::Paillier_len_bytes, -1, 0, sum);
    SANITY(count == 1);
    *length = CryptoManager::Paillier_len_bytes;
    mpz_clear(sum);
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
    ZZFromBytes(field, (const uint8_t *) args->args[0], args->lengths[0]);
    ZZFromBytes(val, (const uint8_t *) args->args[1], args->lengths[1]);
    ZZFromBytes(n2, (const uint8_t *) args->args[2], args->lengths[2]);

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
