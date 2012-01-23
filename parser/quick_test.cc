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



int main(int argc, char **argv) {
    CryptoManager cm("12345");
    crypto_manager_stub stub(&cm);

    bool isBin;

    // test int type
    {
        string ct = stub.crypt(cm.getmkey(), "34512", TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        string pt = stub.crypt(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);
        assert("34512" == pt);
    }

    // test char type
    {
        char c = 'K';
        string ct = stub.crypt<1>(cm.getmkey(), to_s(unsigned(c)), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        string pt = stub.crypt<1>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);

        uint64_t v = valFromStr(pt);
        assert(c == ((char)v));
    }

    // test date type
    {
        int encoding = encode_yyyy_mm_dd("1988-12-10");
        string ct = stub.crypt<3>(cm.getmkey(), to_s(encoding), TYPE_INTEGER,
                               "my_field_name", SECLEVEL::PLAIN_DET, SECLEVEL::DET,
                               isBin, 12345);
        string pt = stub.crypt<3>(cm.getmkey(), ct, TYPE_INTEGER,
                               "my_field_name", SECLEVEL::DET, SECLEVEL::PLAIN_DET,
                               isBin, 12345);
        assert(to_s(encoding) == pt);
    }

    return 0;
}
