#pragma once

#include <stdint.h>
#include <assert.h>
#include <openssl/aes.h>
#include <vector>

class AES {
 public:
    AES(const std::vector<uint8_t> &key) {
        AssertValidKeySize(key.size());
        AES_set_encrypt_key(&key[0], key.size() * 8, &enc);
        AES_set_decrypt_key(&key[0], key.size() * 8, &dec);
    }

    AES(const uint8_t *key_data, size_t key_size) {
        AssertValidKeySize(key_size);
        AES_set_encrypt_key(key_data, key_size * 8, &enc);
        AES_set_decrypt_key(key_data, key_size * 8, &dec);
    }

    void block_encrypt(const uint8_t *ptext, uint8_t *ctext) const {
        AES_encrypt(ptext, ctext, &enc);
    }

    void block_decrypt(const uint8_t *ctext, uint8_t *ptext) const {
        AES_decrypt(ctext, ptext, &dec);
    }

    static const size_t blocksize = 16;

 private:
    static inline void AssertValidKeySize(size_t s) {
        assert(s == 16 || s == 24 || s == 32);
    }

    AES_KEY enc;
    AES_KEY dec;
};
