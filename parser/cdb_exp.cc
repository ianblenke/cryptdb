#include <cstdlib>
#include <cstdio>
#include <ctime>
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
#include <util/util.hh>

using namespace std;
using namespace NTL;

class NamedTimer : public Timer {
public:
  NamedTimer(const string &key0) : key0(key0) {}
  NamedTimer(const string &key0, const string &key1)
    : key0(key0), key1(key1) {}
  ~NamedTimer() {
    uint64_t e = lap();
    if (key1.empty()) {
      cout << key0 << " " << e << endl;
    } else {
      cout << key0 << ":" << key1 << " " << e << endl;
    }
  }
private:
  string key0;
  string key1;
};

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

template <typename Result>
Result decryptRowFromTo(const string &data,
											  uint64_t salt,
                  		  const string &fieldname,
                  		  fieldType f,
											  SECLEVEL max,
											  SECLEVEL min,
                  		  CryptoManager &cm) {
    bool isBin;
    string res = cm.crypt(
        cm.getmkey(),
        data,
        f,
        fieldname,
        max, min, isBin, salt);
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

// TODO: remove duplication
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

enum nation {
  n_nationkey,
  n_name,
  n_regionkey,
  n_comment,
};

enum partsupp {
  ps_partkey,
  ps_suppkey,
  ps_availqty,
  ps_supplycost,
  ps_comment,

  ps_value,
};

struct q1entry {
    q1entry(string l_returnflag,
            string l_linestatus,
            double sum_qty,
            double sum_base_price,
            double sum_disc_price,
            double sum_charge,
            double avg_qty,
            double avg_price,
            double avg_disc,
            uint64_t count_order) :
    l_returnflag(l_returnflag),
    l_linestatus(l_linestatus),
    sum_qty(sum_qty),
    sum_base_price(sum_base_price),
    sum_disc_price(sum_disc_price),
    sum_charge(sum_charge),
    avg_qty(avg_qty),
    avg_price(avg_price),
    avg_disc(avg_disc),
    count_order(count_order) {}

    string l_returnflag;
    string l_linestatus;
    double sum_qty;
    double sum_base_price;
    double sum_disc_price;
    double sum_charge;
    double avg_qty;
    double avg_price;
    double avg_disc;
    uint64_t count_order;
};

inline ostream& operator<<(ostream &o, const q1entry &q) {
    o << q.l_returnflag << "|"
      << q.l_linestatus << "|"
      << q.sum_qty << "|"
      << q.sum_base_price << "|"
      << q.sum_disc_price << "|"
      << q.sum_charge << "|"
      << q.avg_qty << "|"
      << q.avg_price << "|"
      << q.avg_disc << "|"
      << q.count_order;
    return o;
}

struct q11entry {
  q11entry(uint64_t ps_partkey,
           double   value) :
    ps_partkey(ps_partkey), value(value) {}
  uint64_t ps_partkey;
  double   value;
};

inline ostream& operator<<(ostream &o, const q11entry &q) {
	o << q.ps_partkey << "|" << q.value;
	return o;
}

static void do_query_orig(Connect &conn,
                          uint32_t year,
                          vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream buf;
    buf << "select SQL_NO_CACHE l_returnflag, l_linestatus, sum(l_quantity) as sum_qty, sum(l_extendedprice) as sum_base_price, sum(l_extendedprice * (1 - l_discount)) as sum_disc_price, sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) as sum_charge, avg(l_quantity) as avg_qty, avg(l_extendedprice) as avg_price, avg(l_discount) as avg_disc, count(*) as count_order from LINEITEM where l_shipdate <= date '" << year << "-1-1' group by l_returnflag, l_linestatus order by l_returnflag, l_linestatus";

    DBResult * dbres;
    {
      NamedTimer t(__func__, "execute");
      conn.execute(buf.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }
    for (auto row : res.rows) {
        const string &l_returnflag = row[0].data;
        const string &l_linestatus = row[1].data;
        double sum_qty = resultFromStr<double>(row[2].data);
        double sum_base_price = resultFromStr<double>(row[3].data);
        double sum_disc_price = resultFromStr<double>(row[4].data);
        double sum_charge = resultFromStr<double>(row[5].data);
        double avg_qty = resultFromStr<double>(row[6].data);
        double avg_price = resultFromStr<double>(row[7].data);
        double avg_disc = resultFromStr<double>(row[8].data);
        uint64_t count_order = resultFromStr<uint64_t>(row[9].data);
        results.push_back(q1entry(
            l_returnflag,
            l_linestatus,
            sum_qty,
            sum_base_price,
            sum_disc_price,
            sum_charge,
            avg_qty,
            avg_price,
            avg_disc,
            count_order));
    }
}

static inline uint64_t ExtractTimeInfo(const string &data) {
  const uint64_t *p = (const uint64_t *) data.data();
  return *p;
}

static inline string ExtractAggPayload(const string &data) {
  return data.substr(sizeof(uint64_t));
}

static void do_query_cryptdb_sum(Connect &conn,
                                 CryptoManager &cm,
                                 uint32_t year,
                                 vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encYEAR = cm.crypt(cm.getmkey(), strFromVal(year), TYPE_INTEGER,
                              fieldname(10, "year_OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encMONTH = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                               fieldname(10, "month_OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDAY = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                             fieldname(10, "day_OPE"),
                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    auto y = valFromStr(encYEAR);
    auto m = valFromStr(encMONTH);
    auto d = valFromStr(encDAY);

    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
          << "l_returnflag_DET, l_returnflag_SALT, "
          << "l_linestatus_DET, l_linestatus_SALT, "
          << "sum(l_quantity_AGG), "
          << "sum(l_extendedprice_AGG), "
          << "sum(l_disc_price_AGG), "
          << "sum(l_charge_AGG), "
          << "sum(l_discount_AGG), "
          << "count(*) "
      << "FROM lineitem_enc_opt_compact "
      << "WHERE l_shipdate_year_OPE < " << y << " "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE < " << m << " ) "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE = " << m << " AND l_shipdate_day_OPE <= " << d << " ) "
      << "GROUP BY l_returnflag_OPE, l_linestatus_OPE "
      << "ORDER BY l_returnflag_OPE, l_linestatus_OPE ";

    DBResult * dbres;
    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

}

static void do_query_cryptdb_opt(Connect &conn,
                                 CryptoManager &cm,
                                 uint32_t year,
                                 vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encYEAR = cm.crypt(cm.getmkey(), strFromVal(year), TYPE_INTEGER,
                              fieldname(10, "year_OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encMONTH = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                               fieldname(10, "month_OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDAY = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                             fieldname(10, "day_OPE"),
                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345);


    DBResult * dbres;

    auto y = valFromStr(encYEAR);
    auto m = valFromStr(encMONTH);
    auto d = valFromStr(encDAY);

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
          << "l_returnflag_DET, l_returnflag_SALT, "
          << "l_linestatus_DET, l_linestatus_SALT, "
          << "agg(l_quantity_AGG, " << pkinfo << "), "
          << "agg(l_extendedprice_AGG, " << pkinfo << "), "
          << "agg(l_disc_price_AGG, " << pkinfo << "), "
          << "agg(l_charge_AGG, " << pkinfo << "), "
          << "agg(l_discount_AGG, " << pkinfo << "), "
          << "count(*) "
      << "FROM lineitem_enc_opt "
      << "WHERE l_shipdate_year_OPE < " << y << " "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE < " << m << " ) "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE = " << m << " AND l_shipdate_day_OPE <= " << d << " ) "
      << "GROUP BY l_returnflag_OPE, l_linestatus_OPE "
      << "ORDER BY l_returnflag_OPE, l_linestatus_OPE ";

    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

    {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {
          // l_returnflag
          string l_returnflag = decryptRow<string>(
                  mysqlHexToBinaryData(row[0].data),
                  valFromStr(row[1].data),
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_TEXT,
                  oDET,
                  cm);

          // l_linestatus
          string l_linestatus = decryptRow<string>(
                  mysqlHexToBinaryData(row[2].data),
                  valFromStr(row[3].data),
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_TEXT,
                  oDET,
                  cm);

          // sum_qty
          uint64_t sum_qty_int = decryptRow<uint64_t>(
                  row[4].data,
                  12345,
                  fieldname(lineitem::l_quantity, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_qty = ((double)sum_qty_int)/100.0;

          // sum_base_price
          uint64_t sum_base_price_int = decryptRow<uint64_t>(
                  row[5].data,
                  12345,
                  fieldname(lineitem::l_extendedprice, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_base_price = ((double)sum_base_price_int)/100.0;

          // sum_disc_price
          uint64_t sum_disc_price_int = decryptRow<uint64_t>(
                  row[6].data,
                  12345,
                  fieldname(lineitem::l_disc_price, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_disc_price = ((double)sum_disc_price_int)/100.0;

          // sum_charge
          uint64_t sum_charge_int = decryptRow<uint64_t>(
                  row[7].data,
                  12345,
                  fieldname(lineitem::l_charge, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_charge = ((double)sum_charge_int)/100.0;

          // sum_discount
          uint64_t sum_discount_int = decryptRow<uint64_t>(
                  row[8].data,
                  12345,
                  fieldname(lineitem::l_discount, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_discount = ((double)sum_discount_int)/100.0;

          // count_order
          uint64_t count_order = resultFromStr<uint64_t>(row[9].data);

          double avg_qty = sum_qty / ((double)count_order);
          double avg_price = sum_base_price / ((double)count_order);
          double avg_disc = sum_discount / ((double)count_order);

          results.push_back(q1entry(
              l_returnflag,
              l_linestatus,
              sum_qty,
              sum_base_price,
              sum_disc_price,
              sum_charge,
              avg_qty,
              avg_price,
              avg_disc,
              count_order));
      }
    }
}

static void do_query_cryptdb_opt_compact_sort_key(Connect &conn,
                                                  CryptoManager &cm,
                                                  uint32_t year,
                                                  vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encYEAR = cm.crypt(cm.getmkey(), strFromVal(year), TYPE_INTEGER,
                              fieldname(10, "year_OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encMONTH = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                               fieldname(10, "month_OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDAY = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                             fieldname(10, "day_OPE"),
                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345);


    DBResult * dbres;

    auto y = valFromStr(encYEAR);
    auto m = valFromStr(encMONTH);
    auto d = valFromStr(encDAY);

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
          << "l_returnflag_DET, l_returnflag_SALT, "
          << "l_linestatus_DET, l_linestatus_SALT, "
          << "agg(l_quantity_AGG, " << pkinfo << "), "
          << "agg(l_extendedprice_AGG, " << pkinfo << "), "
          << "agg(l_disc_price_AGG, " << pkinfo << "), "
          << "agg(l_charge_AGG, " << pkinfo << "), "
          << "agg(l_discount_AGG, " << pkinfo << "), "
          << "count(*) "
      << "FROM lineitem_enc_opt_sort_key "
      << "WHERE l_shipdate_year_OPE < " << y << " "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE < " << m << " ) "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE = " << m << " AND l_shipdate_day_OPE <= " << d << " ) "
      << "GROUP BY l_returnflag_OPE, l_linestatus_OPE "
      << "ORDER BY l_returnflag_OPE, l_linestatus_OPE ";

    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

    {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {
          // l_returnflag
          unsigned char l_returnflag_ch = (unsigned char) decryptRow<uint32_t>(
                  row[0].data,
                  valFromStr(row[1].data),
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_returnflag(1, l_returnflag_ch);

          // l_linestatus
          unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t>(
                  row[2].data,
                  valFromStr(row[3].data),
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_linestatus(1, l_linestatus_ch);

          // sum_qty
          uint64_t sum_qty_int = decryptRow<uint64_t>(
                  row[4].data,
                  12345,
                  fieldname(lineitem::l_quantity, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_qty = ((double)sum_qty_int)/100.0;

          // sum_base_price
          uint64_t sum_base_price_int = decryptRow<uint64_t>(
                  row[5].data,
                  12345,
                  fieldname(lineitem::l_extendedprice, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_base_price = ((double)sum_base_price_int)/100.0;

          // sum_disc_price
          uint64_t sum_disc_price_int = decryptRow<uint64_t>(
                  row[6].data,
                  12345,
                  fieldname(lineitem::l_disc_price, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_disc_price = ((double)sum_disc_price_int)/100.0;

          // sum_charge
          uint64_t sum_charge_int = decryptRow<uint64_t>(
                  row[7].data,
                  12345,
                  fieldname(lineitem::l_charge, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_charge = ((double)sum_charge_int)/100.0;

          // sum_discount
          uint64_t sum_discount_int = decryptRow<uint64_t>(
                  row[8].data,
                  12345,
                  fieldname(lineitem::l_discount, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_discount = ((double)sum_discount_int)/100.0;

          // count_order
          uint64_t count_order = resultFromStr<uint64_t>(row[9].data);

          double avg_qty = sum_qty / ((double)count_order);
          double avg_price = sum_base_price / ((double)count_order);
          double avg_disc = sum_discount / ((double)count_order);

          results.push_back(q1entry(
              l_returnflag,
              l_linestatus,
              sum_qty,
              sum_base_price,
              sum_disc_price,
              sum_charge,
              avg_qty,
              avg_price,
              avg_disc,
              count_order));
      }
    }
}

static void do_query_cryptdb_opt_compact_table(Connect &conn,
                                               CryptoManager &cm,
                                               uint32_t year,
                                               vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encYEAR = cm.crypt(cm.getmkey(), strFromVal(year), TYPE_INTEGER,
                              fieldname(10, "year_OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encMONTH = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                               fieldname(10, "month_OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDAY = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                             fieldname(10, "day_OPE"),
                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345);


    DBResult * dbres;

    auto y = valFromStr(encYEAR);
    auto m = valFromStr(encMONTH);
    auto d = valFromStr(encDAY);

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
          << "l_returnflag_DET, l_returnflag_SALT, "
          << "l_linestatus_DET, l_linestatus_SALT, "
          << "agg(l_quantity_AGG, " << pkinfo << "), "
          << "agg(l_extendedprice_AGG, " << pkinfo << "), "
          << "agg(l_disc_price_AGG, " << pkinfo << "), "
          << "agg(l_charge_AGG, " << pkinfo << "), "
          << "agg(l_discount_AGG, " << pkinfo << "), "
          << "count(*) "
      << "FROM lineitem_enc_opt_compact "
      << "WHERE l_shipdate_year_OPE < " << y << " "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE < " << m << " ) "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE = " << m << " AND l_shipdate_day_OPE <= " << d << " ) "
      << "GROUP BY l_returnflag_OPE, l_linestatus_OPE "
      << "ORDER BY l_returnflag_OPE, l_linestatus_OPE ";

    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

    {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {
          // l_returnflag
          unsigned char l_returnflag_ch = (unsigned char) decryptRow<uint32_t>(
                  row[0].data,
                  valFromStr(row[1].data),
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_returnflag(1, l_returnflag_ch);

          // l_linestatus
          unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t>(
                  row[2].data,
                  valFromStr(row[3].data),
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_linestatus(1, l_linestatus_ch);

          // sum_qty
          uint64_t sum_qty_int = decryptRow<uint64_t>(
                  row[4].data,
                  12345,
                  fieldname(lineitem::l_quantity, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_qty = ((double)sum_qty_int)/100.0;

          // sum_base_price
          uint64_t sum_base_price_int = decryptRow<uint64_t>(
                  row[5].data,
                  12345,
                  fieldname(lineitem::l_extendedprice, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_base_price = ((double)sum_base_price_int)/100.0;

          // sum_disc_price
          uint64_t sum_disc_price_int = decryptRow<uint64_t>(
                  row[6].data,
                  12345,
                  fieldname(lineitem::l_disc_price, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_disc_price = ((double)sum_disc_price_int)/100.0;

          // sum_charge
          uint64_t sum_charge_int = decryptRow<uint64_t>(
                  row[7].data,
                  12345,
                  fieldname(lineitem::l_charge, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_charge = ((double)sum_charge_int)/100.0;

          // sum_discount
          uint64_t sum_discount_int = decryptRow<uint64_t>(
                  row[8].data,
                  12345,
                  fieldname(lineitem::l_discount, "AGG"),
                  TYPE_INTEGER,
                  oAGG,
                  cm);
          double sum_discount = ((double)sum_discount_int)/100.0;

          // count_order
          uint64_t count_order = resultFromStr<uint64_t>(row[9].data);

          double avg_qty = sum_qty / ((double)count_order);
          double avg_price = sum_base_price / ((double)count_order);
          double avg_disc = sum_discount / ((double)count_order);

          results.push_back(q1entry(
              l_returnflag,
              l_linestatus,
              sum_qty,
              sum_base_price,
              sum_disc_price,
              sum_charge,
              avg_qty,
              avg_price,
              avg_disc,
              count_order));
      }
    }
}

static long extract_from_slot(const ZZ &m, size_t slot) {
  static const size_t wordbits = sizeof(uint64_t) * 8;
  static const size_t slotbits = wordbits * 2;
  ZZ mask = to_ZZ((uint64_t)-1);
  return to_long((m >> (slotbits * slot)) & mask);
}

static void do_query_cryptdb_opt_all(Connect &conn,
                                     CryptoManager &cm,
                                     uint32_t year,
                                     vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encYEAR = cm.crypt(cm.getmkey(), strFromVal(year), TYPE_INTEGER,
                              fieldname(10, "year_OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encMONTH = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                               fieldname(10, "month_OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDAY = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                             fieldname(10, "day_OPE"),
                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345);


    DBResult * dbres;

    auto y = valFromStr(encYEAR);
    auto m = valFromStr(encMONTH);
    auto d = valFromStr(encDAY);

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
          << "l_returnflag_DET, l_returnflag_SALT, "
          << "l_linestatus_DET, l_linestatus_SALT, "
          << "agg_par(l_bitpacked_AGG, " << pkinfo << "), "
          << "count(*) "
      << "FROM lineitem_enc_opt_all "
      << "WHERE l_shipdate_year_OPE < " << y << " "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE < " << m << " ) "
      << "OR ( l_shipdate_year_OPE = " << y << " AND l_shipdate_month_OPE = " << m << " AND l_shipdate_day_OPE <= " << d << " ) "
      << "GROUP BY l_returnflag_OPE, l_linestatus_OPE "
      << "ORDER BY l_returnflag_OPE, l_linestatus_OPE ";

    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

    {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {
          // l_returnflag
          unsigned char l_returnflag_ch = (unsigned char) decryptRow<uint32_t>(
                  row[0].data,
                  valFromStr(row[1].data),
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_returnflag(1, l_returnflag_ch);

          // l_linestatus
          unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t>(
                  row[2].data,
                  valFromStr(row[3].data),
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_linestatus(1, l_linestatus_ch);

          ZZ m;
          cm.decrypt_Paillier(row[4].data, m);
          long sum_qty_int = extract_from_slot(m, 0);
          long sum_base_price_int = extract_from_slot(m, 1);
          long sum_discount_int = extract_from_slot(m, 2);
          //long sum_tax_int = extract_from_slot(m, 3);
          long sum_disc_price_int = extract_from_slot(m, 4);
          long sum_charge_int = extract_from_slot(m, 5);

          double sum_qty = ((double)sum_qty_int)/100.0;
          double sum_base_price = ((double)sum_base_price_int)/100.0;
          double sum_discount = ((double)sum_discount_int)/100.0;
          double sum_disc_price = ((double)sum_disc_price_int)/100.0;
          double sum_charge = ((double)sum_charge_int)/100.0;

          // count_order
          uint64_t count_order = resultFromStr<uint64_t>(row[5].data);

          double avg_qty = sum_qty / ((double)count_order);
          double avg_price = sum_base_price / ((double)count_order);
          double avg_disc = sum_discount / ((double)count_order);

          results.push_back(q1entry(
              l_returnflag,
              l_linestatus,
              sum_qty,
              sum_base_price,
              sum_disc_price,
              sum_charge,
              avg_qty,
              avg_price,
              avg_disc,
              count_order));
      }
    }
}

static void do_query_cryptdb(Connect &conn,
                             CryptoManager &cm,
                             uint32_t year,
                             vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encYEAR = cm.crypt(cm.getmkey(), strFromVal(year), TYPE_INTEGER,
                              fieldname(10, "year_OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encMONTH = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                               fieldname(10, "month_OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDAY = cm.crypt(cm.getmkey(), "1", TYPE_INTEGER,
                             fieldname(10, "day_OPE"),
                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345);


    DBResult * dbres;

    auto y = valFromStr(encYEAR);
    auto m = valFromStr(encMONTH);
    auto d = valFromStr(encDAY);

    ostringstream s;
    s << "SELECT SQL_NO_CACHE "

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

    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

    {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {
          // l_returnflag
          string l_returnflag = decryptRow<string>(
                  mysqlHexToBinaryData(row[0].data),
                  valFromStr(row[1].data),
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_TEXT,
                  oDET,
                  cm);

          // l_linestatus
          string l_linestatus = decryptRow<string>(
                  mysqlHexToBinaryData(row[2].data),
                  valFromStr(row[3].data),
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_TEXT,
                  oDET,
                  cm);

          // l_quantity
          uint64_t l_quantity_int = decryptRow<uint64_t>(
                  row[4].data,
                  valFromStr(row[5].data),
                  fieldname(lineitem::l_quantity, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_quantity = ((double)l_quantity_int)/100.0;

          // l_extendedprice
          uint64_t l_extendedprice_int = decryptRow<uint64_t>(
                  row[6].data,
                  valFromStr(row[7].data),
                  fieldname(lineitem::l_extendedprice, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_extendedprice = ((double)l_extendedprice_int)/100.0;

          // l_discount
          uint64_t l_discount_int = decryptRow<uint64_t>(
                  row[8].data,
                  valFromStr(row[9].data),
                  fieldname(lineitem::l_discount, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_discount = ((double)l_discount_int)/100.0;

          // l_tax
          uint64_t l_tax_int = decryptRow<uint64_t>(
                  row[10].data,
                  valFromStr(row[11].data),
                  fieldname(lineitem::l_tax, "DET"),
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
    }

    {
      NamedTimer t(__func__, "aggregation");
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

          results.push_back(q1entry(
              key.l_returnflag(),
              key.l_linestatus(),
              sum_qty,
              sum_base_price,
              sum_disc_price,
              sum_charge,
              avg_qty,
              avg_price,
              avg_disc,
              count_order));
      }
    }
}

static struct q11entry_sorter {
  inline bool operator()(const q11entry &lhs, const q11entry &rhs) const {
    return lhs.value > rhs.value;
  }
} q11entry_functor;

static void do_query_q11_noopt(Connect &conn,
                               CryptoManager &cm,
                               const string &name,
                               double fraction,
                               vector<q11entry> &results) {
    NamedTimer fcnTimer(__func__);

    bool isBin;
    string encNAME = cm.crypt(cm.getmkey(), name, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);

    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "ps_partkey_DET, ps_partkey_SALT, "
        << "ps_supplycost_DET, ps_supplycost_SALT, "
        << "ps_availqty_DET, ps_availqty_SALT "
      << "FROM partsupp_enc, supplier_enc, nation_enc "
      << "WHERE "
        << "ps_suppkey_DET = s_suppkey_DET AND "
        << "s_nationkey_DET = n_nationkey_DET AND "
        << "n_name_DET = " << marshallBinary(encNAME);
    cerr << s.str() << endl;

    DBResult * dbres;
    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

    map<uint64_t, double> aggState;
    double totalSum = 0.0;
    {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {

        // ps_partkey
        uint64_t ps_partkey = decryptRowFromTo<uint64_t>(
                row[0].data,
                valFromStr(row[1].data),
                fieldname(partsupp::ps_partkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
								getMin(oDET),
								cm);

        // ps_supplycost
        uint64_t ps_supplycost_int = decryptRow<uint64_t>(
                row[2].data,
                valFromStr(row[3].data),
                fieldname(partsupp::ps_supplycost, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double ps_supplycost = ((double)ps_supplycost_int)/100.0;

        // ps_availqty
        uint64_t ps_availqty = decryptRow<uint64_t>(
                row[4].data,
                valFromStr(row[5].data),
                fieldname(partsupp::ps_availqty, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

        double value = ps_supplycost * ((double)ps_availqty);

        auto it = aggState.find(ps_partkey);
        if (it == aggState.end()) {
          aggState[ps_partkey] = value;
        } else {
          aggState[ps_partkey] += value;
        }
        totalSum += value;
      }
    }

    {
      NamedTimer t(__func__, "agg");
      double threshold = totalSum * fraction;
      for (map<uint64_t, double>::iterator it = aggState.begin();
           it != aggState.end();
           ++it) {
        if (it->second > threshold) {
          results.push_back(q11entry(it->first, it->second));
        }
      }
      // sort
      sort(results.begin(), results.end(), q11entry_functor);
    }
}

struct q11entry_remover {
  q11entry_remover(double threshold) : threshold(threshold) {}
  inline bool operator()(const q11entry &e) const {
    return e.value <= threshold;
  }
  const double threshold;
};

static void do_query_q11(Connect &conn,
                         const string &name,
                         double fraction,
                         vector<q11entry> &results) {
    NamedTimer fcnTimer(__func__);

		ostringstream s;
		s << "select SQL_NO_CACHE ps_partkey, sum(ps_supplycost * ps_availqty) as value from PARTSUPP, SUPPLIER, NATION where ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = '" << name << "' group by ps_partkey having sum(ps_supplycost * ps_availqty) > ( select sum(ps_supplycost * ps_availqty) * " << fraction << " from PARTSUPP, SUPPLIER, NATION where ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = '" << name << "') order by value desc";

    DBResult * dbres;
    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

		for (auto row : res.rows) {
			results.push_back(
					q11entry(resultFromStr<uint64_t>(row[0].data),
									 resultFromStr<double>(row[1].data)));
		}
}

static void do_query_q11_opt(Connect &conn,
                             CryptoManager &cm,
                             const string &name,
                             double fraction,
                             vector<q11entry> &results) {
    NamedTimer fcnTimer(__func__);

    bool isBin;
    string encNAME = cm.crypt(cm.getmkey(), name, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "ps_partkey_DET, ps_partkey_SALT, "
        << "agg(ps_value_AGG, " << pkinfo << ") "
      << "FROM partsupp_enc_opt, supplier_enc_opt, nation_enc_opt "
      << "WHERE "
        << "ps_suppkey_DET = s_suppkey_DET AND "
        << "s_nationkey_DET = n_nationkey_DET AND "
        << "n_name_DET = " << valFromStr(encNAME) << " "
      << "GROUP BY ps_partkey_DET";

    DBResult * dbres;
    {
      NamedTimer t(__func__, "execute");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

    double totalSum = 0.0;
    {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {

        // ps_partkey
        uint64_t ps_partkey = decryptRowFromTo<uint64_t>(
                row[0].data,
                valFromStr(row[1].data),
                fieldname(partsupp::ps_partkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
								getMin(oDET),
								cm);

        // ps_value
        uint64_t ps_value_int = decryptRow<uint64_t>(
                row[2].data,
                12345,
                fieldname(partsupp::ps_value, "AGG"),
                TYPE_INTEGER,
                oAGG,
                cm);
        double ps_value = ((double)ps_value_int)/100.0;

        totalSum += ps_value;
        results.push_back(q11entry(ps_partkey, ps_value));
      }
    }

    {
      NamedTimer t(__func__, "agg");
      double threshold = totalSum * fraction;
      // remove elements less than threshold, stl style
      results.erase(
          remove_if(results.begin(), results.end(), q11entry_remover(threshold)),
          results.end());

      // sort
      sort(results.begin(), results.end(), q11entry_functor);
    }
}

static inline uint32_t random_year() {
    static const uint32_t gap = 1999 - 1993 + 1;
    return 1993 + (rand() % gap);

}

static void usage(char **argv) {
    cerr << "[USAGE]: " << argv[0] << " "
         << "(((--orig|--crypt|--crypt-opt|--crypt-opt-compact-sort-key|--crypt-opt-compact-table|--crypt-opt-all|--crypt-sum) year)|(--orig-query11|--crypt-query11|--crypt-opt-query11 nation fraction)) num_queries"

         << endl;
}

int main(int argc, char **argv) {
    srand(time(NULL));
    if (argc != 4 && argc != 5) {
        usage(argv);
        return 1;
    }
    if (strcmp(argv[1], "--orig") &&
        strcmp(argv[1], "--crypt") &&
        strcmp(argv[1], "--crypt-opt") &&
        strcmp(argv[1], "--crypt-opt-compact-sort-key") &&
        strcmp(argv[1], "--crypt-opt-compact-table") &&
        strcmp(argv[1], "--crypt-opt-all") &&
        strcmp(argv[1], "--crypt-sum") &&
        strcmp(argv[1], "--orig-query11") &&
        strcmp(argv[1], "--crypt-query11") &&
        strcmp(argv[1], "--crypt-opt-query11")) {
        usage(argv);
        return 1;
    }

    bool query1 =
        !strcmp(argv[1], "--orig") ||
        !strcmp(argv[1], "--crypt") ||
        !strcmp(argv[1], "--crypt-opt") ||
        !strcmp(argv[1], "--crypt-opt-compact-sort-key") ||
        !strcmp(argv[1], "--crypt-opt-compact-table") ||
        !strcmp(argv[1], "--crypt-opt-all") ||
        !strcmp(argv[1], "--crypt-sum");

    Connect conn("localhost", "root", "sam15theboss", "tpch");
    int input_nruns = atoi(argv[query1 ? 3 : 4]);
    uint32_t nruns = (uint32_t) input_nruns;

    unsigned long ctr = 0;
    CryptoManager cm("12345");

    if (query1) {
      int input_year = atoi(argv[2]);
			if (input_year < 0 || input_nruns < 0) {
				usage(argv);
				return 1;
			}
      uint32_t year = (uint32_t) input_year;
      vector<q1entry> results;
      if (!strcmp(argv[1], "--orig")) {
          // vanilla MYSQL case
          for (size_t i = 0; i < nruns; i++) {
              do_query_orig(conn, year, results);
              ctr += results.size();
              //for (auto r : results) {
              //    cout << r << endl;
              //}
              //cout << "------" << endl;
              results.clear();
          }
      } else {
          // crypt case
          if (!strcmp(argv[1], "--crypt")) {
              for (size_t i = 0; i < nruns; i++) {
                  do_query_cryptdb(conn, cm, year, results);
                  ctr += results.size();
                  //for (auto r : results) {
                  //    cout << r << endl;
                  //}
                  //cout << "------" << endl;
                  results.clear();
              }
          } else if (!strcmp(argv[1], "--crypt-opt")) {
              for (size_t i = 0; i < nruns; i++) {
                  do_query_cryptdb_opt(conn, cm, year, results);
                  ctr += results.size();
                  //for (auto r : results) {
                  //    cout << r << endl;
                  //}
                  //cout << "------" << endl;
                  results.clear();
              }
          } else if (!strcmp(argv[1], "--crypt-opt-compact-sort-key")) {
              for (size_t i = 0; i < nruns; i++) {
                  do_query_cryptdb_opt_compact_sort_key(conn, cm, year, results);
                  ctr += results.size();
                  //for (auto r : results) {
                  //    cout << r << endl;
                  //}
                  //cout << "------" << endl;
                  results.clear();
              }
          } else if (!strcmp(argv[1], "--crypt-opt-compact-table")) {
              for (size_t i = 0; i < nruns; i++) {
                  do_query_cryptdb_opt_compact_table(conn, cm, year, results);
                  ctr += results.size();
                  //for (auto r : results) {
                  //    cout << r << endl;
                  //}
                  //cout << "------" << endl;
                  results.clear();
              }
          } else if (!strcmp(argv[1], "--crypt-opt-all")) {
              for (size_t i = 0; i < nruns; i++) {
                  do_query_cryptdb_opt_all(conn, cm, year, results);
                  ctr += results.size();
                  //for (auto r : results) {
                  //    cout << r << endl;
                  //}
                  //cout << "------" << endl;
                  results.clear();
              }
          } else if (!strcmp(argv[1], "--crypt-sum")) {
              for (size_t i = 0; i < nruns; i++) {
                  do_query_cryptdb_sum(conn, cm, year, results);
                  ctr += results.size();
                  //for (auto r : results) {
                  //    cout << r << endl;
                  //}
                  //cout << "------" << endl;
                  results.clear();
              }
          } else assert(false);
      }
    } else {
      string nation = argv[2];
      double fraction = atof(argv[3]);
      if (fraction < 0.0 || fraction > 1.0) {
        usage(argv);
        return 1;
      }
      vector<q11entry> results;
      if (!strcmp(argv[1], "--orig-query11")) {
        for (size_t i = 0; i < nruns; i++) {
					do_query_q11(conn, nation, fraction, results);
          ctr += results.size();
          for (auto r : results) {
              cout << r << endl;
          }
          cout << "------" << endl;
          results.clear();
        }
      } else if (!strcmp(argv[1], "--crypt-query11")) {
        for (size_t i = 0; i < nruns; i++) {
          do_query_q11_noopt(conn, cm, nation, fraction, results);
          ctr += results.size();
          for (auto r : results) {
              cout << r << endl;
          }
          cout << "------" << endl;
          results.clear();
        }
      } else if (!strcmp(argv[1], "--crypt-opt-query11")) {
        for (size_t i = 0; i < nruns; i++) {
          do_query_q11_opt(conn, cm, nation, fraction, results);
          ctr += results.size();
          for (auto r : results) {
              cout << r << endl;
          }
          cout << "------" << endl;
          results.clear();
        }
			} else assert(false);
    }
    cerr << ctr << endl;
    return 0;
}
