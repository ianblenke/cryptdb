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
#include <parser/cdb_helpers.hh>
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

template <typename T, size_t WordsPerSlot>
struct PallierSlotManager {
public:
  static const size_t WordBits = sizeof(T) * 8;
  static const size_t SlotBits = WordBits * WordsPerSlot;
  static const size_t TotalBits = CryptoManager::Paillier_len_bits / 2;
  static const size_t NumSlots = TotalBits / SlotBits;

  static ZZ ZZMask;

  /** assumes slot currently un-occupied */
  static void insert_into_slot(ZZ &z, T value, size_t slot) {
    assert(slot < NumSlots);
    z |= (to_ZZ(value) << (SlotBits * slot));
  }

  template <typename R>
  static R extract_from_slot(const ZZ &z, size_t slot) {
    assert(slot < NumSlots);
    return (R) to_long((z >> (SlotBits * slot)) & ZZMask);
  }

private:
  static inline ZZ MakeSlotMask() {
    static const typename make_unsigned<T>::type PrimitiveMask = -1;
    ZZ mask = to_ZZ(PrimitiveMask);
    for (size_t i = 1; i < WordsPerSlot; i++) {
      mask |= (to_ZZ(PrimitiveMask) << (WordBits * i));
    }
    assert((size_t)NumBits(mask) == SlotBits);
    return mask;
  }
};

template <typename T, size_t WordsPerSlot>
ZZ PallierSlotManager<T, WordsPerSlot>::ZZMask = MakeSlotMask();

static inline string lower_s(const string &s) {
  string ret(s);
  // why isn't there a string.to_lower()?
  transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
  return ret;
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

template <typename Result, size_t n_bytes = 4>
Result decryptRow(const string &data,
                  uint64_t salt,
                  const string &fieldname,
                  fieldType f,
                  onion o,
                  CryptoManager &cm) {
    crypto_manager_stub cm_stub(&cm);
    bool isBin;
    string res = cm_stub.crypt<n_bytes>(
        cm.getmkey(),
        data,
        f,
        fieldname,
        getUsefulMax(o), getMin(o), isBin, salt);
    return resultFromStr<Result>(res);
}

template <typename Result, size_t n_bytes = 4>
Result decryptRowFromTo(const string &data,
                        uint64_t salt,
                        const string &fieldname,
                        fieldType f,
                        SECLEVEL max,
                        SECLEVEL min,
                        CryptoManager &cm) {
    crypto_manager_stub cm_stub(&cm);
    bool isBin;
    string res = cm_stub.crypt<n_bytes>(
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

enum part {
  p_partkey     ,
  p_name        ,
  p_mfgr        ,
  p_brand       ,
  p_type        ,
  p_size        ,
  p_container   ,
  p_retailprice ,
  p_comment     ,
};

enum region {
  r_regionkey,
  r_name,
  r_comment,
};

enum supplier {
  s_suppkey,
  s_name,
  s_address,
  s_nationkey,
  s_phone,
  s_acctbal,
  s_comment,
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

struct q2entry {
  q2entry(
    double s_acctbal,
    const string& s_name,
    const string& n_name,
    uint64_t p_partkey,
    const string& p_mfgr,
    const string& s_address,
    const string& s_phone,
    const string& s_comment) :

  s_acctbal(s_acctbal),
  s_name(s_name),
  n_name(n_name),
  p_partkey(p_partkey),
  p_mfgr(p_mfgr),
  s_address(s_address),
  s_phone(s_phone),
  s_comment(s_comment) {}

  double s_acctbal;
  string s_name;
  string n_name;
  uint64_t p_partkey;
  string p_mfgr;
  string s_address;
  string s_phone;
  string s_comment;
};

inline ostream& operator<<(ostream &o, const q2entry &q) {
  o << q.s_acctbal << "|"
    << q.s_name << "|"
    << q.n_name << "|"
    << q.p_partkey << "|"
    << q.p_mfgr << "|"
    << q.s_address << "|"
    << q.s_phone << "|"
    << q.s_comment;
  return o;
}

typedef double q14entry;

static void do_query_q1(Connect &conn,
                        uint32_t year,
                        vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream buf;
    buf << "select SQL_NO_CACHE l_returnflag, l_linestatus, sum(l_quantity) as sum_qty, sum(l_extendedprice) as sum_base_price, sum(l_extendedprice * (1 - l_discount)) as sum_disc_price, sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) as sum_charge, avg(l_quantity) as avg_qty, avg(l_extendedprice) as avg_price, avg(l_discount) as avg_disc, count(*) as count_order from LINEITEM "

        << "where l_shipdate <= date '" << year << "-1-1' "
        << "group by l_returnflag, l_linestatus order by l_returnflag, l_linestatus"

        ;
    cerr << buf.str() << endl;

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

static long extract_from_slot(const ZZ &m, size_t slot) {
  static const size_t wordbits = sizeof(uint64_t) * 8;
  static const size_t slotbits = wordbits * 2;
  ZZ mask = to_ZZ((uint64_t)-1);
  return to_long((m >> (slotbits * slot)) & mask);
}

static inline uint32_t EncodeDate(uint32_t month, uint32_t day, uint32_t year) {
  return day | (month << 5) | (year << 9);
}

static inline void ExtractDate(uint32_t encoding,
                               uint32_t &month,
                               uint32_t &day,
                               uint32_t &year) {
  static const uint32_t DayMask = 0x1F;
  static const uint32_t MonthMask = 0x1E0;
  static const uint32_t YearMask = ((uint32_t)-1) << 9;

  day = encoding & DayMask;
  month = (encoding & MonthMask) >> 5;
  year = (encoding & YearMask) >> 9;
}

static void do_query_q1_packed_opt(Connect &conn,
                                   CryptoManager &cm,
                                   uint32_t year,
                                   vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    // do aggregation per group on all entries with enddate <= date.
    // for intervals with start <= date < end, then we need to collect the
    // pkey entries here, and query them separately.

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "l_returnflag_DET, "
        << "l_linestatus_DET, "

        << "agg(IF(l_shipdate_OPE_end <= " << encDATE << ", z1, NULL), " << pkinfo << "), "
        << "agg(IF(l_shipdate_OPE_end <= " << encDATE << ", z2, NULL), " << pkinfo << "), "
        << "agg(IF(l_shipdate_OPE_end <= " << encDATE << ", z3, NULL), " << pkinfo << "), "
        << "agg(IF(l_shipdate_OPE_end <= " << encDATE << ", z4, NULL), " << pkinfo << "), "
        << "agg(IF(l_shipdate_OPE_end <= " << encDATE << ", z5, NULL), " << pkinfo << "), "

        << "sum(IF(l_shipdate_OPE_end <= " << encDATE << ", packed_count, 0)), "

        << "GROUP_CONCAT("
          << "IF(l_shipdate_OPE_start <= " << encDATE << " AND " << encDATE << " < l_shipdate_OPE_end, "
          << "CONCAT_WS(',',";

    for (size_t i = 1; i <= 16; i++) {
      s << "IF(l_orderkey_DET_" << i << " IS NULL, NULL, CONCAT_WS('|', l_orderkey_DET_" << i << ", l_orderkey_DET_" << i << "))";
      if (i != 16) s << ",";
    }

    s << "), NULL)) FROM lineitem_packed_enc GROUP BY l_returnflag_DET, l_linestatus_DET";
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

    {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {
          // l_returnflag
          unsigned char l_returnflag_ch = (unsigned char) decryptRow<uint32_t>(
                  row[0].data,
                  12345,
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_returnflag(1, l_returnflag_ch);

          // l_linestatus
          unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t>(
                  row[1].data,
                  12345,
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_linestatus(1, l_linestatus_ch);

          ZZ z1, z2, z3, z4, z5;
          cm.decrypt_Paillier(row[2].data, z1);
          cm.decrypt_Paillier(row[3].data, z2);
          cm.decrypt_Paillier(row[4].data, z3);
          cm.decrypt_Paillier(row[5].data, z4);
          cm.decrypt_Paillier(row[6].data, z5);

          typedef PallierSlotManager<uint32_t, 2> PSM;
          uint64_t sum_qty_int = 0;
          uint64_t sum_base_price_int = 0;
          uint64_t sum_discount_int = 0;
          uint64_t sum_disc_price_int = 0;
          uint64_t sum_charge_int = 0;

          for (size_t i = 0; i < PSM::NumSlots; i++) {
            sum_qty_int += PSM::extract_from_slot<uint64_t>(z1, i);
            sum_base_price_int += PSM::extract_from_slot<uint64_t>(z2, i);
            sum_discount_int += PSM::extract_from_slot<uint64_t>(z3, i);
            sum_disc_price_int += PSM::extract_from_slot<uint64_t>(z4, i);
            sum_charge_int += PSM::extract_from_slot<uint64_t>(z5, i);
          }

          // count_order
          uint64_t count_order = resultFromStr<uint64_t>(row[7].data);

          double sum_qty = ((double)sum_qty_int)/100.0;
          double sum_base_price = ((double)sum_base_price_int)/100.0;
          double sum_discount = ((double)sum_discount_int)/100.0;
          double sum_disc_price = ((double)sum_disc_price_int)/100.0;
          double sum_charge = ((double)sum_charge_int)/100.0;

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

static void do_query_q1_opt(Connect &conn,
                            CryptoManager &cm,
                            uint32_t year,
                            vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    DBResult * dbres;

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
          << "l_returnflag_DET, "
          << "l_linestatus_DET, "
          //<< "agg(l_bitpacked_AGG, " << pkinfo << "), "
          << "agg8(l_bitpacked_AGG_0, l_bitpacked_AGG_1, l_bitpacked_AGG_2, l_bitpacked_AGG_3, "
               << "l_bitpacked_AGG_4, l_bitpacked_AGG_5, l_bitpacked_AGG_6, l_bitpacked_AGG_7, " << pkinfo << "), "
          << "count(*) "
      << "FROM lineitem_enc "
      << "WHERE l_shipdate_OPE <= " << encDATE << " "
      << "GROUP BY l_returnflag_OPE, l_linestatus_OPE "
      << "ORDER BY l_returnflag_OPE, l_linestatus_OPE"
      ;
    cerr << s.str() << endl;

    // attempt 1
    //
    //s << "SELECT SQL_NO_CACHE r1.l_returnflag_DET, r1.l_linestatus_DET, agg(r2.l_bitpacked_AGG, " << pkinfo << "), count(*) FROM ( SELECT l_returnflag_DET, l_linestatus_DET, l_returnflag_OPE, l_linestatus_OPE, agg_ptr FROM lineitem_enc_2_proj WHERE l_shipdate_OPE < " << encDATE << " ) AS r1 JOIN lineitem_enc_agg_proj AS r2 ON r1.agg_ptr = r2.id GROUP BY r1.l_returnflag_OPE, r1.l_linestatus_OPE ORDER BY r1.l_returnflag_OPE, r1.l_linestatus_OPE";

    //s << "SELECT SQL_NO_CACHE r1.l_returnflag_DET, r1.l_linestatus_DET, sum(length(r2.l_bitpacked_AGG)), count(*) FROM ( SELECT l_returnflag_DET, l_linestatus_DET, l_returnflag_OPE, l_linestatus_OPE, agg_ptr FROM lineitem_enc_2_proj WHERE l_shipdate_OPE < " << encDATE << " ) AS r1 JOIN lineitem_enc_agg_proj AS r2 ON r1.agg_ptr = r2.id GROUP BY r1.l_returnflag_OPE, r1.l_linestatus_OPE ORDER BY r1.l_returnflag_OPE, r1.l_linestatus_OPE";

    //s << "SELECT SQL_NO_CACHE r1.l_returnflag_DET, r1.l_linestatus_DET, agg(r2.l_bitpacked_AGG, " << pkinfo << "), count(*) FROM lineitem_enc_2_proj AS r1 JOIN lineitem_enc_agg_proj AS r2 ON r1.agg_ptr = r2.id WHERE r1.l_shipdate_OPE < " << encDATE << " GROUP BY r1.l_returnflag_OPE, r1.l_linestatus_OPE ORDER BY r1.l_returnflag_OPE, r1.l_linestatus_OPE";

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
          unsigned char l_returnflag_ch = (unsigned char) decryptRow<uint32_t, 1>(
                  row[0].data,
                  12345,
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_returnflag(1, l_returnflag_ch);

          // l_linestatus
          unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t, 1>(
                  row[1].data,
                  12345,
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_linestatus(1, l_linestatus_ch);

          ZZ m;
          cm.decrypt_Paillier(row[2].data, m);
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
          uint64_t count_order = resultFromStr<uint64_t>(row[3].data);

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

static void do_query_q1_noopt(Connect &conn,
                              CryptoManager &cm,
                              uint32_t year,
                              vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);


    DBResult * dbres;

    ostringstream s;
    s << "SELECT SQL_NO_CACHE "

          << "l_returnflag_DET, "
          << "l_linestatus_DET, "
          << "l_quantity_DET, "
          << "l_extendedprice_DET, "
          << "l_discount_DET, "
          << "l_tax_DET "

      << "FROM lineitem_enc "
      << "WHERE l_shipdate_OPE < " << encDATE;

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
                  12345,
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_returnflag(1, l_returnflag_ch);

          // l_linestatus
          unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t>(
                  row[1].data,
                  12345,
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          string l_linestatus(1, l_linestatus_ch);

          // l_quantity
          uint64_t l_quantity_int = decryptRow<uint64_t>(
                  row[2].data,
                  12345,
                  fieldname(lineitem::l_quantity, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_quantity = ((double)l_quantity_int)/100.0;

          // l_extendedprice
          uint64_t l_extendedprice_int = decryptRow<uint64_t>(
                  row[3].data,
                  12345,
                  fieldname(lineitem::l_extendedprice, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_extendedprice = ((double)l_extendedprice_int)/100.0;

          // l_discount
          uint64_t l_discount_int = decryptRow<uint64_t>(
                  row[4].data,
                  12345,
                  fieldname(lineitem::l_discount, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_discount = ((double)l_discount_int)/100.0;

          // l_tax
          uint64_t l_tax_int = decryptRow<uint64_t>(
                  row[5].data,
                  12345,
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
    crypto_manager_stub cm_stub(&cm);
    NamedTimer fcnTimer(__func__);

    bool isBin;
    string encNAME = cm_stub.crypt(cm.getmkey(), name, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);

    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "ps_partkey_DET, "
        << "ps_supplycost_DET, "
        << "ps_availqty_DET "
      << "FROM partsupp_enc, supplier_enc, nation_enc "
      << "WHERE "
        << "ps_suppkey_DET = s_suppkey_DET AND "
        << "s_nationkey_DET = n_nationkey_DET AND "
        << "n_name_DET = " << marshallBinary(encNAME);

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
                12345,
                fieldname(partsupp::ps_partkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
                getMin(oDET),
                cm);

        // ps_supplycost
        uint64_t ps_supplycost_int = decryptRow<uint64_t>(
                row[1].data,
                12345,
                fieldname(partsupp::ps_supplycost, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double ps_supplycost = ((double)ps_supplycost_int)/100.0;

        // ps_availqty
        uint64_t ps_availqty = decryptRow<uint64_t>(
                row[2].data,
                12345,
                fieldname(partsupp::ps_availqty, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

        double value = ps_supplycost * ((double)ps_availqty);

        auto it = aggState.find(ps_partkey);
        if (it == aggState.end()) {
          aggState[ps_partkey] = value;
        } else {
          it->second += value;
        }
        totalSum += value;
      }
    }

    {
      NamedTimer t(__func__, "agg");
      double threshold = totalSum * fraction;
      results.reserve(aggState.size());
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
    s << "select SQL_NO_CACHE ps_partkey, sum(ps_supplycost * ps_availqty) as value "
      << "from PARTSUPP, SUPPLIER, NATION "
      << "where ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = '" << name << "' "
      << "group by ps_partkey having sum(ps_supplycost * ps_availqty) > ("
        << "select sum(ps_supplycost * ps_availqty) * " << fraction << " "
        << "from PARTSUPP, SUPPLIER, NATION "
        << "where ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = '" << name << "') "
      << "order by value desc";

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

static void do_query_q14_opt(Connect &conn,
                             CryptoManager &cm,
                             uint32_t year,
                             vector<q14entry> &results) {
    crypto_manager_stub cm_stub(&cm);
    NamedTimer fcnTimer(__func__);

    bool isBin;
    string encDateLower = cm_stub.crypt(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                                   TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDateUpper = cm_stub.crypt(cm.getmkey(), strFromVal(EncodeDate(2, 1, year)),
                                   TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    Binary key(cm.getKey(cm.getmkey(), fieldname(part::p_type, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary("promo"));

    ZZ zero(to_ZZ(0));
    string encZERO = cm.encrypt_Paillier(zero);

    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "agg(CASE WHEN "
          << "searchSWP("
            << marshallBinary(string((char *)t.ciph.content, t.ciph.len))
            << ", "
            << marshallBinary(string((char *)t.wordKey.content, t.wordKey.len))
            << ", p_type_SWP) = 1 "
          << "THEN l_bitpacked_AGG ELSE " << marshallBinary(encZERO) << " END), "
        << "agg(l_bitpacked_AGG) "
      << "FROM lineitem_enc, part_enc "
      << "WHERE "
        << "l_partkey_DET = p_partkey_DET AND "
        << "l_shipdate_OPE >= " << encDateLower << " AND "
        << "l_shipdate_OPE < " << encDateUpper;

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
      ZZ m0, m1;
      cm.decrypt_Paillier(row[0].data, m0);
      cm.decrypt_Paillier(row[1].data, m1);

      long sum_disc_price_int_top = extract_from_slot(m0, 4);
      long sum_disc_price_int_bot = extract_from_slot(m1, 4);

      double sum_disc_price_top = ((double)sum_disc_price_int_top)/100.0;
      double sum_disc_price_bot = ((double)sum_disc_price_int_bot)/100.0;

      double result = 100.0 * (sum_disc_price_top / sum_disc_price_bot);
      results.push_back(result);
    }
}

static void do_query_q14(Connect &conn,
                         uint32_t year,
                         vector<q14entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s << "select 100.00 * sum(case when p_type like 'PROMO%' then l_extendedprice * (1 - l_discount) else 0 end) / sum(l_extendedprice * (1 - l_discount)) as promo_revenue from LINEITEM, PART where l_partkey = p_partkey and l_shipdate >= date '" << year << "-01-01' and l_shipdate < date '" << year << "-01-01' + interval '1' month";

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
      results.push_back(resultFromStr<double>(row[0].data));
    }
}

static inline long roundToLong(double x) {
    return ((x)>=0?(long)((x)+0.5):(long)((x)-0.5));
}

static void tokenize(const string &s,
                     const string &delim,
                     vector<string> &tokens) {

    size_t i = 0;
    while (true) {
        size_t p = s.find(delim, i);
        tokens.push_back(
                s.substr(
                    i, (p == string::npos ? s.size() : p) - i));
        if (p == string::npos) break;
        i = p + delim.size();
    }
}

static void do_query_q11_opt_proj(Connect &conn,
                                  CryptoManager &cm,
                                   const string &name,
                                   double fraction,
                                   vector<q11entry> &results) {
    crypto_manager_stub cm_stub(&cm);
    NamedTimer fcnTimer(__func__);
    assert(name == "IRAQ"); // TODO: can only handle IRAQ for now

    bool isBin;
    string encNAME = cm_stub.crypt(cm.getmkey(), name, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "agg(ps_value_bitpacked_AGG, " << pkinfo << ") "
      << "FROM partsupp_enc_proj, nation_enc "
      << "WHERE "
        << "n_nationkey_DET = nationkey_DET AND "
        << "n_name_DET = " << marshallBinary(encNAME);

    DBResult * dbres;
    {
      NamedTimer t(__func__, "execute_agg");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack_agg");
      res = dbres->unpack();
      assert(res.ok);
    }
    assert(res.rows.size() == 1);

    ZZ mask = to_ZZ((uint64_t)-1);
    auto row = res.rows[0];
    ZZ m;
    cm.decrypt_Paillier(row[0].data, m);
    static const size_t nslots =
      CryptoManager::Paillier_len_bytes / sizeof(uint64_t);
    uint64_t sum_int = 0L;
    for (size_t i = 0; i < nslots; i++) {
      sum_int += (uint64_t) to_long(m & mask);
      m >>= sizeof(uint64_t) * 8;
    }
    double sum = ((double)sum_int)/100.0;
    //cerr << "sum_int: " << sum_int << endl;
    //cerr << "sum: " << sum << endl;

    double threshold = sum * fraction;
    // encrypt threshold for ps_value_OPE
    uint64_t threshold_int = (uint64_t) roundToLong(threshold * 100.0);
    //cerr << "threshold_int: " << threshold_int << endl;
    string encVALUE = cm_stub.crypt(cm.getmkey(), to_s(threshold_int), TYPE_INTEGER,
                               fieldname(partsupp::ps_value, "OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    // do all the one element agg group cases first.
    // this is a very hacky query where we try to do several things at the
    // same time, while optimizing for minimum data sent back
    ostringstream s1;
    s1
      << "SELECT SQL_NO_CACHE ps_partkey_DET, ps_value_DET, cnt, suppkeys FROM ("
        << "SELECT SQL_NO_CACHE "
          << "ps_partkey_DET, ps_value_DET, "
          << "COUNT(*) AS cnt, COUNT(*) > 1 OR ps_value_OPE > "
            << encVALUE << " AS filter, "
          << "CASE COUNT(*) WHEN 1 THEN NULL ELSE GROUP_CONCAT(ps_suppkey_DET) END AS suppkeys "
        << "FROM partsupp_enc, supplier_enc, nation_enc "
        << "WHERE "
          << "ps_suppkey_DET = s_suppkey_DET AND "
          << "s_nationkey_DET = n_nationkey_DET AND "
          << "n_name_DET = " << marshallBinary(encNAME) << " "
        << "GROUP BY ps_partkey_DET"
      << ") AS __ANON__ WHERE filter = 1";
    //cerr << s1.str() << endl;

    {
      NamedTimer t(__func__, "execute_groups");
      conn.execute(s1.str(), dbres);
    }
    {
      NamedTimer t(__func__, "unpack_groups");
      res = dbres->unpack();
      assert(res.ok);
    }

    typedef pair<string, vector<string> > GroupEntry;
    vector<GroupEntry> savedKeys;
    {
      NamedTimer t(__func__, "decrypt_groups");
      for (auto row : res.rows) {
        // first, check count(*). if count(*) == 1, go ahead
        // and do the decryption. if count(*) > 1, then save ps_partkey_DET
        // away for later (but do NOT decrypt it)

        if (valFromStr(row[2].data) == 1) {
          // ps_partkey
          uint64_t ps_partkey = decryptRowFromTo<uint64_t>(
                  row[0].data,
                  12345,
                  fieldname(partsupp::ps_partkey, "DET"),
                  TYPE_INTEGER,
                  SECLEVEL::DETJOIN,
                  getMin(oDET),
                  cm);

          // ps_value
          uint64_t ps_value_int = decryptRow<uint64_t>(
                  row[1].data,
                  12345,
                  fieldname(partsupp::ps_value, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double ps_value = ((double)ps_value_int)/100.0;
          assert(ps_value_int > threshold_int);

          results.push_back(q11entry(ps_partkey, ps_value));
        } else {
          vector<string> suppkeys;
          tokenize(row[3].data, ",", suppkeys);
          assert(suppkeys.size() > 1);
          savedKeys.push_back(GroupEntry(row[0].data, suppkeys));
        }
      }
    }

    //cerr << "results.size (w/ 1 groups): " << results.size() << endl;
    //sort(results.begin(), results.end(), q11entry_functor);
    //for (auto r : results) {
    //  cerr << r << endl;
    //}

    // 1) map each GroupEntry into strings of
    //  (ps_partkey_DET = X AND ps_suppkey IN (s1, s2, ..., sN))
    // 2) join each of the strings from above with OR

    struct gentry_mapper {
      inline string operator()(const GroupEntry &e) const {
        ostringstream o;
        o << "(ps_partkey_DET = "
          << e.first << " AND ps_suppkey_DET IN ("
            << join(e.second, ",")
          << "))";
        return o.str();
      }
    };

    vector<string> mapped;
    mapped.resize(savedKeys.size());
    transform(savedKeys.begin(), savedKeys.end(),
              mapped.begin(), gentry_mapper());

    ostringstream s2;
    s2
      << "SELECT SQL_NO_CACHE "
        << "ps_partkey_DET, ps_value_DET "
      << "FROM partsupp_enc "
      << "WHERE " << join(mapped, " OR ");

    {
      NamedTimer t(__func__, "execute_more_one_groups");
      conn.execute(s2.str(), dbres);
    }
    {
      NamedTimer t(__func__, "unpack_more_one_groups");
      res = dbres->unpack();
      assert(res.ok);
    }

    map<uint64_t, double> aggState;
    {
      NamedTimer t(__func__, "decrypt_more_one_groups");
      for (auto row : res.rows) {

        // ps_partkey
        uint64_t ps_partkey = decryptRowFromTo<uint64_t>(
                row[0].data,
                12345,
                fieldname(partsupp::ps_partkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
                getMin(oDET),
                cm);

        // ps_value
        uint64_t ps_value_int = decryptRow<uint64_t>(
                row[1].data,
                12345,
                fieldname(partsupp::ps_value, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double ps_value = ((double)ps_value_int)/100.0;

        auto it = aggState.find(ps_partkey);
        if (it == aggState.end()) {
          aggState[ps_partkey] = ps_value;
        } else {
          aggState[ps_partkey] += ps_value;
        }
      }
    }

    {
      NamedTimer t(__func__, "agg");
      for (map<uint64_t, double>::iterator it = aggState.begin();
           it != aggState.end();
           ++it) {
        if (it->second > threshold) {
          q11entry e(it->first, it->second);
          results.push_back(e);
        }
      }
      // sort
      sort(results.begin(), results.end(), q11entry_functor);
    }
}

static void do_query_q11_opt(Connect &conn,
                             CryptoManager &cm,
                             const string &name,
                             double fraction,
                             vector<q11entry> &results) {
    crypto_manager_stub cm_stub(&cm);
    assert(name.size() <= 25);
    NamedTimer fcnTimer(__func__);

    bool isBin = false;
    string encNAME = cm_stub.crypt(cm.getmkey(), name, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(isBin);
    assert(encNAME.size() == name.size());
    encNAME.resize(25);

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "ps_partkey_DET, "
        << "agg(ps_value_AGG, " << pkinfo << ") "
      << "FROM partsupp_enc, supplier_enc, nation_enc "
      << "WHERE "
        << "ps_suppkey_DET = s_suppkey_DET AND "
        << "s_nationkey_DET = n_nationkey_DET AND "
        << "n_name_DET = " << marshallBinary(encNAME) << " "
      << "GROUP BY ps_partkey_DET";
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

    double totalSum = 0.0;
    {
      NamedTimer t(__func__, "decrypt");
      results.reserve(res.rows.size());
      for (auto row : res.rows) {

        // ps_partkey
        uint64_t ps_partkey = decryptRowFromTo<uint64_t>(
                row[0].data,
                12345,
                fieldname(partsupp::ps_partkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
                getMin(oDET),
                cm);

        // ps_value
        uint64_t ps_value_int = decryptRow<uint64_t>(
                row[1].data,
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

static void do_query_q2(Connect &conn,
                        uint64_t size,
                        const string &type,
                        const string &name,
                        vector<q2entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s << "select SQL_NO_CACHE s_acctbal, s_name, n_name, p_partkey, p_mfgr, s_address, s_phone, s_comment from PART, SUPPLIER, PARTSUPP, NATION, REGION where p_partkey = ps_partkey and s_suppkey = ps_suppkey and p_size = " << size << " and p_type like '%" << type << "' and s_nationkey = n_nationkey and n_regionkey = r_regionkey and r_name = '" << name << "' and ps_supplycost = ( select min(ps_supplycost) from PARTSUPP, SUPPLIER, NATION, REGION where p_partkey = ps_partkey and s_suppkey = ps_suppkey and s_nationkey = n_nationkey and n_regionkey = r_regionkey and r_name = '" << name << "') order by s_acctbal desc, n_name, s_name, p_partkey limit 100";
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

    for (auto row : res.rows) {
      results.push_back(
          q2entry(resultFromStr<double>(row[0].data),
                  row[1].data,
                  row[2].data,
                  resultFromStr<uint64_t>(row[3].data),
                  row[4].data,
                  row[5].data,
                  row[6].data,
                  row[7].data));
    }

}

static void do_query_q2_noopt(Connect &conn,
                              CryptoManager &cm,
                              uint64_t size,
                              const string &type,
                              const string &name,
                              vector<q2entry> &results) {
    assert(name.size() <= 25);

    crypto_manager_stub cm_stub(&cm);
    NamedTimer fcnTimer(__func__);

    string lowertype(lower_s(type));

    bool isBin = false;
    string encSIZE = cm_stub.crypt(cm.getmkey(), to_s(size), TYPE_INTEGER,
                              fieldname(part::p_size, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(!isBin);

    string encNAME = cm_stub.crypt(cm.getmkey(), name, TYPE_TEXT,
                              fieldname(region::r_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(name.size() == encNAME.size());
    assert(isBin);

    // right-pad encNAME with 0's
    encNAME.resize(25);

    Binary key(cm.getKey(cm.getmkey(), fieldname(part::p_type, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary(lowertype));

    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "s_acctbal_DET, s_name_DET, n_name_DET, p_partkey_DET, "
        << "p_mfgr_DET, s_address_DET, s_phone_DET, s_comment_DET, p_type_DET "
      << "FROM part_enc, supplier_enc, partsupp_enc, nation_enc, region_enc "
      << "WHERE "
        << "p_partkey_DET = ps_partkey_DET AND "
        << "s_suppkey_DET = ps_suppkey_DET AND "
        << "p_size_DET = " << encSIZE << " AND "
        << "searchSWP("
          << marshallBinary(string((char *)t.ciph.content, t.ciph.len))
          << ", "
          << marshallBinary(string((char *)t.wordKey.content, t.wordKey.len))
          << ", p_type_SWP) = 1 AND "
        << "s_nationkey_DET = n_nationkey_DET AND "
        << "n_regionkey_DET = r_regionkey_DET AND "
        << "r_name_DET = " << marshallBinary(encNAME) << " AND "
        << "ps_supplycost_OPE = ("
          << "SELECT "
            << "min(ps_supplycost_OPE) "
          << "FROM partsupp_enc, supplier_enc, nation_enc, region_enc "
          << "WHERE "
            << "p_partkey_DET = ps_partkey_DET AND "
            << "s_suppkey_DET = ps_suppkey_DET AND "
            << "s_nationkey_DET = n_nationkey_DET AND "
            << "n_regionkey_DET = r_regionkey_DET AND "
            << "r_name_DET = " << marshallBinary(encNAME) << ") "
      << "ORDER BY "
        << "s_acctbal_OPE DESC, n_name_OPE, s_name_OPE, p_partkey_OPE "
      << "LIMIT 100";
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

    for (auto row : res.rows) {

        uint64_t s_acctbal_int = decryptRow<uint64_t, 7>(
                row[0].data,
                12345,
                fieldname(supplier::s_acctbal, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double s_acctbal = ((double)s_acctbal_int)/100.0;

        string s_name = decryptRow<string>(
                row[1].data,
                12345,
                fieldname(supplier::s_name, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        string n_name = decryptRow<string>(
                row[2].data,
                12345,
                fieldname(nation::n_name, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        uint64_t p_partkey = decryptRowFromTo<uint64_t>(
                row[3].data,
                12345,
                fieldname(part::p_partkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
                getMin(oDET),
                cm);

        string p_mfgr = decryptRow<string>(
                row[4].data,
                12345,
                fieldname(part::p_mfgr, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        string s_address = decryptRow<string>(
                row[5].data,
                12345,
                fieldname(supplier::s_address, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        string s_phone = decryptRow<string>(
                row[6].data,
                12345,
                fieldname(supplier::s_phone, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        string s_comment = decryptRow<string>(
                row[7].data,
                12345,
                fieldname(supplier::s_comment, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        string p_type = decryptRow<string>(
                row[8].data,
                12345,
                fieldname(part::p_type, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        q2entry entry(s_acctbal,
                      s_name,
                      n_name,
                      p_partkey,
                      p_mfgr,
                      s_address,
                      s_phone,
                      s_comment);

        if (lower_s(p_type).find(lowertype) != string::npos) {
          results.push_back(entry);
        } else {
          cerr << "bad row found (p_type = " << p_type << "): " << entry << endl;
        }
    }
}

static inline uint32_t random_year() {
    static const uint32_t gap = 1999 - 1993 + 1;
    return 1993 + (rand() % gap);

}

static void usage(char **argv) {
    cerr << "[USAGE]: " << argv[0] << " "
         << "(((--orig-query1|--crypt-query1|--crypt-opt-query1) year)|(--orig-query11|--crypt-query11|--crypt-opt-query11|--crypt-opt-proj-query11 nation fraction)|((--orig-query2|--crypt-query2) size type name)|((--orig-query14|--crypt-opt-query14) year)) num_queries db_name"

         << endl;
}

static const bool PrintResults = true;

enum query_selection {
  query1,
  query2,
  query11,
  query14,
};

int main(int argc, char **argv) {
    srand(time(NULL));
    if (argc != 5 && argc != 6 && argc != 7) {
        usage(argv);
        return 1;
    }

    static const char * Query1Strings[] = {
        "--orig-query1",
        "--crypt-query1",
        "--crypt-opt-query1",
        "--crypt-opt-packed-query1",
    };
    std::set<string> Query1Modes
      (Query1Strings, Query1Strings + NELEMS(Query1Strings));

    static const char * Query2Strings[] = {
        "--orig-query2",
        "--crypt-query2",
    };
    std::set<string> Query2Modes
      (Query2Strings, Query2Strings + NELEMS(Query2Strings));

    static const char * Query11Strings[] = {
        "--orig-query11",
        "--crypt-query11",
        "--crypt-opt-query11",
        "--crypt-opt-proj-query11",
    };
    std::set<string> Query11Modes
      (Query11Strings, Query11Strings + NELEMS(Query11Strings));

    static const char * Query14Strings[] = {
        "--orig-query14",
        "--crypt-opt-query14",
    };
    std::set<string> Query14Modes
      (Query14Strings, Query14Strings + NELEMS(Query14Strings));

    enum query_selection q;

    if (Query1Modes.find(argv[1]) != Query1Modes.end()) {
        q = query1;
    } else if (Query2Modes.find(argv[1]) != Query2Modes.end()) {
        q = query2;
    } else if (Query11Modes.find(argv[1]) != Query11Modes.end()) {
        q = query11;
    } else if (Query14Modes.find(argv[1]) != Query14Modes.end()) {
        q = query14;
    } else {
        usage(argv);
        return 1;
    }


    int input_nruns;
    string db_name;
    switch (q) {
      case query1: input_nruns = atoi(argv[3]); db_name = argv[4]; break;
      case query2: input_nruns = atoi(argv[5]); db_name = argv[6]; break;
      case query11: input_nruns = atoi(argv[4]); db_name = argv[5]; break;
      case query14: input_nruns = atoi(argv[3]); db_name = argv[4]; break;
    }
    uint32_t nruns = (uint32_t) input_nruns;

    unsigned long ctr = 0;

    Connect conn("localhost", "root", "", db_name, 3307);
    CryptoManager cm("12345");

#define PRINT_RESULTS() \
    do { \
        if (PrintResults) { \
            for (auto r : results) { \
                cout << r << endl; \
            } \
            cout << "------" << endl; \
        } \
    } while (0)

    string mode(argv[1] + 2); // remove "--"

    switch (q) {
      case query1:
        {
          int input_year = atoi(argv[2]);
          assert(input_year >= 0);
          uint32_t year = (uint32_t) input_year;
          vector<q1entry> results;
          if (mode == "orig-query1") {
            // vanilla MYSQL case
            for (size_t i = 0; i < nruns; i++) {
              do_query_q1(conn, year, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query1") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q1_noopt(conn, cm, year, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-opt-query1") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q1_opt(conn, cm, year, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-opt-packed-query1") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q1_packed_opt(conn, cm, year, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
          break;
        }
      case query2:
        {
          uint64_t size = (uint64_t) atoi(argv[2]);
          string type(argv[3]);
          string name(argv[4]);
          vector<q2entry> results;
          if (mode == "orig-query2") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q2(conn, size, type, name, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query2") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q2_noopt(conn, cm, size, type, name, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;
      case query11:
        {
          string nation = argv[2];
          double fraction = atof(argv[3]);
          if (fraction < 0.0 || fraction > 1.0) {
            usage(argv);
            return 1;
          }
          vector<q11entry> results;
          if (mode == "orig-query11") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q11(conn, nation, fraction, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query11") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q11_noopt(conn, cm, nation, fraction, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-opt-query11") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q11_opt(conn, cm, nation, fraction, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if ("crypt-opt-proj-query11") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q11_opt_proj(conn, cm, nation, fraction, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;
      case query14:
        {
          int input_year = atoi(argv[2]);
          assert(input_year >= 0);
          uint32_t year = (uint32_t) input_year;
          vector<q14entry> results;
          if (mode == "orig-query14") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q14(conn, year, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-opt-query14") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q14_opt(conn, cm, year, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;
      default: assert(false);
    }
    cerr << ctr << endl;
    return 0;
}
