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
    size_t ptsize = ptext.size();
    assert(ptsize >= BlockCipher::blocksize);
    ctext->resize(ptsize);

    const uint8_t *iv = &ivec[0];
    const uint8_t *pt = &ptext[0];
    uint8_t *ct = &(*ctext)[0];

    for (size_t i = 0;
         i <= ptsize - BlockCipher::blocksize;
         i += BlockCipher::blocksize)
    {
        uint8_t x[BlockCipher::blocksize];
        const uint8_t *y = (i == 0) ? &iv[0] : &ct[i-BlockCipher::blocksize];
        xor_block<BlockCipher>(x, &pt[i], y);
        c->block_encrypt(x, &ct[i]);
    }

    // Ciphertext stealing (CTS)
    size_t leftover = ptsize % BlockCipher::blocksize;
    if (leftover) {
        uint8_t x[BlockCipher::blocksize];
        const uint8_t *y = &ct[ptsize-leftover-BlockCipher::blocksize];
        uint8_t z[BlockCipher::blocksize];

        memset(z, 0, BlockCipher::blocksize);
        memcpy(z, &pt[ptsize-leftover], leftover);
        xor_block<BlockCipher>(x, z, y);

        memcpy(&ct[ptsize-leftover], &ct[ptsize-leftover-BlockCipher::blocksize], leftover);
        c->block_encrypt(x, &ct[ptsize-leftover-BlockCipher::blocksize]);
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
    size_t ctsize = ctext.size();
    assert(ctsize >= BlockCipher::blocksize);
    ptext->resize(ctsize);

    const uint8_t *iv = &ivec[0];
    const uint8_t *ct = &ctext[0];
    uint8_t *pt = &(*ptext)[0];

    for (size_t i = 0;
         i <= ctsize - BlockCipher::blocksize;
         i += BlockCipher::blocksize)
    {
        uint8_t x[BlockCipher::blocksize];
        c->block_decrypt(&ct[i], x);
        const uint8_t *y = (i == 0) ? &iv[0] : &ct[i-BlockCipher::blocksize];
        xor_block<BlockCipher>(&pt[i], x, y);
    }

    // Ciphertext stealing (CTS)
    size_t leftover = ctsize % BlockCipher::blocksize;
    if (leftover) {
        uint8_t xlast[BlockCipher::blocksize];
        c->block_decrypt(&ct[ctsize-leftover-BlockCipher::blocksize], xlast);

        uint8_t ctprev[BlockCipher::blocksize];
        memcpy(&ctprev[0], &ct[ctsize-leftover], leftover);
        memcpy(&ctprev[leftover], &xlast[leftover], BlockCipher::blocksize-leftover);

        uint8_t ptlast[BlockCipher::blocksize];
        xor_block<BlockCipher>(ptlast, xlast, ctprev);
        memcpy(&pt[ctsize-leftover], ptlast, leftover);

        uint8_t xprev[BlockCipher::blocksize];
        c->block_decrypt(ctprev, xprev);
        const uint8_t *y = (ctsize < 2*BlockCipher::blocksize) ?
                           &iv[0] :
                           &ct[ctsize-leftover-2*BlockCipher::blocksize];
        xor_block<BlockCipher>(&pt[ctsize-leftover-BlockCipher::blocksize],
                               xprev, y);
    }
}
