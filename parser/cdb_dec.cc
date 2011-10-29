#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <list>
#include <algorithm>

#include <edb/Connect.hh>
#include <parser/cdb_rewrite.hh>

using namespace std;

static string fieldname(size_t fieldnum, const string &suffix) {
    ostringstream s;
    s << "field" << fieldnum << suffix;
    return s.str();
}

template <typename T>
inline string to_s(const T& t) {
    ostringstream s;
    s << t;
    return s.str();
}

int main(int argc, char **argv) {
    CryptoManager cm("12345");

	// l_shipdate <= date '1996-01-29'
    bool isBin;
    string encYEAR = cm.crypt(cm.getmkey(), "1996", TYPE_INTEGER,
                              fieldname(10, "year_OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encMONTH = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                               fieldname(10, "month_OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDAY = cm.crypt(cm.getmkey(), "29", TYPE_INTEGER,
                             fieldname(10, "day_OPE"),
                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345);


    Connect conn("localhost", "root", "letmein", "tpch");
    DBResult * dbres;

    auto y = valFromStr(encYEAR);
    auto m = valFromStr(encMONTH);
    auto d = valFromStr(encDAY);

    ostringstream s;
    s << "SELECT l_partkey_DET, l_partkey_SALT FROM lineitem_enc "
      << "WHERE l_shipdate_year_OPE < " << y << " "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE < " << m << " ) "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE = " << m << " AND l_shipdate_day_OPE <= " << d << " ) ";

    cout << s.str() << endl;

    conn.execute(s.str(), dbres);
    ResType res = dbres->unpack();
    assert(res.ok);
    for (auto row : res.rows) {
        string det = row[0].data;
        string salt = row[1].data;
        bool isBin;
        string res = cm.crypt(
            cm.getmkey(),
            det,
            TYPE_INTEGER,
            fieldname(1, "DET"),
            SECLEVEL::DET, getMin(oDET), isBin, valFromStr(salt)
        );
        cout << res << endl;
    }
    return 0;
}
