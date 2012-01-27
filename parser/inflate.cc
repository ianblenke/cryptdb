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

enum lineitem {
    l_orderkey,
    l_partkey,
    l_suppkey,
    l_linenumber,
    l_quantity,
    l_extendedprice,
    l_discount,
    l_tax,
    l_returnflag,
    l_linestatus,
    l_shipdate,
    l_commitdate,
    l_receiptdate,
    l_shipinstruct,
    l_shipmode,
    l_comment,
    l_disc_price,
    l_charge
};

static void process_row(CryptoManager& cm, const vector<string>& elems) {
    crypto_manager_stub cm_stub(&cm);

    vector<string> newelems = elems;

    bool isBin;

    // we will add 3000000 to l_orderkey_DET
    string decDET = cm_stub.crypt<4>(cm.getmkey(), elems[8], TYPE_INTEGER,
                         fieldname(lineitem::l_orderkey, "DET"),
                         SECLEVEL::DETJOIN, SECLEVEL::PLAIN_DET, isBin, 12345);

    string encDET = cm_stub.crypt<4>(cm.getmkey(), to_s(resultFromStr<uint64_t>(decDET) + 6000000),
                         TYPE_INTEGER, fieldname(lineitem::l_orderkey, "DET"),
                         SECLEVEL::PLAIN_DET, SECLEVEL::DETJOIN, isBin, 12345);

    newelems[8] = encDET;

    // re-escape strings
    for (size_t i = 0; i < newelems.size(); i++) {
        newelems[i] = to_mysql_escape_varbin(newelems[i]);
    }

    cout << join(newelems, "|") << endl;
}

int main(int argc, char** argv) {

    char buf[4096];
    vector<string> elems;
    string cur_elem;
    CryptoManager cm("12345");
    bool onescape = false;
    while (cin.good()) {
        cin.read(buf, sizeof(buf));
        for (streamsize i = 0; i < cin.gcount(); i++) {
            if (onescape) {
                cur_elem.push_back(buf[i]);
                onescape = false;
                continue;
            }
            switch (buf[i]) {
            case '\\':
                onescape = true;
                break;
            case '|':
                elems.push_back(cur_elem);
                cur_elem.clear();
                break;
            case '\n':
                elems.push_back(cur_elem);
                cur_elem.clear();
                process_row(cm, elems);
                elems.clear();
                break;
            default:
                cur_elem.push_back(buf[i]);
                break;
            }
        }
    }
    assert(!onescape);
    return 0;
}
