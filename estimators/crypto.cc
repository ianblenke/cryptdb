#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <list>
#include <algorithm>
#include <type_traits>

#include <unistd.h>

#include <estimators/agg_data.hh>
#include <crypto-old/CryptoManager.hh>

// HACK: the using namespaces must go before including
// cdb_helpers.hh
using namespace std;
using namespace NTL;

// HACK: util/util.hh must come BEFORE parser/cdb_helpers.hh
#include <util/util.hh>
#include <parser/cdb_helpers.hh>
#include <util/onions.hh>

#ifdef NDEBUG
#warning "estimator will possibly not be accurate"
#endif /* NDEBUG */

#define REPEAT(num) \
    for (size_t _i = 0; _i < (num); _i++)

#define TEST_SNIPPET(name) \
    static void name(size_t numOps, CryptoManager& cm)

TEST_SNIPPET(det_encrypt) {
    crypto_manager_stub cm_stub(&cm);
    REPEAT(numOps) {
        bool isBin;
        string encDET =
            cm_stub.crypt<4>(cm.getmkey(), "54321", TYPE_INTEGER,
                             "my_field_DET",
                             getMin(oDET), SECLEVEL::DET, isBin, 12345);
        assert(!isBin);
        assert(!encDET.empty());
    }
}

TEST_SNIPPET(det_decrypt) {
    crypto_manager_stub cm_stub(&cm);
    REPEAT(numOps) {
        bool isBin;
        string res =
            cm_stub.crypt<4>(cm.getmkey(), "1286566808", TYPE_INTEGER,
                             "my_field_DET",
                             SECLEVEL::DET, getMin(oDET), isBin, 12345);
        assert(!isBin);
        uint64_t v = resultFromStr<uint64_t>(res);
        assert(v == 54321);
    }
}

TEST_SNIPPET(ope_encrypt) {
    crypto_manager_stub cm_stub(&cm);
    REPEAT(numOps) {
        bool isBin;
        string encOPE =
            cm_stub.crypt<4>(cm.getmkey(), "54321", TYPE_INTEGER,
                             "my_field_OPE",
                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
        assert(!isBin);
        assert(!encOPE.empty());
    }
}

TEST_SNIPPET(ope_decrypt) {
    crypto_manager_stub cm_stub(&cm);
    REPEAT(numOps) {
        bool isBin;
        string res =
            cm_stub.crypt<4>(cm.getmkey(), "234387332552407", TYPE_INTEGER,
                             "my_field_OPE",
                             SECLEVEL::OPE, getMin(oOPE), isBin, 12345);
        assert(!isBin);
        uint64_t v = resultFromStr<uint64_t>(res);
        assert(v == 54321);
    }
}

TEST_SNIPPET(agg_encrypt) {
    REPEAT(numOps) {
        ZZ z = to_ZZ(12345);
        string enc = cm.encrypt_Paillier(z);
        assert(!enc.empty());
    }
}

TEST_SNIPPET(agg_decrypt) {
    REPEAT(numOps) {
        ZZ z;
        cm.decrypt_Paillier(AGG_DATA_0, z);
        long l = to_long(z);
        assert(l);
    }
}

TEST_SNIPPET(agg_add) {
    const string& a = AGG_DATA_1;
    string pkinfo = cm.getPKInfo();
    ZZ n2;
    ZZFromBytes(n2, (const uint8_t *) pkinfo.data(), pkinfo.size());

    ZZ sum = to_ZZ(0);
    REPEAT(numOps) {
        ZZ e;
        ZZFromBytes(e, (const uint8_t *) a.data(), a.size());
        MulMod(sum, sum, e, n2);
    }
}

// extern, so GCC doesn't try to pre-compute rand(), and
// copy-prop it into int_add()
extern const int32_t RAND_INT = (rand() % 10);

TEST_SNIPPET(int_add) {
    int32_t sum = 0;
    int32_t a = RAND_INT;
    REPEAT(numOps) {
        // use some inline assembly, so GCC doesn't optimize the
        // loop addition away
        __asm__ __volatile__("addl  %%ebx,%%eax"
                             :"=a"(sum)
                             :"a"(sum), "b"(a)
                             );
    }
    assert(sum == (RAND_INT * static_cast<int32_t>(numOps)));
}

static void time_snippet(
        const char *snippet_name,
        void (*fcn)(size_t, CryptoManager&),
        size_t numOps) {
    CryptoManager cm("12345");
    double tt;
    {
        Timer t;
        fcn(numOps, cm);
        tt = (t.lap_ms()) / static_cast<double>(numOps);
    }
    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(10);
    cout << snippet_name << ": " << tt << endl;
}

// this program estimates the cost of doing crypto operations. the output is a
// list of key: value pairs, where key is the operator, and value is the time
// to perform a single operation (in ms)
int main(int argc, char** argv) {

#define TIME_SNIPPET(name, numOps) \
    time_snippet(#name, name, numOps)

    { // begin DET

        TIME_SNIPPET(det_encrypt, 1000);
        TIME_SNIPPET(det_decrypt, 1000);

    } // end DET

    { // begin OPE

        TIME_SNIPPET(ope_encrypt, 50);
        TIME_SNIPPET(ope_decrypt, 50);

    } // end OPE

    { // begin AGG

        TIME_SNIPPET(agg_encrypt, 100);
        TIME_SNIPPET(agg_decrypt, 100);
        TIME_SNIPPET(agg_add,     100);

    } // end AGG

    { // begin NO-CRYPTO

        TIME_SNIPPET(int_add, 100000000);

    } // end NO-CRYPTO

    return 0;
}
