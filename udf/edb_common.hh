#include <crypto-old/CryptoManager.hh> /* various functions for EDB */
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

#define LIKELY(pred)   __builtin_expect(!!(pred), true)
#define UNLIKELY(pred) __builtin_expect(!!(pred), false)
#define ATTR_UNUSED    __attribute__ ((unused))

#define TRACE()
#define SANITY(x) assert(x)

static inline void ATTR_UNUSED
log(std::string s)
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
