#pragma once

#include <vector>
#include <stdint.h>


template<class BlockCipher>
void
xor_block(uint8_t *dst, const uint8_t *a, const uint8_t *b)
{
    for (size_t j = 0; j < BlockCipher::blocksize; j++)
        dst[j] = a[j] ^ b[j];
}

template<class BlockCipher>
void
cbc_encrypt(const BlockCipher *c,
            const std::vector<uint8_t> &ivec,
            const std::vector<uint8_t> &ptext,
            std::vector<uint8_t> *ctext)
{
    assert(ivec.size() == BlockCipher::blocksize);
    assert(ptext.size() % BlockCipher::blocksize == 0);
    ctext->resize(ptext.size());

    const uint8_t *iv = &ivec[0];
    const uint8_t *pt = &ptext[0];
    uint8_t *ct = &(*ctext)[0];

    for (size_t i = 0; i < ptext.size(); i += BlockCipher::blocksize) {
        uint8_t x[BlockCipher::blocksize];
        const uint8_t *y = (i == 0) ? &iv[0] : &ct[i-BlockCipher::blocksize];
        xor_block<BlockCipher>(x, &pt[i], y);
        c->block_encrypt(x, &ct[i]);
    }
}

template<class BlockCipher>
void
cbc_decrypt(const BlockCipher *c,
            const std::vector<uint8_t> &ivec,
            const std::vector<uint8_t> &ctext,
            std::vector<uint8_t> *ptext)
{
    assert(ivec.size() == BlockCipher::blocksize);
    assert(ctext.size() % BlockCipher::blocksize == 0);
    ptext->resize(ctext.size());

    const uint8_t *iv = &ivec[0];
    const uint8_t *ct = &ctext[0];
    uint8_t *pt = &(*ptext)[0];

    for (size_t i = 0; i < ctext.size(); i += BlockCipher::blocksize) {
        uint8_t x[BlockCipher::blocksize];
        c->block_decrypt(&ct[i], x);
        const uint8_t *y = (i == 0) ? &iv[0] : &ct[i-BlockCipher::blocksize];
        xor_block<BlockCipher>(&pt[i], x, y);
    }
}
