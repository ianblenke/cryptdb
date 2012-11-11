#pragma once

/*
 * Same implementation as BasicCrypto.hh with the change
 * that it brings AES to the interface 
 * needed fo the ope templatized implementation
 */

#include <crypto/BasicCrypto.hh>

struct aes_cbc {

    // if det is set this will be deterministic encryption
    // else, it will be random (salt prepended to ciphertext)
    aes_cbc(std::string passwd, bool det);

    std::string encrypt(std::string pt);
    std::string decrypt(std::string ct);
    
    AES_KEY * enc_key;
    AES_KEY * dec_key;
    bool is_det;
    std::string det_salt;

};
