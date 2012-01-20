#pragma once

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <vector>

class skip32 {
 public:
    skip32(const std::vector<uint8_t> &k) {
        assert(k.size() == 10);
        key = k;
    }

    void block_encrypt(const uint8_t *ptext, uint8_t *ctext) const {
        memcpy(ctext, ptext, 4);
        process(ctext, 1);
    }

    void block_decrypt(const uint8_t *ctext, uint8_t *ptext) const {
        memcpy(ptext, ctext, 4);
        process(ptext, 0);
    }

    static const size_t blocksize = 4;

 private:
    void process(uint8_t *data, int encrypt) const;

    std::vector<uint8_t> key;
};
