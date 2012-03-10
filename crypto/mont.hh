#pragma once

#include <NTL/ZZ.h>

class montgomery {
 private:
    NTL::ZZ _m;
    uint _mbits;
    NTL::ZZ _r;
    NTL::ZZ _rinv;
    NTL::ZZ _minusm_inv_modr;

 public:
    montgomery(const NTL::ZZ &m)
        : _m(m), _mbits(NumBits(m)), _r(NTL::to_ZZ(1) << _mbits),
          _rinv(InvMod(_r % _m, _m)), _minusm_inv_modr(InvMod(_r-_m, _r)) {}

    NTL::ZZ to_mont(const NTL::ZZ &a);
    NTL::ZZ from_mont(const NTL::ZZ &a);
    NTL::ZZ mmul(const NTL::ZZ &a, const NTL::ZZ &b);
};
