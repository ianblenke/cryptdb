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
    ZZ abr = a * b;

    ZZ l;   // low bits that need to be shot down
    trunc(l, abr, _mbits);

    ZZ x = l * _minusm_inv_modr;

    ZZ xlow;
    trunc(xlow, x, _mbits);
    ZZ c = _m * xlow;

    // assert((abr + c) % _r == 0);

    ZZ ab = (abr + c) >> _mbits;
    if (ab > _m)
        return ab - _m;
    else
        return ab;
}
