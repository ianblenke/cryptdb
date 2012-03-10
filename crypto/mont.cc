#include <algorithm>
#include <assert.h>
#include <crypto/mont.hh>

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

ZZ
montgomery::mmul(const ZZ &a, const ZZ &b)
{
    ZZ ab = a * b;

    for (uint i = 0; i < _mbits; i += sizeof(long)) {
        uint thisbits = std::min((uint) sizeof(long), _mbits - i);
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
