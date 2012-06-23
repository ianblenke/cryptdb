#include <crypto-old/CryptoManager.hh> /* various functions for EDB */
#include <util/likely.hh>
#include <util/params.hh>
#include <util/util.hh>
#include <util/timer.hh>
#include <sys/time.h>
#include <time.h>

#include <map>

#include <gmp.h>
#include <pthread.h>
#include <tbb/concurrent_queue.h>
#include <cassert>

#define ATTR_UNUSED    __attribute__ ((unused))

#define TRACE()
#define SANITY(x) assert(x)

struct mpz_wrapper { mpz_t mp; };

namespace {

inline void
log(const std::string& s)
{
    /* Writes to the server's error log */
    fprintf(stderr, "%s\n", s.c_str());
}

inline AES_KEY *
get_key_SEM(const std::string &key)
{
    return CryptoManager::get_key_SEM(key);
}
/*
inline AES_KEY *
get_key_DET(const std::string &key)
{
    return CryptoManager::get_key_DET(key);
}
*/
inline uint64_t
decrypt_SEM(uint64_t value, AES_KEY * aesKey, uint64_t salt)
{
    return CryptoManager::decrypt_SEM(value, aesKey, salt);
}
/*
inline uint64_t
decrypt_DET(uint64_t ciph, AES_KEY* aesKey)
{
    return CryptoManager::decrypt_DET(ciph, aesKey);
}

inline uint64_t
encrypt_DET(uint64_t plaintext, AES_KEY * aesKey)
{
    return CryptoManager::encrypt_DET(plaintext, aesKey);
}
*/
inline std::string
decrypt_SEM(unsigned char *eValueBytes, uint64_t eValueLen,
            AES_KEY * aesKey, uint64_t salt)
{
    std::string c((char *) eValueBytes, (unsigned int) eValueLen);
    return CryptoManager::decrypt_SEM(c, aesKey, salt);
}
/*
inline std::string
decrypt_DET(unsigned char *eValueBytes, uint64_t eValueLen, AES_KEY * key)
{
    std::string c((char *) eValueBytes, (unsigned int) eValueLen);
    return CryptoManager::decrypt_DET(c, key);
}
*/
inline bool
search(const Token & token, const Binary & overall_ciph)
{
   return CryptoManager::searchExists(token, overall_ciph);
}

inline void
BytesFromMPZ(uint8_t* p, mpz_t rop, size_t n) {
    size_t count;
    mpz_export(p, &count, -1, n, -1, 0, rop);
    assert(count == 1);
}

inline void
InitMPZRunningSums(size_t rows_per_agg, bool allow_multi_slots, std::vector<mpz_wrapper>& v) {
    assert( rows_per_agg > 0 );
    size_t n_elems = allow_multi_slots ? ((0x1UL << rows_per_agg) - 1) : rows_per_agg;
    v.resize(n_elems);
    for (size_t i = 0; i < n_elems; i++) {
      mpz_init_set_ui(v[i].mp, 1);
    }
}

}
