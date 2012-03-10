#include <algorithm>
#include <assert.h>
#include <crypto/mont.hh>

#include <gmp.h>
#include <util/static_assert.hh>

using namespace std;
using namespace NTL;

ZZ
montgomery::to_mont(const ZZ &a)
{
    return MulMod(a, _r, _m);
}

ZZ
montgomery::from_mont(const ZZ &a)
{
    return MulMod(a, _rinv, _m);
}

#define SIZE(p) (((long *) (p))[1])
#define DATA(p) ((mp_limb_t *) (((long *) (p)) + 2))

ZZ
montgomery::mmul(const ZZ &a, const ZZ &b)
{
    _static_assert( sizeof(mp_limb_t) == sizeof(long) );
    ZZ ab = a * b;

    for (uint i = 0; i < _mbits; i += sizeof(long) * 8) {
        uint thisbits = std::min((uint) sizeof(long) * 8, _mbits - i);
        long l = trunc_long(ab, thisbits);  // bits to cancel out
        long c = _minusm_inv_modr * l & (~(long)0 >> (sizeof(long)-thisbits));
        ab += _m * c;

        // assert(trunc_long(ab, thisbits) == 0);
        ab = ab >> thisbits;
    }

    while (ab >= _m)
        ab = ab - _m;
    return ab;
}
