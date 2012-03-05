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

static CryptoManager cm("12345");
static crypto_manager_stub stub(&cm, false);

static void test_big_ope(uint64_t a, uint64_t b, bool test_decrypt = true)
{
    bool isBin = false;
    uint64_t i = a;
    string ct = stub.crypt<8>(cm.getmkey(), to_s(i), TYPE_INTEGER,
                           "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                           isBin, 12345);
    assert(isBin);
    assert(ct.size() <= 16);

    if (test_decrypt) {
        string pt = stub.crypt<8>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(!isBin);
        //cerr << "i: " << i << ", pt: " << pt << endl;
        assert(to_s(i) == pt);
    }

    uint64_t i1 = b;
    string ct1 = stub.crypt<8>(cm.getmkey(), to_s(i1), TYPE_INTEGER,
                           "my_field_name", SECLEVEL::PLAIN_OPE, SECLEVEL::OPE,
                           isBin, 12345);
    assert(isBin);
    assert(ct1.size() <= 16);

    if (test_decrypt) {
        string pt1 = stub.crypt<8>(cm.getmkey(), ct1, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::OPE, SECLEVEL::PLAIN_OPE,
                               isBin, 12345);
        assert(!isBin);
        assert(to_s(i1) == pt1);
    }

    // test OPE properties

    // 1) pad
    ct.resize(16); ct1.resize(16);

    // 2) reverse
    ct = str_reverse(ct); ct1 = str_reverse(ct1);

    // 3) check OPE properties

    assert((i < i1) == (memcmp(ct.data(), ct1.data(), 16) < 0));
}

int main(int argc, char **argv) {
    // det tests

    // test int type
    {
        bool isBin = false;
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
        bool isBin = false;
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
        bool isBin = false;
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
        bool isBin = false;
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
        bool isBin = false;
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

    // test big int type
    {
        test_big_ope(99999999999999UL, 88888888888888UL);
        test_big_ope(88888888888888UL, 99999999999999UL);
        test_big_ope(12234542345UL, 823982315UL);
        test_big_ope(13932UL, 1UL);
        test_big_ope(9084234534UL, 9023859023UL);
        test_big_ope(1UL, 2UL);
        test_big_ope(18328954UL, 289345808UL);
        test_big_ope(9085908UL, 23285230346UL);
        test_big_ope(123UL, 123UL);
    }

    // test char type
    {
        bool isBin = false;
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
        bool isBin = false;
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
