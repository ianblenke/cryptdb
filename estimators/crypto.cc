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

static Token* t = NULL;

static void init_static_data(CryptoManager& cm) {
    if (t) return;
    t = new Token;
    Binary key(cm.getKey(cm.getmkey(), fieldname(4, "SWP"), SECLEVEL::SWP));
    Token t0 = CryptoManager::token(key, Binary("promo"));
    t->ciph = t0.ciph;
    t->wordKey = t0.wordKey;
}

TEST_SNIPPET(det_encrypt) {
    crypto_manager_stub cm_stub(&cm, false);
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
    crypto_manager_stub cm_stub(&cm, false);
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
    crypto_manager_stub cm_stub(&cm, false);
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
    crypto_manager_stub cm_stub(&cm, false);
    REPEAT(numOps) {
        bool isBin;
        string res =
            cm_stub.crypt<4>(cm.getmkey(), "233963315384007", TYPE_INTEGER,
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
    const string* aggs[] = {
        &AGG_DATA_0,
        &AGG_DATA_1,
        &AGG_DATA_2,
        &AGG_DATA_3,
        &AGG_DATA_4,
        &AGG_DATA_5,
        &AGG_DATA_6,
        &AGG_DATA_7,
        &AGG_DATA_8,
        &AGG_DATA_9,
    };
    string pkinfo = cm.getPKInfo();
    ZZ n2;
    ZZFromBytes(n2, (const uint8_t *) pkinfo.data(), pkinfo.size());

    ZZ sum = to_ZZ(1);
    REPEAT(numOps) {
        ZZ e;
        const string& a = *aggs[_i % NELEMS(aggs)];
        ZZFromBytes(e, (const uint8_t *) a.data(), a.size());
        MulMod(sum, sum, e, n2);
    }
}

TEST_SNIPPET(swp_encrypt_for_query) {
    size_t cnt = 0;
    REPEAT(numOps) {
        Binary key(cm.getKey(cm.getmkey(), "my_field_SWP", SECLEVEL::SWP));
        Token t = CryptoManager::token(key, Binary("foobar"));
        cnt += t.ciph.len; // just so we can use Token (prevent optimization)
    }
}

TEST_SNIPPET(swp_search) {
    assert(t);
    size_t n_matches = 0;
    REPEAT(numOps) {
        Binary overallciph = Binary(SWP_DATA_0);
        if (CryptoManager::searchExists(*t, overallciph)) n_matches++;
    }
}

extern int32_t RAND_INT(void);

int32_t RAND_INT(void) {
    return (rand() % 10);
}

TEST_SNIPPET(int_add) {
    int32_t sum = 0;
    int32_t a = RAND_INT();
    REPEAT(numOps) {
        // use some inline assembly, so GCC doesn't optimize the
        // loop addition away
        __asm__ __volatile__("addl  %%ebx,%%eax"
                             :"=a"(sum)
                             :"a"(sum), "b"(a)
                             );
    }
    assert(sum == (a * static_cast<int32_t>(numOps)));
}

static CryptoManager CRYPTO_MANAGER("12345");

static void time_snippet(
        const char *snippet_name,
        void (*fcn)(size_t, CryptoManager&),
        size_t numOps) {
    double tt;
    {
        Timer t;
        fcn(numOps, CRYPTO_MANAGER);
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

    init_static_data(CRYPTO_MANAGER);

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
        TIME_SNIPPET(agg_add,     5000);

    } // end AGG

    { // begin SWP

        TIME_SNIPPET(swp_encrypt_for_query, 500);
        TIME_SNIPPET(swp_search,            500);

    } // end SWP

    { // begin NO-CRYPTO

        TIME_SNIPPET(int_add, 100000000);

    } // end NO-CRYPTO

    return 0;
}
