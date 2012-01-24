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

#include <parser/cdb_rewrite.hh>
#include <parser/cdb_helpers.hh>
#include <parser/encdata.hh>

#include <util/cryptdb_log.hh>

using namespace std;
using namespace NTL;

#ifdef NDEBUG
#warning "assertions are turned off- quick_test will do nothing"
#endif /* NDEBUG */

int main(int argc, char **argv) {
    CryptoManager cm("12345");
    crypto_manager_stub stub(&cm);

    bool isBin;

    // det tests

    // test int type
    {
        string ct = stub.crypt(cm.getmkey(), "34512", TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= uint64_t(uint32_t(-1)));
        string pt = stub.crypt(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);
        assert(!isBin);
        assert("34512" == pt);
    }

    // test char type
    {
        char c = 'K';
        string ct = stub.crypt<1>(cm.getmkey(), to_s(unsigned(c)), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= uint64_t(uint8_t(-1)));
        string pt = stub.crypt<1>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);

        assert(!isBin);
        uint64_t v = valFromStr(pt);
        assert(c == ((char)v));
    }

    // test date type
    {
        int encoding = encode_yyyy_mm_dd("1988-12-10");
        string ct = stub.crypt<3>(cm.getmkey(), to_s(encoding), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= max_size<3 * 8>::value);
        string pt = stub.crypt<3>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);
        assert(to_s(encoding) == pt);
    }

    // test str type
    {
        string teststr = "testing 123";
        string ct = stub.crypt(cm.getmkey(), teststr, TYPE_TEXT,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        assert(ct.size() == teststr.size());
        string pt = stub.crypt(cm.getmkey(), ct, TYPE_TEXT,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);
        assert(pt == teststr);
    }

    // ope tests

    // test int type
    {
        uint32_t i = 93254;
        string ct = stub.crypt<4>(cm.getmkey(), to_s(i), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= uint64_t(-1));
        string pt = stub.crypt<4>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(to_s(i) == pt);

        uint32_t i1 = 1193254;
        string ct1 = stub.crypt<4>(cm.getmkey(), to_s(i1), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct1) <= uint64_t(-1));
        string pt1 = stub.crypt<4>(cm.getmkey(), ct1, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(to_s(i1) == pt1);

        // test OPE properties
        assert((i < i1) == (resultFromStr<uint64_t>(ct) < resultFromStr<uint64_t>(ct1)));
    }

    // test char type
    {
        char c = 'k';
        string ct = stub.crypt<1>(cm.getmkey(), to_s(unsigned(c)), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= uint64_t(uint16_t(-1)));
        string pt = stub.crypt<1>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);

        assert(!isBin);
        uint64_t v = valFromStr(pt);
        assert(c == ((char)v));

        char c1 = 'q';
        string ct1 = stub.crypt<1>(cm.getmkey(), to_s(unsigned(c1)), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct1) <= uint64_t(uint16_t(-1)));
        string pt1 = stub.crypt<1>(cm.getmkey(), ct1, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);

        assert(!isBin);
        uint64_t v1 = valFromStr(pt1);
        assert(c1 == ((char)v1));

        assert((c < c1) == (resultFromStr<uint64_t>(ct) < resultFromStr<uint64_t>(ct1)));
    }

    // test date type
    {
        int encoding = encode_yyyy_mm_dd("1988-12-10");
        string ct = stub.crypt<3>(cm.getmkey(), to_s(encoding), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct) <= max_size<3 * 8 * 2>::value);
        string pt = stub.crypt<3>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(to_s(encoding) == pt);

        int encoding1 = encode_yyyy_mm_dd("1988-12-13");
        string ct1 = stub.crypt<3>(cm.getmkey(), to_s(encoding1), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(resultFromStr<uint64_t>(ct1) <= max_size<3 * 8 * 2>::value);
        string pt1 = stub.crypt<3>(cm.getmkey(), ct1, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(to_s(encoding1) == pt1);

        assert((encoding < encoding1) ==
               (resultFromStr<uint64_t>(ct) < resultFromStr<uint64_t>(ct1)));
    }

    return 0;
}
