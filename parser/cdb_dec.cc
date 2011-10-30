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

static SECLEVEL getUsefulMax(onion o) {
    switch (o) {
    case oDET: {return SECLEVEL::DET;}
    case oOPE: {return SECLEVEL::OPE;}
    case oAGG: {return SECLEVEL::SEMANTIC_AGG;}
    case oSWP: {return SECLEVEL::SWP;}
    default: {}
    }

    assert_s(false, "invalid onion");
    return SECLEVEL::INVALID;
}

template <typename R>
R resultFromStr(const string &r) {
    stringstream ss(r);
    R val;
    ss >> val;
    return val;
}

template <>
string resultFromStr(const string &r) { return r; }

template <typename Result>
Result decryptRow(const string &data,
                  uint64_t salt,
                  const string &fieldname,
                  fieldType f,
                  onion o,
                  CryptoManager &cm) {
    bool isBin;
    string res = cm.crypt(
        cm.getmkey(),
        data,
        f,
        fieldname,
        getUsefulMax(o), getMin(o), isBin, salt);
    return resultFromStr<Result>(res);
}

static inline bool isUpperHexChar(unsigned char c) {
    return (c >= 'A' && c <= 'F') ||
           (c >= '0' && c <= '9');
}

static inline unsigned char hexToInt(unsigned char c) {
    return (c >= 'A') ? (c - 'A' + 10) : (c - '0');
}

struct lineitem_group {
    lineitem_group(
            double l_quantity,
            double l_extendedprice,
            double l_discount,
            double l_tax) :
        l_quantity(l_quantity),
        l_extendedprice(l_extendedprice),
        l_discount(l_discount),
        l_tax(l_tax) {}
    double l_quantity;
    double l_extendedprice;
    double l_discount;
    double l_tax;
};

class lineitem_key : public pair<string, string> {
public:
    lineitem_key(const string &returnflag,
                 const string &linestatus)
        : pair<string, string>(returnflag, linestatus) {}
    inline string l_returnflag() const { return first;  }
    inline string l_linestatus() const { return second; }
};

static map<lineitem_key, vector<lineitem_group> > agg_groups;

// mysql api returns binary data in form of
// X'...', where .. is the ASCII of the hex data...
//
// TODO: this function probably exists somewhere else, i can't find it though.
// so rolling my own for now
static inline string mysqlHexToBinaryData(const string &q) {
    assert(q.substr(0, 2) == "X'");
    assert(q[q.size()-1] == '\'');

    string asciiHexData = q.substr(2, q.size() - 3);
    assert((asciiHexData.size() % 2) == 0);
    ostringstream buf;
    for (size_t p = 0; p < asciiHexData.size(); p += 2) {
        assert(isUpperHexChar(asciiHexData[p]) &&
               isUpperHexChar(asciiHexData[p+1]));
        unsigned char c = 0;
        c = (hexToInt(asciiHexData[p]) << 4) |
             hexToInt(asciiHexData[p+1]);
        buf << c;
    }

    return buf.str();
}

enum f {
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
};

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
    s << "SELECT "

          << "l_returnflag_DET, l_returnflag_SALT, "
          << "l_linestatus_DET, l_linestatus_SALT, "
          << "l_quantity_DET, l_quantity_SALT, "
          << "l_extendedprice_DET, l_extendedprice_SALT, "
          << "l_discount_DET, l_discount_SALT, "
          << "l_tax_DET, l_tax_SALT "

      << "FROM lineitem_enc "
      << "WHERE l_shipdate_year_OPE < " << y << " "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE < " << m << " ) "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE = " << m << " AND l_shipdate_day_OPE <= " << d << " ) "
      << "ORDER BY l_returnflag_OPE, l_linestatus_OPE";

    cout << s.str() << endl;

    conn.execute(s.str(), dbres);
    ResType res = dbres->unpack();
    assert(res.ok);
    for (auto row : res.rows) {
        // l_returnflag
        string l_returnflag = decryptRow<string>(
                mysqlHexToBinaryData(row[0].data),
                valFromStr(row[1].data),
                fieldname(f::l_returnflag, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        // l_linestatus
        string l_linestatus = decryptRow<string>(
                mysqlHexToBinaryData(row[2].data),
                valFromStr(row[3].data),
                fieldname(f::l_linestatus, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        // l_quantity
        uint64_t l_quantity_int = decryptRow<uint64_t>(
                row[4].data,
                valFromStr(row[5].data),
                fieldname(f::l_quantity, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double l_quantity = ((double)l_quantity_int)/100.0;

        // l_extendedprice
        uint64_t l_extendedprice_int = decryptRow<uint64_t>(
                row[6].data,
                valFromStr(row[7].data),
                fieldname(f::l_extendedprice, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double l_extendedprice = ((double)l_extendedprice_int)/100.0;

        // l_discount
        uint64_t l_discount_int = decryptRow<uint64_t>(
                row[8].data,
                valFromStr(row[9].data),
                fieldname(f::l_discount, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double l_discount = ((double)l_discount_int)/100.0;

        // l_tax
        uint64_t l_tax_int = decryptRow<uint64_t>(
                row[10].data,
                valFromStr(row[11].data),
                fieldname(f::l_tax, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double l_tax = ((double)l_tax_int)/100.0;

        lineitem_key key(l_returnflag, l_linestatus);
        lineitem_group elem(l_quantity, l_extendedprice, l_discount, l_tax);

        auto it = agg_groups.find(key);
        if (it == agg_groups.end()) {
            vector<lineitem_group> v;
            v.push_back(elem);
            agg_groups[key] = v;
        } else {
            it->second.push_back(elem);
        }
    }

    for (auto it = agg_groups.begin();
         it != agg_groups.end(); ++it) {
        double sum_qty = 0.0;
        double sum_base_price = 0.0;
        double sum_disc_price = 0.0;
        double sum_charge = 0.0;
        double sum_l_discount = 0.0;

        const lineitem_key &key = it->first;

        for (auto it0 = it->second.begin();
             it0 != it->second.end();
             ++it0) {
            const lineitem_group &elem = *it0;
            sum_qty += elem.l_quantity;
            sum_base_price += elem.l_extendedprice;
            sum_disc_price += elem.l_extendedprice * (1.0 - elem.l_discount);
            sum_charge += elem.l_extendedprice * (1.0 - elem.l_discount) * (1.0 + elem.l_tax);
            sum_l_discount += elem.l_discount;
        }

        double count = (double) it->second.size();
        double avg_qty = sum_qty / count;
        double avg_price = sum_base_price / count;
        double avg_disc = sum_l_discount / count;

        uint64_t count_order = it->second.size();

        cout << key.l_returnflag() << "|"
             << key.l_linestatus() << "|"
             << sum_qty << "|"
             << sum_base_price << "|"
             << sum_disc_price << "|"
             << sum_charge << "|"
             << avg_qty << "|"
             << avg_price << "|"
             << avg_disc << "|"
             << count_order << endl;
    }

    return 0;
}
