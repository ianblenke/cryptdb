#pragma once

#include <vector>
#include <stdint.h>
#include <string>
#include <crypto/BasicCrypto.hh>
#include <crypto/prng.hh>

class RND {
 public:
    RND(const std::string &key) {
	enckey = get_AES_enc_key(key);
	deckey = get_AES_dec_key(key);
	u = new urandom();
    }

    std::string encrypt(std::string pt) const {
	std::string salt = u->rand_string(saltlen);
	std::string ct = encrypt_AES_CBC(pt, enckey, salt, true);
	return salt + ct;
    }

    std::string decrypt(std::string ct) const {
	std::string salt = ct.substr(0, saltlen);
	std::string barect = ct.substr(saltlen, ct.length()-saltlen);
        return decrypt_AES_CBC(barect, deckey, salt, true);
    }


 private:
    AES_KEY * enckey;
    AES_KEY * deckey;
    PRNG * u;

    static const uint saltlen = 8; //in bytes
};
