#pragma once

#include <string>
#include <stdio.h>

#include <NTL/ZZ.h>

/*
 *
 * These functions implement OPE (order-preserving encryption).
 *
 * Optimization:
 * - batch encryption and decryption by temporarily storing assigned data
 * - first N intervals are generated using a PRG
 * - store in a tree for reuse
 */

class OPEInternals;

class OPE {
 public:
    /*
     * sizes are in bits
     * requires: key should have a number of bytes equal to OPE_KEY_SIZE
     */
    OPE(const std::string &key, unsigned int OPEPlaintextSize,
        unsigned int OPECiphertextSize);
    ~OPE();

    std::string encrypt(const NTL::ZZ &plaintext);

    std::string encrypt(const std::string &plaintext);
    std::string decrypt(const std::string &ciphertext);

    uint64_t encrypt(uint32_t plaintext);
    uint32_t decrypt(uint64_t ciphertext);

    uint64_t decryptToU64(const std::string &ciphertext);

 private:
    OPEInternals * iOPE;     //private methods and fields of OPE
};
