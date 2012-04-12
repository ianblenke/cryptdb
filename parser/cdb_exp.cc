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

#include <edb/ConnectNew.hh>
#include <parser/cdb_rewrite.hh>
#include <parser/cdb_helpers.hh>
#include <crypto/paillier.hh>
#include <util/executor.hpp>
#include <util/util.hh>

using namespace std;
using namespace NTL;

static bool UseOldOpe = false;
static bool DoParallel = true;
static bool UseMYISAM = false;
static bool UseMySQL = true;
static const size_t NumThreads = 8;

template <typename T> struct ctype_to_itemres {};

template <> struct ctype_to_itemres<int64_t> {
  static const uint8_t type = INT_RESULT;
};
template <> struct ctype_to_itemres<double> {
  static const uint8_t type = REAL_RESULT;
};
template <> struct ctype_to_itemres<string> {
  static const uint8_t type = STRING_RESULT;
};

template <typename T>
T ReadKeyElem(const uint8_t*& buf) {
  uint8_t type = *buf++;
  cerr << "type: " << (int)type << endl;
  assert(type == ctype_to_itemres<T>::type);
  const T* p = (const T *) buf;
  T r = *p;
  buf += sizeof(T);
  return r;
}

template <>
string ReadKeyElem<string>(const uint8_t*& buf) {
  uint8_t type = *buf++;
  assert(type == ctype_to_itemres<string>::type);
  const uint32_t* p = (const uint32_t *) buf;
  uint32_t s = *p;
  buf += sizeof(uint32_t);
  string r((const char *)buf, (size_t) s);
  buf += s;
  return r;
}

static inline uint32_t EncodeDate(uint32_t month, uint32_t day, uint32_t year) {
  return day | (month << 5) | (year << 9);
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

struct date_t {
  // dd-mm-yyyy
  // TODO: this implementation doesn't check for valid dates
  date_t(
    uint32_t day,
    uint32_t month,
    uint32_t year) {}

  date_t(
    const string& yyyy_mm_dd) {
    int year, month, day;
    int ret = sscanf(yyyy_mm_dd.c_str(), "%d-%d-%d", &year, &month, &day);
    assert(ret == 3);
    assert(1 <= day && day <= 31);
    assert(1 <= month && month <= 12);
    assert(year >= 0);
    this->day = day;
    this->month = month;
    this->year = year;
  }

  inline uint32_t encode() const { return EncodeDate(month, day, year); }

  void add_years(uint32_t years) {
    year += years;
  }

  // TODO: this implementation doesn't take into account
  // truncation, ie
  //
  // 11-30-2009 + 3 months should go to 2-28-2010, not 2-30-2010
  void add_months(uint32_t months) {
    uint32_t tmp = (month - 1) + months;
    month = (tmp % 12) + 1;
    year += (tmp / 12);
  }

private:
  uint32_t day;   // [1-31]
  uint32_t month; // [1-12]
  uint32_t year;
};

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
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
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
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
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

typedef vector<SqlItem> SqlRow;
typedef vector<SqlRow>  SqlRowGroup;

// split allRows into numGroups evenly, putting the results in
// groups. extra rows are placed into the last non-empty group.
// empty groups are created if there are not enough rows to go
// around

template <typename T>
static void SplitRowsIntoGroups(
    vector< vector<T> >& groups,
    const vector< T >& allRows,
    size_t numGroups) {
  assert(numGroups > 0);
  groups.resize(numGroups);
  size_t blockSize = allRows.size() / numGroups;
  if (blockSize == 0) {
    groups[0] = allRows;
    return;
  }
  for (size_t i = 0; i < numGroups; i++) {
    size_t start = i * blockSize;
    if (i == numGroups - 1) {
      // last group
      groups[i].insert(
          groups[i].begin(),
          allRows.begin() + start,
          allRows.end());
    } else {
      groups[i].insert(
          groups[i].begin(),
          allRows.begin() + start,
          allRows.begin() + start + blockSize);
    }
  }
}

struct lineitem_group {
    lineitem_group() :
      l_quantity(0.0),
      l_extendedprice(0.0),
      disc_price(0.0),
      charge(0.0),
      l_discount(0.0),
      l_tax(0.0),
      count(0) {}

    lineitem_group(
      double l_quantity,
      double l_extendedprice,
      double disc_price,
      double charge,
      double l_discount,
      double l_tax,
      uint64_t count) :
    l_quantity(l_quantity),
    l_extendedprice(l_extendedprice),
    disc_price(disc_price),
    charge(charge),
    l_discount(l_discount),
    l_tax(l_tax),
    count(count) {}

    double l_quantity;
    double l_extendedprice;
    double disc_price;
    double charge;
    double l_discount;
    double l_tax;
    uint64_t count;

    lineitem_group operator+(const lineitem_group& b) const {
      return lineitem_group(
        l_quantity + b.l_quantity,
        l_extendedprice + b.l_extendedprice,
        disc_price + b.disc_price,
        charge + b.charge,
        l_discount + b.l_discount,
        l_tax + b.l_tax,
        count + b.count);
    }

    lineitem_group& operator+=(const lineitem_group& b) {
      l_quantity += b.l_quantity;
      l_extendedprice += b.l_extendedprice;
      disc_price += b.disc_price;
      charge += b.charge;
      l_discount += b.l_discount;
      l_tax += b.l_tax;
      count += b.count;
      return *this;
    }
};

class lineitem_key : public pair<string, string> {
public:
    lineitem_key(const string &returnflag,
                 const string &linestatus)
        : pair<string, string>(returnflag, linestatus) {}
    inline string l_returnflag() const { return first;  }
    inline string l_linestatus() const { return second; }
};



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

enum customer {
    c_custkey     ,
    c_name        ,
    c_address     ,
    c_nationkey   ,
    c_phone       ,
    c_acctbal     ,
    c_mktsegment  ,
    c_comment     ,
};

enum orders {
    o_orderkey       ,
    o_custkey        ,
    o_orderstatus    ,
    o_totalprice     ,
    o_orderdate      ,
    o_orderpriority  ,
    o_clerk          ,
    o_shippriority   ,
    o_comment        ,
};

struct q1entry {
    q1entry() {} // for map
    q1entry(const string& l_returnflag,
            const string& l_linestatus,
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

struct q3entry {
  q3entry(
    uint64_t l_orderkey,
    double revenue,
    string o_orderdate,
    uint64_t o_shippriority) :
  l_orderkey(l_orderkey),
  revenue(revenue),
  o_orderdate(o_orderdate),
  o_shippriority(o_shippriority) {}

  uint64_t l_orderkey;
  double revenue;
  string o_orderdate;
  uint64_t o_shippriority;
};

struct q3entry_enc {
  q3entry_enc() {}
  q3entry_enc(
    uint64_t l_orderkey_DET,
    double revenue,
    uint64_t o_orderdate_OPE,
    uint64_t o_orderdate_DET,
    uint64_t o_shippriority_DET) :
  l_orderkey_DET(l_orderkey_DET),
  revenue(revenue),
  o_orderdate_OPE(o_orderdate_OPE),
  o_orderdate_DET(o_orderdate_DET),
  o_shippriority_DET(o_shippriority_DET) {}

  uint64_t l_orderkey_DET;
  double revenue;
  uint64_t o_orderdate_OPE;
  uint64_t o_orderdate_DET;
  uint64_t o_shippriority_DET;
};

inline ostream& operator<<(ostream &o, const q3entry &q) {
  o << q.l_orderkey << "|" << q.revenue << "|" << q.o_orderdate << "|" << q.o_shippriority;
  return o;
}

struct q4entry {
  q4entry(
    string o_orderpriority,
    uint64_t order_count)
    : o_orderpriority(o_orderpriority),
      order_count(order_count) {}
  string o_orderpriority;
  uint64_t order_count;
};

inline ostream& operator<<(ostream &o, const q4entry &q) {
  o << q.o_orderpriority << "|" << q.order_count;
  return o;
}

struct q5entry {
  q5entry(
    string n_name,
    double revenue)
    : n_name(n_name),
      revenue(revenue) {}
  string n_name;
  double revenue;
};

inline ostream& operator<<(ostream &o, const q5entry &q) {
  o << q.n_name << "|" << q.revenue;
  return o;
}

typedef double q6entry;

struct q7entry {
  q7entry(
    const string& supp_nation,
    const string& cust_nation,
    uint64_t l_year,
    double revenue) :
  supp_nation(supp_nation),
  cust_nation(cust_nation),
  l_year(l_year),
  revenue(revenue) {}

  string supp_nation;
  string cust_nation;
  uint64_t l_year;
  double revenue;
};

inline ostream& operator<<(ostream &o, const q7entry &q) {
  o << q.supp_nation << "|" << q.cust_nation << "|" << q.l_year << "|" << q.revenue;
  return o;
}

struct q8entry {
  q8entry(
      uint64_t o_year,
      double mkt_share)
    : o_year(o_year), mkt_share(mkt_share) {}
  uint64_t o_year;
  double mkt_share;
};

inline ostream& operator<<(ostream& o, const q8entry& q) {
  o << q.o_year << "|" << q.mkt_share;
  return o;
}

struct q9entry {
  q9entry(
      string nation,
      uint64_t o_year,
      double sum_profit)
    : nation(nation), o_year(o_year), sum_profit(sum_profit) {}
  string nation;
  uint64_t o_year;
  double sum_profit;
};

inline ostream& operator<<(ostream& o, const q9entry& q) {
  o << q.nation << "|" << q.o_year << "|" << q.sum_profit;
  return o;
}

struct q10entry {
  q10entry(
    uint64_t c_custkey,
    const string& c_name,
    double revenue,
    double c_acctbal,
    const string& n_name,
    const string& c_address,
    const string& c_phone,
    const string& c_comment) :
  c_custkey(c_custkey),
  c_name(c_name),
  revenue(revenue),
  c_acctbal(c_acctbal),
  n_name(n_name),
  c_address(c_address),
  c_phone(c_phone),
  c_comment(c_comment) {}

  uint64_t c_custkey;
  string c_name;
  double revenue;
  double c_acctbal;
  string n_name;
  string c_address;
  string c_phone;
  string c_comment;
};

inline ostream& operator<<(ostream& o, const q10entry& q) {
  o << q.c_custkey << "|" << q.c_name << "|" << q.revenue << "|" <<
  q.c_acctbal << "|" << q.n_name << "|" << q.c_address << "|" <<
  q.c_phone << "|" << q.c_comment;
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

struct q12entry {
  q12entry(
    const string& l_shipmode,
    uint64_t high_line_count,
    uint64_t low_line_count) :
    l_shipmode(l_shipmode),
    high_line_count(high_line_count),
    low_line_count(low_line_count) {}
  string l_shipmode;
  uint64_t high_line_count;
  uint64_t low_line_count;
};

inline ostream& operator<<(ostream &o, const q12entry &q) {
  o << q.l_shipmode << "|" << q.high_line_count << "|" << q.low_line_count;
  return o;
}

typedef double q14entry;

struct q15entry {
  q15entry(
    uint64_t s_suppkey,
    const string& s_name,
    const string& s_address,
    const string& s_phone,
    double total_revenue) :
  s_suppkey(s_suppkey),
  s_name(s_name),
  s_address(s_address),
  s_phone(s_phone),
  total_revenue(total_revenue) {}

  uint64_t s_suppkey;
  string s_name;
  string s_address;
  string s_phone;
  double total_revenue;
};

inline ostream& operator<<(ostream &o, const q15entry &q) {
  o <<
    q.s_suppkey << "|" << q.s_name << "|" << q.s_address << "|" <<
    q.s_phone << "|" << q.total_revenue;
  return o;
}

typedef double q17entry;

struct q18entry {
    q18entry(
        const string& c_name,
        uint64_t c_custkey,
        uint64_t o_orderkey,
        uint64_t o_orderdate,
        double o_totalprice,
        double sum_l_quantity) :
    c_name(c_name),
    c_custkey(c_custkey),
    o_orderkey(o_orderkey),
    o_orderdate(o_orderdate),
    o_totalprice(o_totalprice),
    sum_l_quantity(sum_l_quantity) {}

    string c_name;
    uint64_t c_custkey;
    uint64_t o_orderkey;
    uint64_t o_orderdate;
    double o_totalprice;
    double sum_l_quantity;
};

inline ostream& operator<<(ostream &o, const q18entry &q) {
    o << q.c_name << "|" <<
         q.c_custkey << "|" <<
         q.o_orderkey << "|" <<
         q.o_orderdate << "|" <<
         q.o_totalprice << "|" <<
         q.sum_l_quantity;
    return o;
}

struct q20entry {
    q20entry(const string& s_name,
             const string& s_address)
        : s_name(s_name), s_address(s_address) {}
    string s_name;
    string s_address;
};

inline ostream& operator<<(ostream &o, const q20entry &q) {
    o << q.s_name << "|" << q.s_address;
    return o;
}

struct q22entry {
  q22entry(
    const string& cntrycode,
    uint64_t numcust,
    double totacctbal) :
    cntrycode(cntrycode),
    numcust(numcust),
    totacctbal(totacctbal) {}
  string cntrycode;
  uint64_t numcust;
  double totacctbal;
};

inline ostream& operator<<(ostream &o, const q22entry &q) {
  o << q.cntrycode << "|" << q.numcust << "|" << q.totacctbal;
  return o;
}

static void do_query_q1(ConnectNew &conn,
                        uint32_t year,
                        vector<q1entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream buf;
    buf << "select SQL_NO_CACHE l_returnflag, l_linestatus, sum(l_quantity) as sum_qty, sum(l_extendedprice) as sum_base_price, sum(l_extendedprice * (1 - l_discount)) as sum_disc_price, sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) as sum_charge, avg(l_quantity) as avg_qty, avg(l_extendedprice) as avg_price, avg(l_discount) as avg_disc, count(*) as count_order from LINEITEM "

        << "where l_shipdate <= date '" << year << "-1-1' "
        << "group by l_returnflag, l_linestatus order by l_returnflag, l_linestatus"

        ;
    cerr << buf.str() << endl;

    DBResultNew * dbres;
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

template <typename T>
struct Q1NoSortImplInfo {};

template <>
struct Q1NoSortImplInfo<uint64_t> {
  static inline const char * agg_func_name() { return "sum_hash_agg_int"; }
  static inline const char * table_name()
    { return UseMYISAM ? "LINEITEM_INT_MYISAM" : "LINEITEM_INT"; }
  static inline const char * unit() { return "100"; }

  static inline double convert(uint64_t i, size_t n) {
    return double(i)/pow(100.0, n);
  }
};

template <>
struct Q1NoSortImplInfo<double> {
  static inline const char * agg_func_name() { return "sum_hash_agg_double"; }
  static inline const char * table_name()
    { return UseMYISAM ? "LINEITEM_DOUBLE_MYISAM" : "LINEITEM_DOUBLE"; }
  static inline const char * unit() { return "1.0"; }

  static inline double convert(double i, size_t n) { return i; }
};



template <typename T>
struct Q1NoSortImpl {
  typedef map< pair< uint8_t, uint8_t >,
               pair< uint64_t, T > > Q1AggGroup;



  static void ReadChar2AggGroup(const string& data,
                                Q1AggGroup& agg_group) {

    const uint8_t *p   = (const uint8_t *) data.data();
    const uint8_t *end = (const uint8_t *) data.data() + data.size();
    while (p < end) {
      uint8_t l_returnflag = ReadKeyElem<string>(p)[0];
      uint8_t l_linestatus = ReadKeyElem<string>(p)[0];
      const uint64_t *u64p = (const uint64_t *) p;
      uint64_t count = *u64p;
      p += sizeof(uint64_t);
      const T *dp = (const T *) p;
      T value = *dp;
      p += sizeof(T);
      agg_group[make_pair(l_returnflag, l_linestatus)] = make_pair(count, value);
    }
  }

  static void q1_impl(ConnectNew& conn, uint32_t year, vector<q1entry>& results) {
    NamedTimer fcnTimer(__func__);

    const char *agg_name = Q1NoSortImplInfo<T>::agg_func_name();
    const char *unit = Q1NoSortImplInfo<T>::unit();

    ostringstream buf;
    buf << "select SQL_NO_CACHE " <<  agg_name << "(l_returnflag, l_linestatus, l_quantity) as sum_qty, " <<  agg_name << "(l_returnflag, l_linestatus, l_extendedprice) as sum_base_price, " <<  agg_name << "(l_returnflag, l_linestatus, l_extendedprice * (" << unit << " - l_discount)) as sum_disc_price, " <<  agg_name << "(l_returnflag, l_linestatus, l_extendedprice * (" << unit << " - l_discount) * (" << unit << " + l_tax)) as sum_charge, " <<  agg_name << "(l_returnflag, l_linestatus, l_discount) as sum_disc from " << Q1NoSortImplInfo<T>::table_name()
        << " where l_shipdate <= date '" << year << "-1-1'"
        ;
    cerr << buf.str() << endl;

    DBResultNew * dbres;
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
    assert(res.rows.size() == 1);

    Q1AggGroup sum_qty_group;
    Q1AggGroup sum_base_price_group;
    Q1AggGroup sum_disc_price_group;
    Q1AggGroup sum_charge_group;
    Q1AggGroup sum_disc_group;

    ReadChar2AggGroup(res.rows[0][0].data, sum_qty_group);
    ReadChar2AggGroup(res.rows[0][1].data, sum_base_price_group);
    ReadChar2AggGroup(res.rows[0][2].data, sum_disc_price_group);
    ReadChar2AggGroup(res.rows[0][3].data, sum_charge_group);
    ReadChar2AggGroup(res.rows[0][4].data, sum_disc_group);

    for (typename Q1AggGroup::iterator it = sum_qty_group.begin();
         it != sum_qty_group.end(); ++it) {

        pair< uint64_t, T >& p0 = it->second;
        pair< uint64_t, T >& p1 = sum_base_price_group[it->first];
        pair< uint64_t, T >& p2 = sum_disc_price_group[it->first];
        pair< uint64_t, T >& p3 = sum_charge_group[it->first];
        pair< uint64_t, T >& p4 = sum_disc_group[it->first];

        results.push_back(
                q1entry(
                    string(1, it->first.first),
                    string(1, it->first.second),
                    Q1NoSortImplInfo<T>::convert(p0.second, 1),
                    Q1NoSortImplInfo<T>::convert(p1.second, 1),
                    Q1NoSortImplInfo<T>::convert(p2.second, 2),
                    Q1NoSortImplInfo<T>::convert(p3.second, 3),
                    Q1NoSortImplInfo<T>::convert(p0.second, 1) / double(p0.first),
                    Q1NoSortImplInfo<T>::convert(p1.second, 1) / double(p1.first),
                    Q1NoSortImplInfo<T>::convert(p4.second, 1) / double(p4.first),
                    p0.first));
        // TODO: sort results
    }
  }
};

static void do_query_q1_nosort_int(ConnectNew &conn,
                                   uint32_t year,
                                   vector<q1entry> &results) {
  Q1NoSortImpl<uint64_t>::q1_impl(conn, year, results);
}

static void do_query_q1_nosort_double(ConnectNew &conn,
                                      uint32_t year,
                                      vector<q1entry> &results) {
  Q1NoSortImpl<double>::q1_impl(conn, year, results);
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

static void do_query_q1_packed_opt(ConnectNew &conn,
                                   CryptoManager &cm,
                                   uint32_t year,
                                   vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, "ope_join",
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

    DBResultNew * dbres;
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

static void do_query_q1_opt_nosort(ConnectNew &conn,
                                   CryptoManager &cm,
                                   uint32_t year,
                                   vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    DBResultNew * dbres;

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s <<
      "SELECT SQL_NO_CACHE agg_char2("
      "l_returnflag_DET, l_linestatus_DET, l_bitpacked_AGG, "
      << pkinfo << ", " << (DoParallel ? "1" : "0") <<
      ") FROM " << (UseMYISAM ? "lineitem_enc_MYISAM" : "lineitem_enc")
      << "WHERE l_shipdate_OPE <= " << encDATE;
    cerr << s.str() << endl;

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
      assert(res.rows.size() == 1);

      string data = res.rows[0][0].data;

      // format of data is
      // [ l_returnflag_DET (1 byte) | l_linestatus_DET (1 byte) |
      //   count(*) (8 bytes) | agg(l_bitpacked_AGG) (256 bytes) ]*

      assert((data.size() % (1 + 1 + 8 + 256)) == 0);

      typedef map< pair<unsigned char, unsigned char>, q1entry > GroupMap;
      GroupMap groups;

      const uint8_t *p   = (const uint8_t *) data.data();
      const uint8_t *end = (const uint8_t *) data.data() + data.size();
      while (p < end) {

        unsigned char l_returnflag_DET = *p++;
        unsigned char l_linestatus_DET = *p++;

        // l_returnflag
        unsigned char l_returnflag_ch = (unsigned char) decryptRow<uint32_t, 1>(
                to_s((int)l_returnflag_DET),
                12345,
                fieldname(lineitem::l_returnflag, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

        // l_linestatus
        unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t, 1>(
                to_s((int)l_linestatus_DET),
                12345,
                fieldname(lineitem::l_linestatus, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

        // warning: endianness on DB machine must be same as this machine
        const uint64_t *u64p = (const uint64_t *) p;
        uint64_t count_order = *u64p;

        p += sizeof(uint64_t);

        ZZ m;
        cm.decrypt_Paillier(string((const char *) p, 256), m);
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

        double avg_qty = sum_qty / ((double)count_order);
        double avg_price = sum_base_price / ((double)count_order);
        double avg_disc = sum_discount / ((double)count_order);

        p += 256;

        groups[make_pair( l_returnflag_ch, l_linestatus_ch )] = q1entry(
              string(1, (char) l_returnflag_ch),
              string(1, (char) l_linestatus_ch),
              sum_qty,
              sum_base_price,
              sum_disc_price,
              sum_charge,
              avg_qty,
              avg_price,
              avg_disc,
              count_order);
      }

      results.reserve(groups.size());
      for (GroupMap::iterator it = groups.begin();
           it != groups.end(); ++it) {
        results.push_back(it->second);
      }
    }
}

static void do_query_q1_opt_row_col_pack(ConnectNew &conn,
                                         CryptoManager &cm,
                                         uint32_t year,
                                         vector<q1entry> &results,
                                         const string& db) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    static const size_t RowColPackPlainSize = 1256;
    static const size_t RowColPackCipherSize = RowColPackPlainSize * 2;
    auto sk = Paillier_priv::keygen(RowColPackCipherSize / 2,
                                    RowColPackCipherSize / 8);
    Paillier_priv pp(sk);
    auto pk = pp.pubkey();

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    DBResultNew * dbres;

    //conn.execute("set @cnt := -1", dbres);

    string filename =
      "/space/stephentu/data/" + db + "/lineitem_enc/row_col_pack/data";

    string pkinfo = marshallBinary(StringFromZZ(pk[0] * pk[0]));
    ostringstream s;
    s <<
      "SELECT SQL_NO_CACHE agg_hash_agg_row_col_pack("
        "2, "
        "l_returnflag_DET, "
        "l_linestatus_DET, "
        "row_id, "
        << pkinfo << ", " <<
        "\"" << filename << "\", " << (DoParallel ? to_s(NumThreads) : string("0"))
        << ", " << (RowColPackCipherSize/8) << ", 3, 1"
      ") FROM " << (UseMYISAM ? "lineitem_enc_rowid_MYISAM" : "lineitem_enc_rowid")
      << " WHERE l_shipdate_OPE <= " << encDATE
      ;
    cerr << s.str() << endl;

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

    static const size_t BitsPerAggField = 83;
    ZZ mask = to_ZZ(1); mask <<= BitsPerAggField; mask -= 1;
    assert(NumBits(mask) == (int)BitsPerAggField);
    {
      assert(res.rows.size() == 1);
      string data = res.rows[0][0].data;

      // format of data is
      // [ l_returnflag_DET (1 byte) | l_linestatus_DET (1 byte) |
      //   count(*) (8 bytes) | rows (3 * (4 + 256) bytes) ]* ]*
      typedef map< pair<unsigned char, unsigned char>, q1entry > GroupMap;
      GroupMap groups;

      const uint8_t *p   = (const uint8_t *) data.data();
      const uint8_t *end = (const uint8_t *) data.data() + data.size();
      while (p < end) {

        unsigned char l_returnflag_DET = ReadKeyElem<int64_t>(p);
        unsigned char l_linestatus_DET = ReadKeyElem<int64_t>(p);

        // l_returnflag
        unsigned char l_returnflag_ch = (unsigned char) decryptRow<uint32_t, 1>(
                to_s((int)l_returnflag_DET),
                12345,
                fieldname(lineitem::l_returnflag, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

        // l_linestatus
        unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t, 1>(
                to_s((int)l_linestatus_DET),
                12345,
                fieldname(lineitem::l_linestatus, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

        // warning: endianness on DB machine must be same as this machine
        const uint64_t *u64p = (const uint64_t *) p;
        uint64_t count_order = *u64p;

        p += sizeof(uint64_t);

        double sum_qty = 0.0;
        double sum_base_price = 0.0;
        double sum_discount = 0.0;
        double sum_disc_price = 0.0;
        double sum_charge = 0.0;

#define TAKE_FROM_SLOT(z, slot) \
        (to_long(((z) >> (BitsPerAggField * (slot))) & mask))

        const uint32_t *u32p = (const uint32_t *) p;
        uint32_t n_aggs = *u32p;
        p += sizeof(uint32_t);

        for (size_t group_i = 0; group_i < n_aggs; group_i++) {
          // interest mask
          const uint32_t *u32p = (const uint32_t *) p;
          uint32_t interest_mask = *u32p;
          p += sizeof(uint32_t);

          ZZ ct = ZZFromBytes(
              (const uint8_t *) p,
              RowColPackCipherSize / 8);
          ZZ m = pp.decrypt(ct);
          p += RowColPackCipherSize / 8;

          for (size_t i = 0; i < 3; i++) {
            if (!(interest_mask & (0x1 << i))) continue;

            long sum_qty_int = TAKE_FROM_SLOT(m, i * 5 + 0);
            long sum_base_price_int = TAKE_FROM_SLOT(m, i * 5 + 1);
            long sum_discount_int = TAKE_FROM_SLOT(m, i * 5 + 2);
            long sum_disc_price_int = TAKE_FROM_SLOT(m, i * 5 + 3);
            long sum_charge_int = TAKE_FROM_SLOT(m, i * 5 + 4);

            sum_qty += ((double)sum_qty_int)/100.0;
            sum_base_price += ((double)sum_base_price_int)/100.0;
            sum_discount += ((double)sum_discount_int)/100.0;
            sum_disc_price += ((double)sum_disc_price_int)/100.0;
            sum_charge += ((double)sum_charge_int)/100.0;
          }
        }

        double avg_qty = sum_qty / ((double)count_order);
        double avg_price = sum_base_price / ((double)count_order);
        double avg_disc = sum_discount / ((double)count_order);

        groups[make_pair( l_returnflag_ch, l_linestatus_ch )] = q1entry(
              string(1, (char) l_returnflag_ch),
              string(1, (char) l_linestatus_ch),
              sum_qty,
              sum_base_price,
              sum_disc_price,
              sum_charge,
              avg_qty,
              avg_price,
              avg_disc,
              count_order);
      }

      results.reserve(groups.size());
      for (GroupMap::iterator it = groups.begin();
           it != groups.end(); ++it) {
        results.push_back(it->second);
      }
    }
}

static void do_query_q1_opt(ConnectNew &conn,
                            CryptoManager &cm,
                            uint32_t year,
                            vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    DBResultNew * dbres;

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
          << "l_returnflag_DET, "
          << "l_linestatus_DET, "
          << "agg(l_bitpacked_AGG, " << pkinfo << "), "
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

struct _q1_noopt_task_state {
  _q1_noopt_task_state() {}
  vector< vector< SqlItem > > rows;
  map<lineitem_key, lineitem_group> agg_groups;
};

struct _q1_noopt_task {
  void operator()() {
      for (auto row : state->rows) {
          // l_returnflag
          unsigned char l_returnflag_ch = (unsigned char) decryptRow<uint32_t, 1>(
                  row[0].data,
                  12345,
                  fieldname(lineitem::l_returnflag, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  *cm);
          string l_returnflag(1, l_returnflag_ch);

          // l_linestatus
          unsigned char l_linestatus_ch = (unsigned char) decryptRow<uint32_t, 1>(
                  row[1].data,
                  12345,
                  fieldname(lineitem::l_linestatus, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  *cm);
          string l_linestatus(1, l_linestatus_ch);

          // l_quantity
          uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
                  row[2].data,
                  12345,
                  fieldname(lineitem::l_quantity, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  *cm);
          double l_quantity = ((double)l_quantity_int)/100.0;

          // l_extendedprice
          uint64_t l_extendedprice_int = decryptRow<uint64_t, 7>(
                  row[3].data,
                  12345,
                  fieldname(lineitem::l_extendedprice, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  *cm);
          double l_extendedprice = ((double)l_extendedprice_int)/100.0;

          // l_discount
          uint64_t l_discount_int = decryptRow<uint64_t, 7>(
                  row[4].data,
                  12345,
                  fieldname(lineitem::l_discount, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  *cm);
          double l_discount = ((double)l_discount_int)/100.0;

          // l_tax
          uint64_t l_tax_int = decryptRow<uint64_t, 7>(
                  row[5].data,
                  12345,
                  fieldname(lineitem::l_tax, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  *cm);
          double l_tax = ((double)l_tax_int)/100.0;

          lineitem_key key(l_returnflag, l_linestatus);
          lineitem_group elem(
              l_quantity,
              l_extendedprice,
              l_extendedprice * (1.0 - l_discount),
              l_extendedprice * (1.0 - l_discount) * (1.0 + l_tax),
              l_discount,
              l_tax,
              1);

          state->agg_groups[key] += elem;
      }
  }
  CryptoManager* cm;
  _q1_noopt_task_state* state;
};

static void do_query_q1_noopt(ConnectNew &conn,
                              CryptoManager &cm,
                              uint32_t year,
                              vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);


    DBResultNew * dbres;

    ostringstream s;
    s << "SELECT SQL_NO_CACHE "

          << "l_returnflag_DET, "
          << "l_linestatus_DET, "
          << "l_quantity_DET, "
          << "l_extendedprice_DET, "
          << "l_discount_DET, "
          << "l_tax_DET "

      << "FROM " << (UseMYISAM ? "lineitem_enc_rowid_MYISAM" : "lineitem_enc_rowid")
      << " WHERE l_shipdate_OPE <= " << encDATE;
    cerr << s.str() << endl;

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

    map<lineitem_key, lineitem_group> agg_groups;
    if (DoParallel) {
      NamedTimer t(__func__, "decrypt-parallel");

      using namespace exec_service;

      vector<_q1_noopt_task_state> states( NumThreads );
      vector<_q1_noopt_task> tasks( NumThreads );
      vector<SqlRowGroup> groups;
      SplitRowsIntoGroups(groups, res.rows, NumThreads);

      for (size_t i = 0; i < NumThreads; i++) {
        tasks[i].cm    = &cm;
        tasks[i].state = &states[i];
        states[i].rows = groups[i];
      }

      Exec<_q1_noopt_task>::DefaultExecutor exec( NumThreads );
      exec.start();
      for (size_t i = 0; i < NumThreads; i++) {
        exec.submit(tasks[i]);
      }
      exec.stop(); // blocks until completion

      for (size_t i = 0; i < NumThreads; i++) {
        _q1_noopt_task_state& s = states[i];
        for (auto it = s.agg_groups.begin(); it != s.agg_groups.end(); ++it) {
          agg_groups[it->first] += it->second;
        }
      }
    } else {
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

          // l_quantity
          uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
                  row[2].data,
                  12345,
                  fieldname(lineitem::l_quantity, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_quantity = ((double)l_quantity_int)/100.0;

          // l_extendedprice
          uint64_t l_extendedprice_int = decryptRow<uint64_t, 7>(
                  row[3].data,
                  12345,
                  fieldname(lineitem::l_extendedprice, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_extendedprice = ((double)l_extendedprice_int)/100.0;

          // l_discount
          uint64_t l_discount_int = decryptRow<uint64_t, 7>(
                  row[4].data,
                  12345,
                  fieldname(lineitem::l_discount, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_discount = ((double)l_discount_int)/100.0;

          // l_tax
          uint64_t l_tax_int = decryptRow<uint64_t, 7>(
                  row[5].data,
                  12345,
                  fieldname(lineitem::l_tax, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_tax = ((double)l_tax_int)/100.0;

          lineitem_key key(l_returnflag, l_linestatus);
          lineitem_group elem(
              l_quantity,
              l_extendedprice,
              l_extendedprice * (1.0 - l_discount),
              l_extendedprice * (1.0 - l_discount) * (1.0 + l_tax),
              l_discount,
              l_tax,
              1);

          agg_groups[key] += elem;
      }
    }

    {
      NamedTimer t(__func__, "aggregation");
      for (auto it = agg_groups.begin();
           it != agg_groups.end(); ++it) {
          const lineitem_key &key = it->first;

          results.push_back(q1entry(
              key.l_returnflag(),
              key.l_linestatus(),
              it->second.l_quantity,
              it->second.l_extendedprice,
              it->second.disc_price,
              it->second.charge,
              double(it->second.l_quantity) / double(it->second.count),
              double(it->second.l_extendedprice) / double(it->second.count),
              double(it->second.l_discount) / double(it->second.count),
              it->second.count));
      }
    }
}

static void do_query_q2(ConnectNew &conn,
                        uint64_t size,
                        const string &type,
                        const string &name,
                        vector<q2entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s << "select SQL_NO_CACHE s_acctbal, s_name, n_name, p_partkey, p_mfgr, s_address, s_phone, s_comment "
      "from PART_INT, SUPPLIER_INT, PARTSUPP_INT, NATION_INT, REGION_INT "
      "where p_partkey = ps_partkey and s_suppkey = ps_suppkey and p_size = " << size << " and p_type like '%" << type << "' and s_nationkey = n_nationkey and n_regionkey = r_regionkey and r_name = '" << name << "' and ps_supplycost = ( select min(ps_supplycost) from "
      "PARTSUPP_INT, SUPPLIER_INT, NATION_INT, REGION_INT where p_partkey = ps_partkey and s_suppkey = ps_suppkey and s_nationkey = n_nationkey and n_regionkey = r_regionkey and r_name = '" << name << "') order by s_acctbal desc, n_name, s_name, p_partkey limit 100";
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
          q2entry(resultFromStr<double>(row[0].data) / 100.0,
                  row[1].data,
                  row[2].data,
                  resultFromStr<uint64_t>(row[3].data),
                  row[4].data,
                  row[5].data,
                  row[6].data,
                  row[7].data));
    }

}

static void do_query_q2_noopt(ConnectNew &conn,
                              CryptoManager &cm,
                              uint64_t size,
                              const string &type,
                              const string &name,
                              vector<q2entry> &results) {
    assert(name.size() <= 26);

    crypto_manager_stub cm_stub(&cm, UseOldOpe);
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

    DBResultNew * dbres;
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

template <typename T, typename Comparator = less<T> >
class topN_list {
public:
  //typedef typename list<T>::iterator iterator;
  typedef typename list<T>::const_iterator const_iterator;

  topN_list(size_t n, Comparator c = Comparator())
    : n(n), c(c) { assert(n > 0); }

  // true if elem was added, false if it was dropped (b/c it was not
  // top N)
  bool add_elem(const T& elem) {
    if (elems.empty()) {
      elems.push_back(elem);
      return true;
    }

    // check to see if we can discard this candidate
    // (linear search bestSoFar list, assuming topN size is small)
    auto it = elems.begin();
    for (; it != elems.end(); ++it) {
      if (c(elem, *it)) {
        break;
      }
    }

    if (it == elems.end()) {
      // cur group is smaller than every element on list
      if (elems.size() < n) {
        elems.push_back(elem);
        return true;
      } else { /* drop */ return false; }
    } else {
      elems.insert(it, elem);
      if (elems.size() > n) {
        elems.resize(n);
      }
      return true;
    }
  }

  inline const_iterator begin() const { return elems.begin(); }
  inline const_iterator end() const   { return elems.end();   }
private:
  size_t n;
  Comparator c;
  list<T> elems;
};

template <typename InputIterator, typename OutputIterator, typename Comparator>
void n_way_merge(const vector<pair<InputIterator, InputIterator> >& input_sources,
                 OutputIterator output_sink, Comparator c = Comparator(),
                 size_t limit = (size_t)-1) {
  vector<InputIterator> positions;
  positions.reserve(input_sources.size());
  for (auto p : input_sources) positions.push_back(p.first);
  size_t n = 0;
  while (n < limit) {
    // look for next min
    typename InputIterator::pointer minSoFar = NULL;
    size_t minSoFarIdx = 0;
    for (size_t i = 0; i < positions.size(); i++) {
      if (positions[i] != input_sources[i].second) {
        if (!minSoFar || c(*positions[i], *minSoFar)) {
          minSoFar = &(*positions[i]);
          minSoFarIdx = i;
        }
      }
    }
    if (!minSoFar) break;
    positions[minSoFarIdx]++;
    *output_sink++ = *minSoFar;
    n++;
  }
}

struct q3entry_enc_cmp {
  inline bool operator()(const q3entry_enc& a,
                         const q3entry_enc& b) const {
    // TODO: floating point equality errors
    return (a.revenue > b.revenue) ||
           (a.revenue == b.revenue && a.o_orderdate_OPE < b.o_orderdate_OPE);
  }
};

struct _q3_noopt_task_state {
  _q3_noopt_task_state() : bestSoFar(10) {}
  vector< vector< SqlItem > > rows;

  topN_list<q3entry_enc, q3entry_enc_cmp> bestSoFar;
  static const size_t TopN = 10;
};

struct _q3_noopt_task {
  void operator()() {
      for (auto row : state->rows) {
          vector<string> ciphers;
          tokenize(row[1].data, ",", ciphers);
          assert(!ciphers.empty());

          double sum = 0.0;
          for (size_t i = 0; i < ciphers.size(); i++) {
            uint64_t l_disc_price_int = decryptRow<uint64_t, 7>(
                    ciphers[i],
                    12345,
                    fieldname(0, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    *cm);
            double l_disc_price = ((double)l_disc_price_int)/100.0;

            sum += l_disc_price;
          }

          q3entry_enc cur_group(
                  resultFromStr<uint64_t>(row[0].data),
                  sum,
                  resultFromStr<uint64_t>(row[2].data),
                  resultFromStr<uint64_t>(row[3].data),
                  resultFromStr<uint64_t>(row[4].data));

          state->bestSoFar.add_elem(cur_group);
      }
  }
  CryptoManager* cm;
  _q3_noopt_task_state* state;
};

static void do_query_q3(ConnectNew &conn,
                        const string& mktsegment,
                        const string& d,
                        vector<q3entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select l_orderkey, sum(l_extendedprice * (100 - l_discount)) as revenue, o_orderdate, o_shippriority, group_concat('a') "
      "from LINEITEM_INT straight_join ORDERS_INT on l_orderkey = o_orderkey straight_join CUSTOMER_INT on c_custkey = o_custkey "
      "where c_mktsegment = '" << mktsegment << "' and o_orderdate < date '" << d << "' and l_shipdate > date '" << d << "' "
      "group by l_orderkey, o_orderdate, o_shippriority "
      "order by revenue desc, o_orderdate limit 10;"
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
          q3entry(
            resultFromStr<uint64_t>(row[0].data),
            resultFromStr<double>(row[1].data) / 10000.0,
            row[2].data,
            resultFromStr<uint64_t>(row[3].data)));
    }
}

static void do_query_q3_crypt(ConnectNew &conn,
                              CryptoManager &cm,
                              const string& mktsegment,
                              const string& d,
                              vector<q3entry> &results) {

    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin = false;
    string encMKT = cm_stub.crypt(cm.getmkey(), mktsegment, TYPE_TEXT,
                              fieldname(customer::c_mktsegment, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(isBin);
    assert(encMKT.size() == mktsegment.size());
    encMKT.resize(10);

    string enc_d_o_orderdate =
      cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d)),
                              TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    string enc_d_l_shipdate =
      cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    DBResultNew * dbres;

    ostringstream s;
    s << "select "
         "  l_orderkey_DET, "
         "  group_concat(l_disc_price_DET), "
         "  o_orderdate_OPE, "
         "  o_orderdate_DET, "
         "  o_shippriority_DET "
         "from lineitem_enc_rowid straight_join orders_enc on l_orderkey_DET = o_orderkey_DET straight_join customer_enc_rowid on c_custkey_DET = o_custkey_DET "
         "where "
         "  c_mktsegment_DET = " << marshallBinary(encMKT) <<
         "  and o_orderdate_OPE < " << enc_d_o_orderdate <<
         "  and l_shipdate_OPE > " << enc_d_l_shipdate <<
         " group by l_orderkey_DET, o_orderdate_DET, o_shippriority_DET";
    cerr << s.str() << endl;

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

    if (DoParallel) {
      NamedTimer t(__func__, "decrypt-parallel");

      using namespace exec_service;

      vector<_q3_noopt_task_state> states( NumThreads );
      vector<_q3_noopt_task> tasks( NumThreads );
      vector<SqlRowGroup> groups;
      SplitRowsIntoGroups(groups, res.rows, NumThreads);

      for (size_t i = 0; i < NumThreads; i++) {
        tasks[i].cm    = &cm;
        tasks[i].state = &states[i];
        states[i].rows = groups[i];
      }

      Exec<_q3_noopt_task>::DefaultExecutor exec( NumThreads );
      exec.start();
      for (size_t i = 0; i < NumThreads; i++) {
        exec.submit(tasks[i]);
      }
      exec.stop(); // blocks until completion

      // merge the topN lists from the threads, and add the results (
      // remembering to do the final decryptions)

      typedef topN_list<q3entry_enc, q3entry_enc_cmp>::const_iterator iter;
      vector<pair<iter, iter> > positions;
      positions.resize( NumThreads );
      for (size_t i = 0; i < NumThreads; i++) {
        positions[i].first  = states[i].bestSoFar.begin();
        positions[i].second = states[i].bestSoFar.end();
      }

      vector<q3entry_enc> merged(_q3_noopt_task_state::TopN);
      n_way_merge(positions, merged.begin(), q3entry_enc_cmp(),
                  _q3_noopt_task_state::TopN);

      // add the entries to the results
      for (auto it = merged.begin(); it != merged.end(); ++it) {

        // l_orderkey
        uint64_t l_orderkey = decryptRowFromTo<uint64_t, 4>(
                to_s(it->l_orderkey_DET),
                12345,
                fieldname(lineitem::l_orderkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
                getMin(oDET),
                cm);

        // o_orderdate
        uint32_t o_orderdate_PT = decryptRow<uint32_t, 3>(
                to_s(it->o_orderdate_DET),
                12345,
                fieldname(orders::o_orderdate, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

        // o_shippriority
        uint64_t o_shippriority = decryptRow<uint64_t, 4>(
                to_s(it->o_shippriority_DET),
                12345,
                fieldname(orders::o_shippriority, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

        results.push_back(
            q3entry(
              l_orderkey,
              it->revenue,
              stringify_date_from_encoding(o_orderdate_PT),
              o_shippriority));
      }

    } else {
      assert(false); // not implemented
    }

}

static void do_query_q4(ConnectNew &conn,
                        const string& d,
                        vector<q4entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select o_orderpriority, count(*) as order_count "
      "from ORDERS_INT "
      "where o_orderdate >= date '" << d << "' "
      "and o_orderdate < date '" << d << "' + interval '3' month "
      "and exists ( "
      "    select * from "
      "      LINEITEM_INT where l_orderkey = o_orderkey and l_commitdate < l_receiptdate) "
      "group by o_orderpriority "
      "order by o_orderpriority"
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
          q4entry(
            row[0].data,
            resultFromStr<uint64_t>(row[1].data)));
    }
}

static void do_query_crypt_q4(ConnectNew &conn,
                              CryptoManager &cm,
                              const string& d,
                              vector<q4entry> &results) {

    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin;
    string d0 =
      cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d)),
                              TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    date_t endDate(d);
    endDate.add_months(3);
    string d1 =
      cm_stub.crypt<3>(cm.getmkey(), to_s(endDate.encode()),
                              TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    ostringstream s;
    s <<
      "select o_orderpriority_DET, count(*) as order_count "
      "from orders_enc "
      "where o_orderdate_OPE >= " << d0 << " "
      "and o_orderdate_OPE < " << d1 << " "
      "and exists ( "
      "    select * from "
      "      lineitem_enc_rowid where l_orderkey_DET = o_orderkey_DET and l_commitdate_OPE < l_receiptdate_OPE) "
      "group by o_orderpriority_DET "
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

      string o_orderpriority = decryptRow<string>(
              row[0].data,
              12345,
              fieldname(orders::o_orderpriority, "DET"),
              TYPE_TEXT,
              oDET,
              cm);

      // TODO: sort
      results.push_back(
          q4entry(
            o_orderpriority,
            resultFromStr<uint64_t>(row[1].data)));
    }
}

static void do_query_q5(ConnectNew &conn,
                        const string& name,
                        const string& d,
                        vector<q5entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select"
      "  n_name,"
      "  sum(l_extendedprice * (100 - l_discount)) as revenue "
      "from"
      "  CUSTOMER_INT,"
      "  ORDERS_INT,"
      "  LINEITEM_INT,"
      "  SUPPLIER_INT,"
      "  NATION_INT,"
      "  REGION_INT "
      "where"
      "  c_custkey = o_custkey"
      "  and l_orderkey = o_orderkey"
      "  and l_suppkey = s_suppkey"
      "  and c_nationkey = s_nationkey"
      "  and s_nationkey = n_nationkey"
      "  and n_regionkey = r_regionkey"
      "  and r_name = '" << name << "'"
      "  and o_orderdate >= date '" << d << "'"
      "  and o_orderdate < date '" << d << "' + interval '1' year "
      "group by"
      "  n_name "
      "order by"
      "  revenue desc;"
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
          q5entry(
            row[0].data,
            resultFromStr<double>(row[1].data) / 10000.0));
    }
}

struct _q5_noopt_task_state {
  _q5_noopt_task_state() {}
  vector< string > xps;
  vector< double > revenue;
};

struct _q5_noopt_task {
  void operator()() {
    for (size_t i = 0; i < state->xps.size(); i++) {

        // l_disc_price
        uint64_t l_disc_price_int = decryptRow<uint64_t, 7>(
                state->xps[i],
                12345,
                fieldname(0, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);
        double l_disc_price = ((double)l_disc_price_int)/100.0;

        state->revenue.push_back(l_disc_price);
    }
  }
  CryptoManager* cm;
  _q5_noopt_task_state* state;
};

static void do_query_crypt_q5(ConnectNew &conn,
                              CryptoManager &cm,
                              const string& name,
                              const string& d,
                              vector<q5entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin = false;
    string encNAME = cm_stub.crypt(cm.getmkey(), name, TYPE_TEXT,
                              fieldname(region::r_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(name.size() == encNAME.size());
    assert(isBin);

    // right-pad encNAME with 0's
    encNAME.resize(25);

    string d0 =
      cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d)),
                              TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string d1 =
      cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d) + (1 << 9) /* + 1 year */),
                              TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    ostringstream s;
    s <<
      "select"
      "  n_name_DET,"
      " group_concat(l_disc_price_DET) "
      "from"
      "  customer_enc_rowid,"
      "  orders_enc,"
      "  lineitem_enc_rowid,"
      "  supplier_enc,"
      "  nation_enc,"
      "  region_enc "
      "where"
      "  c_custkey_DET = o_custkey_DET"
      "  and l_orderkey_DET = o_orderkey_DET"
      "  and l_suppkey_DET = s_suppkey_DET"
      "  and c_nationkey_DET = s_nationkey_DET"
      "  and s_nationkey_DET = n_nationkey_DET"
      "  and n_regionkey_DET = r_regionkey_DET"
      "  and r_name_DET = " << marshallBinary(encNAME) << " "
      "  and o_orderdate_OPE >= " << d0 << " "
      "  and o_orderdate_OPE < " << d1 << " "
      "group by"
      "  n_name_DET"
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

    assert(DoParallel);
    vector<string> cts;
    vector< pair<size_t, size_t> > offsets;
    for (auto row : res.rows) {
      size_t offset = cts.size();
      tokenize(row[1].data, ",", cts);

      offsets.push_back(
          make_pair(offset, cts.size() - offset));
    }

    vector< vector<string> > xp_groups;
    SplitRowsIntoGroups(xp_groups, cts, NumThreads);

    vector<_q5_noopt_task_state> states( NumThreads );
    vector<_q5_noopt_task> tasks( NumThreads );

    for (size_t i = 0; i < NumThreads; i++) {
      tasks[i].cm    = &cm;
      tasks[i].state = &states[i];
      states[i].xps  = xp_groups[i];
    }

    using namespace exec_service;
    Exec<_q5_noopt_task>::DefaultExecutor exec( NumThreads );
    exec.start();
    for (size_t i = 0; i < NumThreads; i++) {
      exec.submit(tasks[i]);
    }
    exec.stop();

    vector<double> revenue;
    for (size_t i = 0; i < NumThreads; i++) {
      revenue.insert(revenue.end(), states[i].revenue.begin(), states[i].revenue.end());
    }

    size_t i = 0;
    for (auto row : res.rows) {
      string n_name = decryptRow<string>(
          row[0].data,
          12345,
          fieldname(nation::n_name, "DET"),
          TYPE_TEXT,
          oDET,
          cm);

      double r = 0.0;
      for (auto it = revenue.begin() + offsets[i].first;
           it != revenue.begin() + offsets[i].first + offsets[i].second;
           ++it) { r += *it; }

      results.push_back(q5entry(n_name, r));
      i++;
    }
}

static void do_query_q6(ConnectNew &conn,
                        const string& d,
                        double discount,
                        uint64_t quantity,
                        vector<q6entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select"
      "  sum(l_extendedprice * l_discount) as revenue "
      "from"
      "  LINEITEM_INT "
      "where"
      "  l_shipdate >= date '" << d << "'"
      "  and l_shipdate < date '" << d << "' + interval '1' year"
      "  and l_discount between " << roundToLong(discount*100.0)
        << " - 1 and " << roundToLong(discount*100.0) << " + 1.0"
      "  and l_quantity < " << roundToLong (quantity * 100.0 )
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
          q6entry(
            resultFromStr<double>(row[0].data) / 10000.0));
    }
}

static void do_query_crypt_q6(ConnectNew &conn,
                              CryptoManager &cm,
                              const string& d,
                              double discount,
                              uint64_t quantity,
                              vector<q6entry> &results,
                              const string& db) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin = false;
    string d0 = cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);

    string d1 = cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d) + (1 << 9) /* + 1 yr */),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);

    string dsc0 = cm_stub.crypt<7>(
        cm.getmkey(), to_s(roundToLong( (discount - 0.01) * 100.0 )),
        TYPE_INTEGER, fieldname(lineitem::l_discount, "OPE"),
        getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(isBin);
    dsc0.resize( 16 ); dsc0 = str_reverse( dsc0 );

    isBin = false;
    string dsc1 = cm_stub.crypt<7>(
        cm.getmkey(), to_s(roundToLong( (discount + 0.01) * 100.0 )),
        TYPE_INTEGER, fieldname(lineitem::l_discount, "OPE"),
        getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(isBin);
    dsc1.resize( 16 ); dsc1 = str_reverse( dsc1 );

    isBin = false;
    string qty = cm_stub.crypt<7>(
        cm.getmkey(), to_s(roundToLong( double(quantity) * 100.0 )),
        TYPE_INTEGER, fieldname(lineitem::l_quantity, "OPE"),
        getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(isBin);
    qty.resize( 16 ); qty = str_reverse( qty );

    string filename =
      "/space/stephentu/data/" + db + "/lineitem_enc/row_pack/revenue";

    string pkinfo = marshallBinary(cm.getPKInfo());

    ostringstream s;
    s <<
      "select"
      " agg_hash_agg_row_col_pack(0, row_id, "
        << pkinfo << ", " << "\"" << filename << "\", "
        << NumThreads << ", 256, 12, 1) "
      "from"
      "  lineitem_enc_rowid "
      "where"
      "  l_shipdate_OPE >= " << d0 << " "
      "  and l_shipdate_OPE < " << d1 << " "
      "  and l_discount_OPE >= " << marshallBinary(dsc0) << " "
      " and l_discount_OPE <= " << marshallBinary(dsc1) << " "
      "  and l_quantity_OPE < " << marshallBinary(qty)
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

    static const size_t BitsPerAggField = 83;
    ZZ mask = to_ZZ(1); mask <<= BitsPerAggField; mask -= 1;
    assert(NumBits(mask) == (int)BitsPerAggField);
    {
      assert(res.rows.size() == 1);
      string data = res.rows[0][0].data;

      double revenue = 0.0;

      const uint8_t *p   = (const uint8_t *) data.data();
      const uint8_t *end = (const uint8_t *) data.data() + data.size();
      while (p < end) {

        // skip over count
        p += sizeof(uint64_t);

#define TAKE_FROM_SLOT(z, slot) \
        (to_long(((z) >> (BitsPerAggField * (slot))) & mask))

        const uint32_t *u32p = (const uint32_t *) p;
        uint32_t n_aggs = *u32p;
        p += sizeof(uint32_t);

        for (size_t group_i = 0; group_i < n_aggs; group_i++) {
          // interest mask
          const uint32_t *u32p = (const uint32_t *) p;
          uint32_t interest_mask = *u32p;
          p += sizeof(uint32_t);

          string ct = string((const char *) p, (size_t) 256);
          ZZ m;
          cm.decrypt_Paillier(ct, m);
          p += 256;

          for (size_t i = 0; i < 12; i++) {
            if (!(interest_mask & (0x1 << i))) continue;
            long l = TAKE_FROM_SLOT(m, i);
            revenue += ((double)l)/100.0;
          }
        }
      }

      results.push_back( revenue );
    }
}

static void do_query_q7(ConnectNew &conn,
                        const string& n_a,
                        const string& n_b,
                        vector<q7entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
    "select"
    "  sum_hash_agg_int("
    "   n1.n_name as supp_nation,"
    "   n2.n_name as cust_nation,"
    "   extract(year from l_shipdate) as l_year,"
    "    l_extendedprice * (100 - l_discount) as volume) "
    "from"
    "  SUPPLIER_INT,"
    "  LINEITEM_INT,"
    "  ORDERS_INT,"
    "  CUSTOMER_INT,"
    "  NATION_INT n1,"
    "  NATION_INT n2 "
    "where"
    "  s_suppkey = l_suppkey"
    "  and o_orderkey = l_orderkey"
    "  and c_custkey = o_custkey"
    "  and s_nationkey = n1.n_nationkey"
    "  and c_nationkey = n2.n_nationkey"
    "  and ("
    "    (n1.n_name = '" << n_a << "' and n2.n_name = '" << n_b << "')"
    "    or (n1.n_name = '" << n_b << "' and n2.n_name = '" << n_a << "')"
    "  )"
    "  and l_shipdate between date '1995-01-01' and date '1996-12-31'";
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

    assert(res.rows.size() == 1);
    const string& data = res.rows[0][0].data;
    const uint8_t *p   = (const uint8_t *) data.data();
    const uint8_t *end = (const uint8_t *) data.data() + data.size();
    while (p < end) {
      string supp_nation = ReadKeyElem<string>(p);
      string cust_nation = ReadKeyElem<string>(p);
      uint64_t l_year    = ReadKeyElem<int64_t>(p);

      // skip count
      p += sizeof(uint64_t);

      const uint64_t *dp = (const uint64_t *) p;
      uint64_t value = *dp;
      p += sizeof(uint64_t);

      results.push_back(
          q7entry(
            supp_nation,
            cust_nation,
            l_year,
            double(value) / 10000.0));
    }
}

static void do_query_crypt_q7(ConnectNew &conn,
                              CryptoManager &cm,
                              const string& n_a,
                              const string& n_b,
                              vector<q7entry> &results,
                              const string& db) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin = false;
    string d0 = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, 1995)),
                                   TYPE_INTEGER, "ope_join",
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);
    string d1 = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(12, 31, 1996)),
                                   TYPE_INTEGER, "ope_join",
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);

    string n_a_enc = cm_stub.crypt(cm.getmkey(), n_a, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(isBin);
    assert(n_a.size() == n_a_enc.size());
    n_a_enc.resize(25);

    isBin = false;
    string n_b_enc = cm_stub.crypt(cm.getmkey(), n_b, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(isBin);
    assert(n_b.size() == n_b_enc.size());
    n_b_enc.resize(25);

    string pkinfo = marshallBinary(cm.getPKInfo());
    string filename =
      "/space/stephentu/data/" + db + "/lineitem_enc/row_pack/disc_price";

    ostringstream s;
    s <<
    "select"
    "  agg_hash_agg_row_col_pack("
    "   3,"
    "   n1.n_name_DET,"
    "   n2.n_name_DET,"
    "   l_shipdate_year_DET,"
    "   l.row_id," << pkinfo << ", " <<
    "\"" << filename << "\", " << NumThreads << ", 256, 12, "
    "    1) "
    "from"
    "  supplier_enc,"
    "  lineitem_enc_rowid l,"
    "  orders_enc,"
    "  customer_enc_rowid c,"
    "  nation_enc n1,"
    "  nation_enc n2 "
    "where"
    "  s_suppkey_DET = l_suppkey_DET"
    "  and o_orderkey_DET = l_orderkey_DET"
    "  and c_custkey_DET = o_custkey_DET"
    "  and s_nationkey_DET = n1.n_nationkey_DET"
    "  and c_nationkey_DET = n2.n_nationkey_DET"
    "  and ("
    "    (n1.n_name_DET = " << marshallBinary(n_a_enc)
    << " and n2.n_name_DET = " << marshallBinary(n_b_enc) << ")"
    "    or (n1.n_name_DET = " << marshallBinary(n_b_enc)
    << " and n2.n_name_DET = " << marshallBinary(n_a_enc) << ")"
    "  )"
    "  and l_shipdate_OPE >= " << d0 << " and l_shipdate_OPE <= " << d1;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

    static const size_t BitsPerAggField = 83;
    ZZ mask = to_ZZ(1); mask <<= BitsPerAggField; mask -= 1;
    assert(NumBits(mask) == (int)BitsPerAggField);
    assert(res.rows.size() == 1);
    const string& data = res.rows[0][0].data;
    const uint8_t *p   = (const uint8_t *) data.data();
    const uint8_t *end = (const uint8_t *) data.data() + data.size();
    while (p < end) {
      string supp_nation_DET = ReadKeyElem<string>(p);
      string cust_nation_DET = ReadKeyElem<string>(p);
      uint64_t l_year_DET    = ReadKeyElem<int64_t>(p);

      string supp_nation = decryptRow<string>(
              supp_nation_DET,
              12345,
              fieldname(nation::n_name, "DET"),
              TYPE_TEXT,
              oDET,
              cm);

      string cust_nation = decryptRow<string>(
              cust_nation_DET,
              12345,
              fieldname(nation::n_name, "DET"),
              TYPE_TEXT,
              oDET,
              cm);

      uint64_t l_year = decryptRow<uint64_t, 2>(
              to_s(l_year_DET),
              12345,
              fieldname(0, "DET"),
              TYPE_INTEGER,
              oDET,
              cm);

      // skip over count
      p += sizeof(uint64_t);

#define TAKE_FROM_SLOT(z, slot) \
        (to_long(((z) >> (BitsPerAggField * (slot))) & mask))

      const uint32_t *u32p = (const uint32_t *) p;
      uint32_t n_aggs = *u32p;
      p += sizeof(uint32_t);

      double sum = 0.0;
      for (size_t group_i = 0; group_i < n_aggs; group_i++) {
        // interest mask
        const uint32_t *u32p = (const uint32_t *) p;
        uint32_t interest_mask = *u32p;
        p += sizeof(uint32_t);

        string ct = string((const char *) p, (size_t) 256);
        ZZ m;
        cm.decrypt_Paillier(ct, m);
        p += 256;

        for (size_t i = 0; i < 12; i++) {
          if (!(interest_mask & (0x1 << i))) continue;
          long l = TAKE_FROM_SLOT(m, i);
          sum += ((double)l)/100.0;
        }
      }

      results.push_back(
          q7entry(supp_nation, cust_nation, l_year, sum));
    }
}

static void do_query_q8(ConnectNew &conn,
                        const string& n_a,
                        const string& r_a,
                        const string& p_a,
                        vector<q8entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select"
      "  o_year,"
      "  sum(case"
      "    when nation = '" << n_a << "' then volume"
      "    else 0"
      "  end) / sum(volume) as mkt_share "
      "from"
      "  ("
      "    select"
      "      extract(year from o_orderdate) as o_year,"
      "      l_extendedprice * (100 - l_discount) as volume,"
      "      n2.n_name as nation"
      "    from"
      "      PART_INT,"
      "      SUPPLIER_INT,"
      "      LINEITEM_INT,"
      "      ORDERS_INT,"
      "      CUSTOMER_INT,"
      "      NATION_INT n1,"
      "      NATION_INT n2,"
      "      REGION_INT"
      "    where"
      "      p_partkey = l_partkey"
      "      and s_suppkey = l_suppkey"
      "      and l_orderkey = o_orderkey"
      "      and o_custkey = c_custkey"
      "      and c_nationkey = n1.n_nationkey"
      "      and n1.n_regionkey = r_regionkey"
      "      and r_name = '" << r_a << "'"
      "      and s_nationkey = n2.n_nationkey"
      "      and o_orderdate between date '1995-01-01' and date '1996-12-31'"
      "      and p_type = '" << p_a <<"'"
      "  ) as all_nations "
      "group by"
      "  o_year "
      "order by"
      "  o_year;";
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
          q8entry(
            resultFromStr<uint64_t>(row[0].data),
            resultFromStr<double>(row[1].data)));
    }
}

struct _q8_noopt_task_state {
  _q8_noopt_task_state() {}
  vector< vector< SqlItem > > rows;
  map<uint64_t, pair<double, double> > aggState;
};

struct _q8_noopt_task {
  void operator()() {
      for (auto row : state->rows) {
        uint64_t offset = resultFromStr<uint64_t>(row[0].data);

        uint64_t l_disc_price_int = decryptRow<uint64_t, 7>(
                row[1].data,
                12345,
                fieldname(0, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);
        double l_disc_price = ((double)l_disc_price_int)/100.0;

        double value = l_disc_price;

        bool flag = row[2].data == "1";

        uint64_t key = 1995 + offset;
        auto it = state->aggState.find(key);
        if ((it == state->aggState.end())) {
          pair<double, double>& p = state->aggState[key];
          p.first = flag ? value : 0.0;
          p.second = value;
        } else {
          if (flag) it->second.first += value;
          it->second.second += value;
        }
      }
  }
  CryptoManager* cm;
  _q8_noopt_task_state* state;
};

static void do_query_crypt_q8(ConnectNew &conn,
                              CryptoManager &cm,
                              const string& n_a,
                              const string& r_a,
                              const string& p_a,
                              vector<q8entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin = false;
    string d0 = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, 1995)),
                                   TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);
    string d1 = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(12, 31, 1996)),
                                   TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);

    string boundary1 = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, 1996)),
                                   TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    string n_a_enc = cm_stub.crypt(cm.getmkey(), n_a, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(isBin);
    assert(n_a.size() == n_a_enc.size());
    n_a_enc.resize(25);

    isBin = false;
    string r_a_enc = cm_stub.crypt(cm.getmkey(), r_a, TYPE_TEXT,
                              fieldname(region::r_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(isBin);
    assert(r_a.size() == r_a_enc.size());
    r_a_enc.resize(25);

    isBin = false;
    string p_a_enc = cm_stub.crypt(cm.getmkey(), p_a, TYPE_TEXT,
                              fieldname(part::p_type, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(isBin);
    assert(p_a.size() == p_a_enc.size());

    ostringstream s;
    s <<
      "select"
      "  IF(o_orderdate_OPE < " << boundary1 << ", 0, 1),"
      "  l_disc_price_DET,"
      "  n2.n_name_DET = " << marshallBinary(n_a_enc) << " "
      "from"
      "  part_enc,"
      "  supplier_enc,"
      "  lineitem_enc_rowid,"
      "  orders_enc,"
      "  customer_enc_rowid,"
      "  nation_enc n1,"
      "  nation_enc n2,"
      "  region_enc "
      "where"
      "  p_partkey_DET = l_partkey_DET"
      "  and s_suppkey_DET = l_suppkey_DET"
      "  and l_orderkey_DET = o_orderkey_DET"
      "  and o_custkey_DET = c_custkey_DET"
      "  and c_nationkey_DET = n1.n_nationkey_DET"
      "  and n1.n_regionkey_DET = r_regionkey_DET"
      "  and r_name_DET = " << marshallBinary(r_a_enc) << " "
      "  and s_nationkey_DET = n2.n_nationkey_DET"
      "  and o_orderdate_OPE >= " << d0 << " and o_orderdate_OPE <= " << d1 <<
      "  and p_type_DET = " << marshallBinary(p_a_enc)
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
      assert(DoParallel);
      using namespace exec_service;
      vector<_q8_noopt_task_state> states( NumThreads );
      vector<_q8_noopt_task> tasks( NumThreads );
      vector<SqlRowGroup> groups;
      SplitRowsIntoGroups(groups, res.rows, NumThreads);

      for (size_t i = 0; i < NumThreads; i++) {
        tasks[i].cm    = &cm;
        tasks[i].state = &states[i];
        states[i].rows = groups[i];
      }

      Exec<_q8_noopt_task>::DefaultExecutor exec( NumThreads );
      exec.start();
      for (size_t i = 0; i < NumThreads; i++) {
        exec.submit(tasks[i]);
      }
      exec.stop(); // blocks until completion

      map<uint64_t, pair<double, double> > merged;
      for (size_t i = 0; i < NumThreads; i++) {
        for (auto it = states[i].aggState.begin();
             it != states[i].aggState.end(); ++it) {
          auto it0 = merged.find(it->first);
          if ((it0 == merged.end())) {
            merged[it->first] = it->second;
          } else {
            it0->second.first  += it->second.first;
            it0->second.second += it->second.second;
          }
        }
      }

      for (auto it = merged.begin(); it != merged.end(); ++it) {
        results.push_back(
            q8entry(
              it->first,
              it->second.first / it->second.second));
      }
    }
}

static void do_query_q9(ConnectNew &conn,
                        const string& p_a,
                        vector<q9entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select "
      "  nation, "
      "  o_year, "
      "  sum(amount) as sum_profit "
      "from "
      "  ( "
      "    select "
      "      n_name as nation, "
      "      extract(year from o_orderdate) as o_year, "
      "      l_extendedprice * (100 - l_discount) - ps_supplycost * l_quantity as amount "
      "    from "
      "      PART_INT, "
      "      SUPPLIER_INT, "
      "      LINEITEM_INT, "
      "      PARTSUPP_INT, "
      "      ORDERS_INT, "
      "      NATION_INT "
      "    where "
      "      s_suppkey = l_suppkey "
      "      and ps_suppkey = l_suppkey "
      "      and ps_partkey = l_partkey "
      "      and p_partkey = l_partkey "
      "      and o_orderkey = l_orderkey "
      "      and s_nationkey = n_nationkey "
      "      and p_name like '%" << p_a << "%' "
      "  ) as profit "
      "group by "
      "  nation, "
      "  o_year "
      "order by "
      "  nation, "
      "  o_year desc; ";
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
          q9entry(
            row[0].data,
            resultFromStr<uint64_t>(row[1].data),
            resultFromStr<double>(row[2].data)/10000.0));
    }
}

static string make_if_predicate(
    size_t cur, const vector<string>& boundaries, const string& ope_field) {
  assert(cur <= boundaries.size());
  ostringstream buf;
  if (cur == boundaries.size()) {
    buf << cur;
  } else {
    buf << "IF(" << ope_field << " < " << boundaries[cur] << ", " << cur << ", "
        << make_if_predicate(cur + 1, boundaries, ope_field) << ")";
  }
  return buf.str();
}

struct _q9_noopt_task_state {
  _q9_noopt_task_state() {}
  vector< vector< SqlItem > > rows;
  map< pair<string, uint64_t> , double> aggState;
};

struct _q9_noopt_task {
  void operator()() {
    for (auto row : state->rows) {
      pair<string, uint64_t> key =
        make_pair( row[0].data, resultFromStr<uint64_t>(row[1].data) + 1992 );

      uint64_t l_disc_price_int = decryptRow<uint64_t, 7>(
          row[2].data,
          12345,
          fieldname(0, "DET"),
          TYPE_INTEGER,
          oDET,
          *cm);
      double l_disc_price = ((double)l_disc_price_int)/100.0;

      uint64_t ps_supplycost_int = decryptRow<uint64_t, 7>(
          row[3].data,
          12345,
          fieldname(partsupp::ps_supplycost, "DET"),
          TYPE_INTEGER,
          oDET,
          *cm);
      double ps_supplycost = ((double)ps_supplycost_int)/100.0;

      uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
          row[4].data,
          12345,
          fieldname(lineitem::l_quantity, "DET"),
          TYPE_INTEGER,
          oDET,
          *cm);
      double l_quantity = ((double)l_quantity_int)/100.0;

      double value = l_disc_price - ps_supplycost * l_quantity;

      auto it = state->aggState.find(key);
      if (it == state->aggState.end()) {
        state->aggState[key] = value;
      } else {
        it->second += value;
      }
    }
  }
  CryptoManager* cm;
  _q9_noopt_task_state* state;
};

static void do_query_crypt_q9(ConnectNew &conn,
                              CryptoManager& cm,
                              const string& p_a,
                              vector<q9entry> &results) {
  crypto_manager_stub cm_stub(&cm, UseOldOpe);
  NamedTimer fcnTimer(__func__);

  // min order year (inclusive)
  static const size_t MinOrderYear = 1992;

  // max order year (inclusive)
  static const size_t MaxOrderYear = 1998;

  vector<string> boundaries;
  for (size_t i = MinOrderYear + 1; i <= MaxOrderYear; i++) {
    bool isBin = false;
    string b = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, i)),
                                TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                                getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);
    boundaries.push_back(b);
  }

  Binary key(cm.getKey(cm.getmkey(), fieldname(part::p_name, "SWP"), SECLEVEL::SWP));
  Token t = CryptoManager::token(key, Binary(p_a));

  ostringstream s;
  s <<
    "select "
    "  n_name_DET, " << make_if_predicate(0, boundaries, "o_orderdate_OPE") << ", "
    "  l_disc_price_DET, ps_supplycost_DET, l_quantity_DET "
    "from "
    "  part_enc, "
    "  supplier_enc, "
    "  lineitem_enc_rowid, "
    "  partsupp_enc, "
    "  orders_enc, "
    "  nation_enc "
    "where "
    "  s_suppkey_DET = l_suppkey_DET "
    "  and ps_suppkey_DET = l_suppkey_DET "
    "  and ps_partkey_DET = l_partkey_DET "
    "  and p_partkey_DET = l_partkey_DET "
    "  and o_orderkey_DET = l_orderkey_DET "
    "  and s_nationkey_DET = n_nationkey_DET "
    "  and searchSWP("
          << marshallBinary(string((char *)t.ciph.content, t.ciph.len))
          << ", "
          << marshallBinary(string((char *)t.wordKey.content, t.wordKey.len))
          << ", p_name_SWP) = 1";
  cerr << s.str() << endl;

  DBResultNew * dbres;
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

  assert(DoParallel);

  using namespace exec_service;

  vector<_q9_noopt_task_state> states( NumThreads );
  vector<_q9_noopt_task> tasks( NumThreads );
  vector<SqlRowGroup> groups;
  SplitRowsIntoGroups(groups, res.rows, NumThreads);

  for (size_t i = 0; i < NumThreads; i++) {
    tasks[i].cm    = &cm;
    tasks[i].state = &states[i];
    states[i].rows = groups[i];
  }

  Exec<_q9_noopt_task>::DefaultExecutor exec( NumThreads );
  exec.start();
  for (size_t i = 0; i < NumThreads; i++) {
    exec.submit(tasks[i]);
  }
  exec.stop();

  map< pair<string, uint64_t> , double> aggState;
  for (size_t i = 0; i < NumThreads; i++) {
    for (auto it = states[i].aggState.begin();
         it != states[i].aggState.end(); ++it) {
      auto it0 = aggState.find(it->first);
      if (it0 == aggState.end()) {
        aggState[it->first] = it->second;
      } else {
        it0->second += it->second;
      }
    }
  }
  for (auto it = aggState.begin(); it != aggState.end(); ++it) {
    string n_name = decryptRow<string>(
            it->first.first,
            12345,
            fieldname(nation::n_name, "DET"),
            TYPE_TEXT,
            oDET,
            cm);
    results.push_back(
        q9entry(
          n_name,
          it->first.second,
          it->second));
  }
}

static void do_query_q10(ConnectNew &conn,
                         const string& o_a,
                         vector<q10entry> &results) {
  NamedTimer fcnTimer(__func__);

  ostringstream s;
  s <<
    "select "
    "  c_custkey, "
    "  c_name, "
    "  sum(l_extendedprice * (100 - l_discount)) as revenue, "
    "  c_acctbal, "
    "  n_name, "
    "  c_address, "
    "  c_phone, "
    "  c_comment "
    "from "
    "LINEITEM_INT straight_join ORDERS_INT straight_join CUSTOMER_INT straight_join NATION_INT "
    "where "
    "  c_custkey = o_custkey "
    "  and l_orderkey = o_orderkey "
    "  and o_orderdate >= date '" << o_a << "' "
    "  and o_orderdate < date '" << o_a << "' + interval '3' month "
    "  and l_returnflag = 'R' "
    "  and c_nationkey = n_nationkey "
    "group by "
    "  c_custkey, "
    "  c_name, "
    "  c_acctbal, "
    "  c_phone, "
    "  n_name, "
    "  c_address, "
    "  c_comment "
    "order by "
    "  revenue desc "
    "limit 20;"
  ;
  cerr << s.str() << endl;

  DBResultNew * dbres;
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
        q10entry(
          resultFromStr<uint64_t>(row[0].data),
          row[1].data,
          resultFromStr<double>(row[2].data)/10000.0,
          resultFromStr<double>(row[3].data),
          row[4].data,
          row[5].data,
          row[6].data,
          row[7].data));
  }
}

struct q10entry_enc {
  q10entry_enc() {}
  q10entry_enc(
      uint64_t c_custkey_DET,
      const string& c_name_DET,
      uint64_t c_acctbal_DET,
      const string& n_name_DET,
      const string& c_address_DET,
      const string& c_phone_DET,
      const string& c_comment_DET,
      double revenue) :
    c_custkey_DET(c_custkey_DET),
    c_name_DET(c_name_DET),
    c_acctbal_DET(c_acctbal_DET),
    n_name_DET(n_name_DET),
    c_address_DET(c_address_DET),
    c_phone_DET(c_phone_DET),
    c_comment_DET(c_comment_DET),
    revenue(revenue) {}

  uint64_t c_custkey_DET;
  string c_name_DET;
  uint64_t c_acctbal_DET;
  string n_name_DET;
  string c_address_DET;
  string c_phone_DET;
  string c_comment_DET;
  double revenue;
};

struct q10entry_enc_cmp {
  inline bool operator()(
      const q10entry_enc& a,
      const q10entry_enc& b) const {
    return a.revenue > b.revenue;
  }
};

struct _q10_noopt_task_state {
  _q10_noopt_task_state() : bestSoFar(20) {}
  vector< vector< SqlItem > > rows;
  topN_list< q10entry_enc, q10entry_enc_cmp > bestSoFar;
};

struct _q10_noopt_task {
  void operator()() {
    for (auto row : state->rows) {
      vector<string> cts;
      tokenize(row[7].data, ",", cts);

      double sum = 0.0;
      for (size_t i = 0; i < cts.size(); i++) {
        uint64_t l_disc_price_int = decryptRow<uint64_t, 7>(
                cts[i],
                12345,
                fieldname(0, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);
        double l_disc_price = ((double)l_disc_price_int)/100.0;

        sum += l_disc_price;
      }

      q10entry_enc entry(
          resultFromStr<uint64_t>(row[0].data),
          row[1].data,
          resultFromStr<uint64_t>(row[2].data),
          row[3].data,
          row[4].data,
          row[5].data,
          row[6].data,
          sum);
      state->bestSoFar.add_elem(entry);
    }
  }
  CryptoManager* cm;
  _q10_noopt_task_state* state;
};

static void do_query_crypt_q10(ConnectNew &conn,
                               CryptoManager& cm,
                               const string& o_a,
                               vector<q10entry> &results) {
  crypto_manager_stub cm_stub(&cm, UseOldOpe);
  NamedTimer fcnTimer(__func__);

  bool isBin = false;
  string d0 =
    cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(o_a)),
                            TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                            getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
  assert(!isBin);

  string d1 =
    cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(o_a) + (3 << 5) /* + 3 months */),
                            TYPE_INTEGER, fieldname(orders::o_orderdate, "OPE"),
                            getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
  assert(!isBin);

  string l0 =
    cm_stub.crypt<1>(cm.getmkey(), to_s((unsigned int)'R'),
                            TYPE_INTEGER, fieldname(lineitem::l_returnflag, "DET"),
                            getMin(oDET), SECLEVEL::DET, isBin, 12345);
  assert(!isBin);

  ostringstream s;
  s <<
    "select "
    "  c_custkey_DET, "
    "  c_name_DET, "
    "  c_acctbal_DET, "
    "  n_name_DET, "
    "  c_address_DET, "
    "  c_phone_DET, "
    "  c_comment_DET, "
    " group_concat(l_disc_price_DET) "
    "from "
    "lineitem_enc_rowid straight_join orders_enc straight_join customer_enc_rowid straight_join nation_enc "
    "where "
    "  c_custkey_DET = o_custkey_DET "
    "  and l_orderkey_DET = o_orderkey_DET "
    "  and o_orderdate_OPE >= " << d0 << " "
    "  and o_orderdate_OPE < " << d1 << " "
    "  and l_returnflag_DET = " << l0 << " "
    "  and c_nationkey_DET = n_nationkey_DET "
    "group by "
    "  c_custkey_DET, "
    "  c_name_DET, "
    "  c_acctbal_DET, "
    "  c_phone_DET, "
    "  n_name_DET, "
    "  c_address_DET, "
    "  c_comment_DET "
  ;
  cerr << s.str() << endl;

  DBResultNew * dbres;
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

  assert(DoParallel);
  using namespace exec_service;

  vector<_q10_noopt_task_state> states( NumThreads );
  vector<_q10_noopt_task> tasks( NumThreads );
  vector<SqlRowGroup> groups;
  SplitRowsIntoGroups(groups, res.rows, NumThreads);

  for (size_t i = 0; i < NumThreads; i++) {
    tasks[i].cm    = &cm;
    tasks[i].state = &states[i];
    states[i].rows = groups[i];
  }

  Exec<_q10_noopt_task>::DefaultExecutor exec( NumThreads );
  exec.start();
  for (size_t i = 0; i < NumThreads; i++) {
    exec.submit(tasks[i]);
  }
  exec.stop();

  typedef topN_list<q10entry_enc, q10entry_enc_cmp>::const_iterator iter;
  vector<pair<iter, iter> > positions;
  positions.resize( NumThreads );
  for (size_t i = 0; i < NumThreads; i++) {
    positions[i].first  = states[i].bestSoFar.begin();
    positions[i].second = states[i].bestSoFar.end();
  }

  vector<q10entry_enc> merged(20);
  n_way_merge(positions, merged.begin(), q10entry_enc_cmp(), 20);

  for (auto it = merged.begin(); it != merged.end(); ++it) {
    uint64_t c_custkey = decryptRowFromTo<uint64_t>(
            to_s(it->c_custkey_DET),
            12345,
            fieldname(customer::c_custkey, "DET"),
            TYPE_INTEGER,
            SECLEVEL::DETJOIN,
            getMin(oDET),
            cm);

    string c_name = decryptRow<string>(
            it->c_name_DET,
            12345,
            fieldname(customer::c_name, "DET"),
            TYPE_TEXT,
            oDET,
            cm);

    uint64_t c_acctbal_int = decryptRow<uint64_t, 7>(
            to_s(it->c_acctbal_DET),
            12345,
            fieldname(customer::c_acctbal, "DET"),
            TYPE_INTEGER,
            oDET,
            cm);
    double c_acctbal = ((double)c_acctbal_int)/100.0;

    string n_name = decryptRow<string>(
            it->n_name_DET,
            12345,
            fieldname(nation::n_name, "DET"),
            TYPE_TEXT,
            oDET,
            cm);

    string c_address = decryptRow<string>(
            it->c_address_DET,
            12345,
            fieldname(customer::c_address, "DET"),
            TYPE_TEXT,
            oDET,
            cm);

    string c_phone = decryptRow<string>(
            it->c_phone_DET,
            12345,
            fieldname(customer::c_phone, "DET"),
            TYPE_TEXT,
            oDET,
            cm);

    string c_comment = decryptRow<string>(
            it->c_comment_DET,
            12345,
            fieldname(customer::c_comment, "DET"),
            TYPE_TEXT,
            oDET,
            cm);

    results.push_back(
        q10entry(
          c_custkey,
          c_name,
          it->revenue,
          c_acctbal,
          n_name,
          c_address,
          c_phone,
          c_comment));
  }
}


static struct q11entry_sorter {
  inline bool operator()(const q11entry &lhs, const q11entry &rhs) const {
    return lhs.value > rhs.value;
  }
} q11entry_functor;

struct _q11_noopt_task_state {
  _q11_noopt_task_state() : totalSum(0.0) {}
  vector< vector< SqlItem > > rows;
  map<uint64_t, double> aggState;
  double totalSum;
};

struct _q11_noopt_task {
  void operator()() {
      for (auto row : state->rows) {
        // ps_partkey
        uint64_t ps_partkey = decryptRowFromTo<uint64_t, 4>(
                row[0].data,
                12345,
                fieldname(partsupp::ps_partkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
                getMin(oDET),
                *cm);

        // ps_supplycost
        uint64_t ps_supplycost_int = decryptRow<uint64_t, 7>(
                row[1].data,
                12345,
                fieldname(partsupp::ps_supplycost, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);
        double ps_supplycost = ((double)ps_supplycost_int)/100.0;

        // ps_availqty
        uint64_t ps_availqty = decryptRow<uint64_t, 4>(
                row[2].data,
                12345,
                fieldname(partsupp::ps_availqty, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);

        double value = ps_supplycost * ((double)ps_availqty);

        auto it = state->aggState.find(ps_partkey);
        if (it == state->aggState.end()) {
          state->aggState[ps_partkey] = value;
        } else {
          it->second += value;
        }
        state->totalSum += value;
      }
  }
  CryptoManager* cm;
  _q11_noopt_task_state* state;
};

static void do_query_q11_noopt(ConnectNew &conn,
                               CryptoManager &cm,
                               const string &name,
                               double fraction,
                               vector<q11entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    assert(name.size() <= 25);
    NamedTimer fcnTimer(__func__);

    bool isBin = false;
    string encNAME = cm_stub.crypt(cm.getmkey(), name, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(isBin);
    assert(encNAME.size() == name.size());
    encNAME.resize(25);

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
    cerr << s.str() << endl;

    DBResultNew * dbres;
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
    if (DoParallel) {
      NamedTimer t(__func__, "decrypt");

      using namespace exec_service;

      vector<_q11_noopt_task_state> states( NumThreads );
      vector<_q11_noopt_task> tasks( NumThreads );
      vector<SqlRowGroup> groups;
      SplitRowsIntoGroups(groups, res.rows, NumThreads);

      for (size_t i = 0; i < NumThreads; i++) {
        tasks[i].cm    = &cm;
        tasks[i].state = &states[i];
        states[i].rows = groups[i];
      }

      Exec<_q11_noopt_task>::DefaultExecutor exec( NumThreads );
      exec.start();
      for (size_t i = 0; i < NumThreads; i++) {
        exec.submit(tasks[i]);
      }
      exec.stop(); // blocks until completion

      // merge results
      for (size_t i = 0; i < NumThreads; i++) {
        for (map<uint64_t, double>::iterator it = states[i].aggState.begin();
             it != states[i].aggState.end(); ++it) {
          auto it0 = aggState.find(it->first);
          if (it0 == aggState.end()) {
            aggState[it->first] = it->second;
          } else {
            it0->second += it->second;
          }
        }
        totalSum += states[i].totalSum;
      }
    } else {
      NamedTimer t(__func__, "decrypt");
      for (auto row : res.rows) {

        // ps_partkey
        uint64_t ps_partkey = decryptRowFromTo<uint64_t, 4>(
                row[0].data,
                12345,
                fieldname(partsupp::ps_partkey, "DET"),
                TYPE_INTEGER,
                SECLEVEL::DETJOIN,
                getMin(oDET),
                cm);

        // ps_supplycost
        uint64_t ps_supplycost_int = decryptRow<uint64_t, 7>(
                row[1].data,
                12345,
                fieldname(partsupp::ps_supplycost, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);
        double ps_supplycost = ((double)ps_supplycost_int)/100.0;

        // ps_availqty
        uint64_t ps_availqty = decryptRow<uint64_t, 4>(
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

static void do_query_q11(ConnectNew &conn,
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
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

static void do_query_q11_nosubquery(ConnectNew &conn,
                                    const string &name,
                                    double fraction,
                                    vector<q11entry> &results) {
    NamedTimer fcnTimer(__func__);

    // compute threshold
    double threshold = 0.0;
    {
        ostringstream s;
        s << "select sum(ps_supplycost * ps_availqty) * " << fraction << " "
          << "from PARTSUPP_INT, SUPPLIER_INT, NATION_INT "
          << "where ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = '"
          << name << "'";

        DBResultNew * dbres;
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

        assert(res.rows.size() == 1);
        threshold = resultFromStr<double>(res.rows[0][0].data) / 10000.0;
    }

    ostringstream s;
    s << "select SQL_NO_CACHE ps_partkey, sum(ps_supplycost * ps_availqty) as value "
      << "from PARTSUPP_INT, SUPPLIER_INT, NATION_INT "
      << "where ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = '" << name << "' "
      << "group by ps_partkey having sum(ps_supplycost * ps_availqty) > " << roundToLong(threshold * 10000.0)
      //<< "order by value desc"
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

static void do_query_q11_opt_proj(ConnectNew &conn,
                                  CryptoManager &cm,
                                   const string &name,
                                   double fraction,
                                   vector<q11entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
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

    DBResultNew * dbres;
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

static void do_query_q11_opt(ConnectNew &conn,
                             CryptoManager &cm,
                             const string &name,
                             double fraction,
                             vector<q11entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
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
        << "agg(ps_value_AGG, " << pkinfo << ") "
      << "FROM partsupp_enc, supplier_enc, nation_enc "
      << "WHERE "
        << "ps_suppkey_DET = s_suppkey_DET AND "
        << "s_nationkey_DET = n_nationkey_DET AND "
        << "n_name_DET = " << marshallBinary(encNAME)
        ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

    double threshold;
    {
        assert(res.rows.size() == 1);

        // ps_value
        ZZ z;
        cm.decrypt_Paillier(res.rows[0][0].data, z);
        typedef PallierSlotManager<uint64_t, 2> PSM;
        uint64_t ps_value_int = PSM::extract_from_slot<uint64_t>(z, 0);
        double ps_value = ((double)ps_value_int)/100.0;
        threshold = ps_value * fraction;
    }

    //cerr << "threshold: " << threshold << endl;

    // encrypt threshold for ps_value_OPE
    uint64_t threshold_int = (uint64_t) roundToLong(threshold * 100.0);

    isBin = false;
    string encVALUE = cm_stub.crypt<7>(cm.getmkey(), to_s(threshold_int), TYPE_INTEGER,
                               fieldname(partsupp::ps_value, "OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(isBin);
    assert(encVALUE.size() <= 14);
    encVALUE.resize(16); // pad
    encVALUE = str_reverse(encVALUE); // reverse

    // do all the one element agg group cases first.
    // this is a very hacky query where we try to do several things at the
    // same time, while optimizing for minimum data sent back
    ostringstream s1;
    s1
      << "SELECT SQL_NO_CACHE ps_partkey_DET, ps_value_DET, cnt, suppkeys FROM ("
        << "SELECT "
          << "ps_partkey_DET, ps_value_DET, "
          << "COUNT(*) AS cnt, COUNT(*) > 1 OR ps_value_OPE > "
            << marshallBinary(encVALUE) << " AS filter, "
          << "CASE COUNT(*) WHEN 1 THEN NULL ELSE GROUP_CONCAT(ps_suppkey_DET) END AS suppkeys "
        << "FROM partsupp_enc, supplier_enc, nation_enc "
        << "WHERE "
          << "ps_suppkey_DET = s_suppkey_DET AND "
          << "s_nationkey_DET = n_nationkey_DET AND "
          << "n_name_DET = " << marshallBinary(encNAME) << " "
        << "GROUP BY ps_partkey_DET"
      << ") AS __ANON__ WHERE filter = 1";
    cerr << s1.str() << endl;

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
    results.reserve(res.rows.size());
    {
      NamedTimer t(__func__, "decrypt_groups");
      for (auto row : res.rows) {
        // first, check count(*). if count(*) == 1, go ahead
        // and do the decryption. if count(*) > 1, then save ps_partkey_DET
        // away for later (but do NOT decrypt it)

        if (valFromStr(row[2].data) == 1) {
          // ps_partkey
          uint64_t ps_partkey = decryptRowFromTo<uint64_t, 4>(
                  row[0].data,
                  12345,
                  fieldname(partsupp::ps_partkey, "DET"),
                  TYPE_INTEGER,
                  SECLEVEL::DETJOIN,
                  getMin(oDET),
                  cm);

          // ps_value
          uint64_t ps_value_int = decryptRow<uint64_t, 7>(
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

    if (!mapped.empty()) {

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
            uint64_t ps_partkey = decryptRowFromTo<uint64_t, 4>(
                    row[0].data,
                    12345,
                    fieldname(partsupp::ps_partkey, "DET"),
                    TYPE_INTEGER,
                    SECLEVEL::DETJOIN,
                    getMin(oDET),
                    cm);

            // ps_value
            uint64_t ps_value_int = decryptRow<uint64_t, 7>(
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
              it->second += ps_value;
            }
          }
        }

        results.reserve(results.size() + aggState.size());
        for (map<uint64_t, double>::iterator it = aggState.begin();
             it != aggState.end();
             ++it) {
          if (it->second > threshold) {
            results.push_back(q11entry(it->first, it->second));
          }
        }
    }

    {
      NamedTimer t(__func__, "agg");
      // sort
      sort(results.begin(), results.end(), q11entry_functor);
    }
}

//static void do_query_q11_opt(ConnectNew &conn,
//                             CryptoManager &cm,
//                             const string &name,
//                             double fraction,
//                             vector<q11entry> &results) {
//    crypto_manager_stub cm_stub(&cm, UseOldOpe);
//    assert(name.size() <= 25);
//    NamedTimer fcnTimer(__func__);
//
//    bool isBin = false;
//    string encNAME = cm_stub.crypt(cm.getmkey(), name, TYPE_TEXT,
//                              fieldname(nation::n_name, "DET"),
//                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
//    assert(isBin);
//    assert(encNAME.size() == name.size());
//    encNAME.resize(25);
//
//    string pkinfo = marshallBinary(cm.getPKInfo());
//    ostringstream s;
//    s << "SELECT SQL_NO_CACHE "
//        << "ps_partkey_DET, "
//        << "agg(ps_value_AGG, " << pkinfo << ") "
//      << "FROM partsupp_enc, supplier_enc, nation_enc "
//      << "WHERE "
//        << "ps_suppkey_DET = s_suppkey_DET AND "
//        << "s_nationkey_DET = n_nationkey_DET AND "
//        << "n_name_DET = " << marshallBinary(encNAME) << " "
//      << "GROUP BY ps_partkey_DET";
//    cerr << s.str() << endl;
//
//    DBResultNew * dbres;
//    {
//      NamedTimer t(__func__, "execute");
//      conn.execute(s.str(), dbres);
//    }
//    ResType res;
//    {
//      NamedTimer t(__func__, "unpack");
//      res = dbres->unpack();
//      assert(res.ok);
//    }
//
//    double totalSum = 0.0;
//    {
//      NamedTimer t(__func__, "decrypt");
//      results.reserve(res.rows.size());
//      for (auto row : res.rows) {
//
//        // ps_partkey
//        uint64_t ps_partkey = decryptRowFromTo<uint64_t>(
//                row[0].data,
//                12345,
//                fieldname(partsupp::ps_partkey, "DET"),
//                TYPE_INTEGER,
//                SECLEVEL::DETJOIN,
//                getMin(oDET),
//                cm);
//
//        // ps_value
//        uint64_t ps_value_int = decryptRow<uint64_t>(
//                row[1].data,
//                12345,
//                fieldname(partsupp::ps_value, "AGG"),
//                TYPE_INTEGER,
//                oAGG,
//                cm);
//        double ps_value = ((double)ps_value_int)/100.0;
//
//        totalSum += ps_value;
//        results.push_back(q11entry(ps_partkey, ps_value));
//      }
//    }
//
//    {
//      NamedTimer t(__func__, "agg");
//      double threshold = totalSum * fraction;
//      // remove elements less than threshold, stl style
//      results.erase(
//          remove_if(results.begin(), results.end(), q11entry_remover(threshold)),
//          results.end());
//
//      // sort
//      sort(results.begin(), results.end(), q11entry_functor);
//    }
//}

static void do_query_q12(ConnectNew &conn,
                         const string& l_a,
                         const string& l_b,
                         const string& l_c,
                         vector<q12entry> &results) {
  NamedTimer fcnTimer(__func__);
  ostringstream s;
  s <<
    "select "
    "  l_shipmode, "
    "  sum(case "
    "    when o_orderpriority = '1-URGENT' "
    "      or o_orderpriority = '2-HIGH' "
    "      then 1 "
    "    else 0 "
    "  end) as high_line_count, "
    "  sum(case "
    "    when o_orderpriority <> '1-URGENT' "
    "      and o_orderpriority <> '2-HIGH' "
    "      then 1 "
    "    else 0 "
    "  end) as low_line_count "
    "from "
    "  ORDERS_INT, "
    "  LINEITEM_INT "
    "where "
    "  o_orderkey = l_orderkey "
    "  and l_shipmode in ('" << l_a << "', '" << l_b << "') "
    "  and l_commitdate < l_receiptdate "
    "  and l_shipdate < l_commitdate "
    "  and l_receiptdate >= date '" << l_c << "' "
    "  and l_receiptdate < date '" << l_c << "' + interval '1' year "
    "group by "
    "  l_shipmode "
    "order by "
    "  l_shipmode"
    ;
  cerr << s.str() << endl;

  DBResultNew * dbres;
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
        q12entry(
          row[0].data,
          resultFromStr<uint64_t>(row[1].data),
          resultFromStr<uint64_t>(row[2].data)));
  }
}

static string enc_fixed_len_str(
    const string& pt,
    const string& fieldname,
    CryptoManager& cm,
    size_t fixed_len) {
  crypto_manager_stub cm_stub(&cm, UseOldOpe);
  assert(pt.size() <= fixed_len);
  bool isBin = false;
  string ct = cm_stub.crypt(cm.getmkey(), pt, TYPE_TEXT,
                            fieldname, getMin(oDET), SECLEVEL::DET,
                            isBin, 12345);
  assert(isBin);
  assert(ct.size() == pt.size());
  ct.resize(fixed_len);
  return ct;
}

static void do_query_crypt_q12(ConnectNew &conn,
                               CryptoManager &cm,
                               const string& l_a,
                               const string& l_b,
                               const string& l_c,
                               vector<q12entry> &results) {
  crypto_manager_stub cm_stub(&cm, UseOldOpe);
  NamedTimer fcnTimer(__func__);

  string ourgent_ct =
    enc_fixed_len_str(
        "1-URGENT", fieldname(orders::o_orderpriority, "DET"), cm, 15);

  string ohigh_ct =
    enc_fixed_len_str(
        "2-HIGH", fieldname(orders::o_orderpriority, "DET"), cm, 15);

  string l_a_enc =
    enc_fixed_len_str(
        l_a, fieldname(lineitem::l_shipmode, "DET"), cm, 10);

  string l_b_enc =
    enc_fixed_len_str(
        l_b, fieldname(lineitem::l_shipmode, "DET"), cm, 10);

  bool isBin = false;
  string d0 = cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(l_c)),
                                 TYPE_INTEGER, "ope_join",
                                 getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
  assert(!isBin);

  string d1 = cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(l_c) + (1 << 9) /* 1 year */),
                                 TYPE_INTEGER, "ope_join",
                                 getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
  assert(!isBin);

  ostringstream s;
  s <<
    "select "
    "  l_shipmode_DET, "
    "  sum(case "
    "    when o_orderpriority_DET = " << marshallBinary(ourgent_ct) << " "
    "      or o_orderpriority_DET = " << marshallBinary(ohigh_ct)   << " "
    "      then 1 "
    "    else 0 "
    "  end) as high_line_count, "
    "  sum(case "
    "    when o_orderpriority_DET  <> " << marshallBinary(ourgent_ct) << " "
    "      and o_orderpriority_DET <> " << marshallBinary(ohigh_ct)   << " "
    "      then 1 "
    "    else 0 "
    "  end) as low_line_count "
    "from "
    "  orders_enc, "
    "  lineitem_enc_rowid "
    "where "
    "  o_orderkey_DET = l_orderkey_DET "
    "  and l_shipmode_DET in (" << marshallBinary(l_a_enc)
    << ", " << marshallBinary(l_b_enc) << ") "
    "  and l_commitdate_OPE < l_receiptdate_OPE "
    "  and l_shipdate_OPE < l_commitdate_OPE "
    "  and l_receiptdate_OPE >= " << d0 << " "
    "  and l_receiptdate_OPE < "  << d1 << " "
    "group by "
    "  l_shipmode_DET "
    ;
  cerr << s.str() << endl;

  DBResultNew * dbres;
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
    string l_shipmode = decryptRow<string>(
        row[0].data,
        12345,
        fieldname(lineitem::l_shipmode, "DET"),
        TYPE_TEXT,
        oDET,
        cm);
    results.push_back(
        q12entry(
          l_shipmode,
          resultFromStr<uint64_t>(row[1].data),
          resultFromStr<uint64_t>(row[2].data)));
  }
}

static void do_query_q14_opt(ConnectNew &conn,
                             CryptoManager &cm,
                             uint32_t year,
                             vector<q14entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin;
    string encDateLower = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(7, 1, year)),
                                   TYPE_INTEGER, "ope_join",
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDateUpper = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(8, 1, year)),
                                   TYPE_INTEGER, "ope_join",
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    Binary key(cm.getKey(cm.getmkey(), fieldname(part::p_type, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary("promo"));

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s << "SELECT SQL_NO_CACHE "
        << "agg(CASE WHEN "
          << "searchSWP("
            << marshallBinary(string((char *)t.ciph.content, t.ciph.len))
            << ", "
            << marshallBinary(string((char *)t.wordKey.content, t.wordKey.len))
            << ", p_type_SWP) = 1 "
          << "THEN l_bitpacked_AGG ELSE NULL END, " << pkinfo << "), "
        << "agg(l_bitpacked_AGG, " << pkinfo << ") "
      << "FROM lineitem_enc, part_enc "
      << "WHERE "
        << "l_partkey_DET = p_partkey_DET AND "
        << "l_shipdate_OPE >= " << encDateLower << " AND "
        << "l_shipdate_OPE < " << encDateUpper;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

static void do_query_q14_opt2(ConnectNew &conn,
                              CryptoManager &cm,
                              uint32_t year,
                              vector<q14entry> &results,
                              const string& db) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    {
      NamedTimer t(__func__, "execute-prefetch");

      conn.execute("SET GLOBAL innodb_old_blocks_time = 0"); // allow to flood buffer pool

      // NOTE: this is our attempt to mimic a hash join
      // and avoid having to do lots of random IO
      conn.execute("CREATE TEMPORARY TABLE part_enc_tmp ("
                   "p_partkey_DET integer unsigned, "
                   "p_type_SWP varbinary(255), "
                   "PRIMARY KEY (p_partkey_DET)) ENGINE=MEMORY");

      conn.execute("INSERT INTO part_enc_tmp SELECT p_partkey_DET, p_type_SWP FROM part_enc");

      conn.execute("SET GLOBAL innodb_old_blocks_time = 1000");
    }

    bool isBin;
    string encDateLower = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(7, 1, year)),
                                   TYPE_INTEGER, "ope_join",
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDateUpper = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(8, 1, year)),
                                   TYPE_INTEGER, "ope_join",
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    Binary key(cm.getKey(cm.getmkey(), fieldname(part::p_type, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary("promo"));

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;

    string filename =
      "/space/stephentu/data/" + db + "/lineitem_enc/row_pack/disc_price";

    s << "SELECT SQL_NO_CACHE agg_hash_agg_row_col_pack(0, row_id, "
        << pkinfo << ", " <<
        "\"" << filename << "\", " << NumThreads << ", 256, 12, "
        << "searchSWP("
          << marshallBinary(string((char *)t.ciph.content, t.ciph.len))
          << ", "
          << marshallBinary(string((char *)t.wordKey.content, t.wordKey.len))
          << ", p_type_SWP) = 1, 1) "
      << "FROM lineitem_enc_rowid, part_enc_tmp "
      << "WHERE "
        << "l_partkey_DET = p_partkey_DET AND "
        << "l_shipdate_OPE >= " << encDateLower << " AND "
        << "l_shipdate_OPE < " << encDateUpper;

    cerr << s.str() << endl;

    DBResultNew * dbres;
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

    static const size_t BitsPerAggField = 83;
    ZZ mask = to_ZZ(1); mask <<= BitsPerAggField; mask -= 1;
    assert(NumBits(mask) == (int)BitsPerAggField);
    {
      assert(res.rows.size() == 1);
      string data = res.rows[0][0].data;

      double top = 0.0;
      double bot = 0.0;

      const uint8_t *p   = (const uint8_t *) data.data();
      const uint8_t *end = (const uint8_t *) data.data() + data.size();
      while (p < end) {

        // skip over count
        p += sizeof(uint64_t);

#define TAKE_FROM_SLOT(z, slot) \
        (to_long(((z) >> (BitsPerAggField * (slot))) & mask))

        for (size_t mode = 0; mode < 2; mode++) {
          const uint32_t *u32p = (const uint32_t *) p;
          uint32_t n_aggs = *u32p;
          p += sizeof(uint32_t);

          for (size_t group_i = 0; group_i < n_aggs; group_i++) {
            // interest mask
            const uint32_t *u32p = (const uint32_t *) p;
            uint32_t interest_mask = *u32p;
            p += sizeof(uint32_t);

            string ct = string((const char *) p, (size_t) 256);
            ZZ m;
            cm.decrypt_Paillier(ct, m);
            p += 256;

            for (size_t i = 0; i < 12; i++) {
              if (!(interest_mask & (0x1 << i))) continue;
              long l = TAKE_FROM_SLOT(m, i);
              if (mode == 0) {
                top += ((double)l)/100.0;
              } else {
                bot += ((double)l)/100.0;
              }
            }
          }
        }
      }

      double result = 100.0 * (top / bot);
      results.push_back(result);
    }
}

struct _q14_noopt_task_state {
  _q14_noopt_task_state() : running_numer(0.0), running_denom(0.0) {}
  vector< vector< SqlItem > > rows;
  double running_numer;
  double running_denom;
};

struct _q14_noopt_task {
  void operator()() {
    for (auto row : state->rows) {
      uint64_t l_disc_price_int = decryptRow<uint64_t, 7>(
          row[1].data,
          12345,
          fieldname(0, "DET"),
          TYPE_INTEGER,
          oDET,
          *cm);
      double l_disc_price = ((double)l_disc_price_int)/100.0;

      double value = l_disc_price;
      state->running_denom += value;

      // decrypt p_type, to check if matches
      string p_type = decryptRow<string>(
          row[0].data,
          12345,
          fieldname(part::p_type, "DET"),
          TYPE_TEXT,
          oDET,
          *cm);
      if (strncmp(p_type.c_str(), "PROMO", strlen("PROMO")) == 0) {
        state->running_numer += value;
      }
    }
  }
  CryptoManager* cm;
  _q14_noopt_task_state* state;
};

static void do_query_q14_noopt(ConnectNew &conn,
                               CryptoManager &cm,
                               uint32_t year,
                               vector<q14entry> &results,
                               bool use_opt_table = false) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    {
      NamedTimer t(__func__, "execute-prefetch");

      conn.execute("SET GLOBAL innodb_old_blocks_time = 0"); // allow to flood buffer pool

      // NOTE: this is our attempt to mimic a hash join
      // and avoid having to do lots of random IO
      conn.execute("CREATE TEMPORARY TABLE part_enc_tmp ("
                   "p_partkey_DET integer unsigned, "
                   "p_type_DET varbinary(25), "
                   "PRIMARY KEY (p_partkey_DET)) ENGINE=MEMORY");

      conn.execute("INSERT INTO part_enc_tmp SELECT p_partkey_DET, p_type_DET FROM part_enc");

      conn.execute("SET GLOBAL innodb_old_blocks_time = 1000");
    }

    bool isBin;
    string encDateLower = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(7, 1, year)),
            TYPE_INTEGER, "ope_join",
            getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDateUpper = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(8, 1, year)),
            TYPE_INTEGER, "ope_join",
            getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    ostringstream s;
    s << "SELECT SQL_NO_CACHE p_type_DET, l_disc_price_DET "
        << "FROM " << string(use_opt_table ? "lineitem_enc" : "lineitem_enc_rowid")
          << ", part_enc_tmp "
        << "WHERE l_partkey_DET = p_partkey_DET AND "
        << "l_shipdate_OPE >= " << encDateLower << " AND "
        << "l_shipdate_OPE < " << encDateUpper;
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

    cerr << "got " << res.rows.size() << " rows to decrypt" << endl;

    if (DoParallel) {
      NamedTimer t(__func__, "decrypt");
      using namespace exec_service;

      vector<_q14_noopt_task_state> states( NumThreads );
      vector<_q14_noopt_task> tasks( NumThreads );
      vector<SqlRowGroup> groups;
      SplitRowsIntoGroups(groups, res.rows, NumThreads);

      for (size_t i = 0; i < NumThreads; i++) {
        tasks[i].cm    = &cm;
        tasks[i].state = &states[i];
        states[i].rows = groups[i];
      }

      Exec<_q14_noopt_task>::DefaultExecutor exec( NumThreads );
      exec.start();
      for (size_t i = 0; i < NumThreads; i++) {
        exec.submit(tasks[i]);
      }
      exec.stop(); // blocks until completion

      // merge results
      double running_numer = 0.0;
      double running_denom = 0.0;
      for (size_t i = 0; i < NumThreads; i++) {
        running_numer += states[i].running_numer;
        running_denom += states[i].running_denom;
      }
      results.push_back(100.0 * running_numer / running_denom);
    } else {
      NamedTimer t(__func__, "decrypt");
      double running_numer = 0.0;
      double running_denom = 0.0;
      for (auto row : res.rows) {
          // l_extendedprice * (1 - l_discount)
          uint64_t l_extendedprice_int = decryptRow<uint64_t, 7>(
                  row[1].data,
                  12345,
                  fieldname(lineitem::l_extendedprice, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_extendedprice = ((double)l_extendedprice_int)/100.0;

          uint64_t l_discount_int = decryptRow<uint64_t, 7>(
                  row[2].data,
                  12345,
                  fieldname(lineitem::l_discount, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  cm);
          double l_discount = ((double)l_discount_int)/100.0;

          double value = l_extendedprice * (1.0 - l_discount);
          running_denom += value;

          // decrypt p_type, to check if matches
          string p_type = decryptRow<string>(
                  row[0].data,
                  12345,
                  fieldname(part::p_type, "DET"),
                  TYPE_TEXT,
                  oDET,
                  cm);
          if (strncmp(p_type.c_str(), "PROMO", strlen("PROMO")) == 0) {
              running_numer += value;
          }
      }
      results.push_back(100.0 * running_numer / running_denom);
    }
}

static void do_query_q14(ConnectNew &conn,
                         uint32_t year,
                         vector<q14entry> &results) {
    NamedTimer fcnTimer(__func__);

    {
      NamedTimer t(__func__, "execute-prefetch");

      conn.execute("SET GLOBAL innodb_old_blocks_time = 0"); // allow to flood buffer pool

      // NOTE: this is our attempt to mimic a hash join
      // and avoid having to do lots of random IO
      conn.execute("CREATE TEMPORARY TABLE part_tmp ("
                   "p_partkey integer, "
                   "p_type varchar(25), "
                   "PRIMARY KEY (p_partkey)) ENGINE=MEMORY");

      conn.execute("INSERT INTO part_tmp SELECT p_partkey, p_type FROM PART_INT");

      conn.execute("SET GLOBAL innodb_old_blocks_time = 1000");
    }

    ostringstream s;
    s << "select SQL_NO_CACHE 100.00 * sum(case when p_type like 'PROMO%' then l_extendedprice * (100 - l_discount) else 0 end) / sum(l_extendedprice * (100 - l_discount)) as promo_revenue from LINEITEM_INT, part_tmp where l_partkey = p_partkey and l_shipdate >= date '" << year << "-07-01' and l_shipdate < date '" << year << "-07-01' + interval '1' month";
    cerr << s.str() << endl;

    DBResultNew * dbres;
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

static void do_query_q15(ConnectNew &conn,
                         const string& l_a,
                         vector<q15entry> &results) {
  NamedTimer fcnTimer(__func__);

  // query 1
  ostringstream s;
  s <<
    "select "
    "  l_suppkey, "
    "  sum(l_extendedprice * (100 - l_discount)) as revenue "
    "from "
    "  LINEITEM_INT "
    "where "
    "  l_shipdate >= date '" << l_a << "' "
    "  and l_shipdate < date '"<< l_a << "' + interval '3' month "
    "group by "
    "  l_suppkey "
    "order by "
    " revenue desc "
    "limit 1";
  cerr << s.str() << endl;

  DBResultNew * dbres;
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

  assert(res.rows.size() == 1);
  double total_revenue = resultFromStr<double>(res.rows[0][1].data)/10000.0;

  ostringstream s1;
  s1 <<
    "select "
    "  s_suppkey, "
    "  s_name, "
    "  s_address, "
    "  s_phone "
    "from "
    "  SUPPLIER_INT "
    "where "
    "  s_suppkey = " << res.rows[0][0].data << " "
    "order by "
    "  s_suppkey; "
    ;
  cerr << s1.str() << endl;

  {
    NamedTimer t(__func__, "execute-1");
    conn.execute(s1.str(), dbres);
  }
  {
    NamedTimer t(__func__, "unpack-1");
    res = dbres->unpack();
    assert(res.ok);
  }

  for (auto row : res.rows) {
    results.push_back(
      q15entry(
        resultFromStr<uint64_t>(row[0].data),
        row[1].data,
        row[2].data,
        row[3].data,
        total_revenue));
  }
}

struct q15entry_enc {
  q15entry_enc() {}
  q15entry_enc(
      uint64_t l_suppkey_DET,
      double revenue) :
    l_suppkey_DET(l_suppkey_DET),
    revenue(revenue) {}
  uint64_t l_suppkey_DET;
  double revenue;
};

struct q15entry_enc_cmp {
  inline bool operator()(const q15entry_enc &lhs, const q15entry_enc &rhs) const {
    return lhs.revenue > rhs.revenue;
  }
};

struct _q15_noopt_task_state {
  _q15_noopt_task_state() : bestSoFar(1) {}
  vector< vector< SqlItem > > rows;
  topN_list<q15entry_enc, q15entry_enc_cmp> bestSoFar;
};

struct _q15_noopt_task {
  void operator()() {
      for (auto row : state->rows) {
          vector<string> ciphers;
          tokenize(row[1].data, ",", ciphers);
          double sum = 0.0;
          for (size_t i = 0; i < ciphers.size(); i++) {
            uint64_t l_disc_price_int = decryptRow<uint64_t, 7>(
                    ciphers[i],
                    12345,
                    fieldname(0, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    *cm);
            double l_disc_price = ((double)l_disc_price_int)/100.0;
            sum += l_disc_price;
          }
          q15entry_enc cur_group(
                  resultFromStr<uint64_t>(row[0].data),
                  sum);
          state->bestSoFar.add_elem(cur_group);
      }
  }
  CryptoManager* cm;
  _q15_noopt_task_state* state;
};

static void do_query_crypt_q15(ConnectNew &conn,
                               CryptoManager& cm,
                               const string& l_a,
                               vector<q15entry> &results,
                               const string& db) {
  crypto_manager_stub cm_stub(&cm, UseOldOpe);
  NamedTimer fcnTimer(__func__);

  bool isBin = false;
  string d0 = cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(l_a)),
                            TYPE_INTEGER, "ope_join",
                            getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
  assert(!isBin);

  date_t endDate(l_a);
  endDate.add_months(3);
  string d1 = cm_stub.crypt<3>(cm.getmkey(), to_s(endDate.encode()),
                            TYPE_INTEGER, "ope_join",
                            getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
  assert(!isBin);

  ostringstream s;
  s <<
    "select "
    "  l_suppkey_DET, "
    "  group_concat(l_disc_price_DET) "
    "from "
    "  lineitem_enc_rowid "
    "where "
    "  l_shipdate_OPE >= " << d0 << " "
    "  and l_shipdate_OPE < "<< d1 << " "
    "group by "
    "  l_suppkey_DET"
    ;
  cerr << s.str() << endl;

  DBResultNew * dbres;
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

  assert(DoParallel);

  using namespace exec_service;

  vector<_q15_noopt_task_state> states( NumThreads );
  vector<_q15_noopt_task> tasks( NumThreads );
  vector<SqlRowGroup> groups;
  SplitRowsIntoGroups(groups, res.rows, NumThreads);

  for (size_t i = 0; i < NumThreads; i++) {
    tasks[i].cm    = &cm;
    tasks[i].state = &states[i];
    states[i].rows = groups[i];
  }

  Exec<_q15_noopt_task>::DefaultExecutor exec( NumThreads );
  exec.start();
  for (size_t i = 0; i < NumThreads; i++) {
    exec.submit(tasks[i]);
  }
  exec.stop();

  typedef topN_list<q15entry_enc, q15entry_enc_cmp>::const_iterator iter;
  vector<pair<iter, iter> > positions;
  positions.resize( NumThreads );
  for (size_t i = 0; i < NumThreads; i++) {
    positions[i].first  = states[i].bestSoFar.begin();
    positions[i].second = states[i].bestSoFar.end();
  }

  vector<q15entry_enc> merged(1);
  n_way_merge(positions, merged.begin(), q15entry_enc_cmp(), 1);

  // query2

  ostringstream s1;
  s1 <<
    "select "
    "  s_suppkey_DET, "
    "  s_name_DET, "
    "  s_address_DET, "
    "  s_phone_DET "
    "from "
    "  supplier_enc "
    "where "
    "  s_suppkey_DET = " << merged.front().l_suppkey_DET
    ;

  {
    NamedTimer t(__func__, "execute-1");
    conn.execute(s1.str(), dbres);
  }
  {
    NamedTimer t(__func__, "unpack-1");
    res = dbres->unpack();
    assert(res.ok);
  }

  for (auto row : res.rows) {

    uint64_t s_suppkey = decryptRowFromTo<uint64_t, 4>(
            row[0].data,
            12345,
            fieldname(supplier::s_suppkey, "DET"),
            TYPE_INTEGER,
            SECLEVEL::DETJOIN,
            getMin(oDET),
            cm);

    string s_name = decryptRow<string>(
            row[1].data,
            12345,
            fieldname(supplier::s_name, "DET"),
            TYPE_TEXT,
            oDET,
            cm);

    string s_address = decryptRow<string>(
            row[2].data,
            12345,
            fieldname(supplier::s_address, "DET"),
            TYPE_TEXT,
            oDET,
            cm);

    string s_phone = decryptRow<string>(
            row[3].data,
            12345,
            fieldname(supplier::s_phone, "DET"),
            TYPE_TEXT,
            oDET,
            cm);

    results.push_back(
      q15entry(
        s_suppkey, s_name, s_address, s_phone,
        merged.front().revenue));
  }
}

static void do_query_q17(ConnectNew &conn,
                         const string& p_a,
                         const string& p_b,
                         vector<q17entry> &results) {
  NamedTimer fcnTimer(__func__);

  DBResultNew * dbres;
  ResType res;

  conn.execute("CREATE TEMPORARY TABLE inner_tmp ("
               "p integer unsigned, "
               "q bigint unsigned, "
               "PRIMARY KEY (p)) ENGINE=MEMORY");

  {
    ostringstream s;
    s <<
      "INSERT INTO inner_tmp "
      "select l_partkey, 0.2 * avg(l_quantity) from LINEITEM_INT, PART_INT "
      "where p_partkey = l_partkey and "
      "p_brand = '" << p_a << "' and "
      "p_container = '" << p_b << "' "
      "group by l_partkey;"
      ;
    cerr << s.str() << endl;
    {
      NamedTimer t(__func__, "execute-1");
      conn.execute(s.str(), dbres);
    }
  }

  ostringstream s;
  s <<
    "select "
    "  sum(l_extendedprice) / 700.0 as avg_yearly "
    "from "
    "  LINEITEM_INT, "
    "  PART_INT, "
    "  inner_tmp "
    "where "
    "  p_partkey = l_partkey "
    "  and p_partkey = p "
    "  and p_brand = '" << p_a << "' "
    "  and p_container = '"<< p_b << "' "
    "  and l_quantity < q; "
    ;
  cerr << s.str() << endl;

  {
    NamedTimer t(__func__, "execute-2");
    conn.execute(s.str(), dbres);
  }
  {
    NamedTimer t(__func__, "unpack-2");
    res = dbres->unpack();
    assert(res.ok);
  }

  for (auto row : res.rows) {
    results.push_back(resultFromStr<double>(row[0].data));
  }
}

struct _q17_noopt_task_state {
  _q17_noopt_task_state() : local_sum(0.0) {}
  vector< vector< SqlItem > > rows;
  double local_sum;
};

struct _q17_noopt_task {
  void operator()() {
    for (auto row : state->rows) {
      vector<string> ciphers;
      tokenize(row[1].data, ",", ciphers);

      vector<double> quantities;
      quantities.reserve(ciphers.size());

      double sum = 0.0;
      for (size_t i = 0; i < ciphers.size(); i++) {
        uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
            ciphers[i],
            12345,
            fieldname(lineitem::l_quantity, "DET"),
            TYPE_INTEGER,
            oDET,
            *cm);
        double l_quantity = ((double)l_quantity_int)/100.0;
        sum += l_quantity;
        quantities.push_back(l_quantity);
      }

      double threshold = 0.2 * sum / double(ciphers.size());

      vector<string> ext_ciphers;
      tokenize(row[2].data, ",", ext_ciphers);
      assert(quantities.size() == ext_ciphers.size());

      for (size_t i = 0; i < quantities.size(); i++) {
        if (quantities[i] < threshold) {
          // decode l_extendedprice and add to local_sum
          uint64_t l_extendedprice_int = decryptRow<uint64_t, 7>(
              ext_ciphers[i],
              12345,
              fieldname(lineitem::l_extendedprice, "DET"),
              TYPE_INTEGER,
              oDET,
              *cm);
          double l_extendedprice = ((double)l_extendedprice_int)/100.0;
          state->local_sum += l_extendedprice;
        }
      }

    }
  }
  CryptoManager* cm;
  _q17_noopt_task_state* state;
};

static void do_query_crypt_q17(ConnectNew &conn,
                               CryptoManager& cm,
                               const string& p_a,
                               const string& p_b,
                               vector<q17entry> &results) {
  crypto_manager_stub cm_stub(&cm, UseOldOpe);
  NamedTimer fcnTimer(__func__);

  DBResultNew * dbres;
  ResType res;

  bool isBin = false;
  string p_a_enc = cm_stub.crypt(cm.getmkey(), p_a, TYPE_TEXT,
                            fieldname(part::p_brand, "DET"),
                            getMin(oDET), SECLEVEL::DET, isBin, 12345);
  assert(isBin);

  isBin = false;
  string p_b_enc = cm_stub.crypt(cm.getmkey(), p_b, TYPE_TEXT,
                            fieldname(part::p_container, "DET"),
                            getMin(oDET), SECLEVEL::DET, isBin, 12345);
  assert(isBin);

  ostringstream s;
  s <<
    "select "
    "l_partkey_DET, group_concat(l_quantity_DET), group_concat(l_extendedprice_DET) "
    "from lineitem_enc_rowid, part_enc "
    "where p_partkey_DET = l_partkey_DET and "
    "p_brand_DET = "     << marshallBinary(p_a_enc) << " and "
    "p_container_DET = " << marshallBinary(p_b_enc) << " "
    "group by l_partkey_DET"
    ;
  cerr << s.str() << endl;
  {
    NamedTimer t(__func__, "execute-1");
    conn.execute(s.str(), dbres);
  }
  {
    NamedTimer t(__func__, "unpack-1");
    res = dbres->unpack();
    assert(res.ok);
  }

  assert(DoParallel);

  using namespace exec_service;

  vector<_q17_noopt_task_state> states( NumThreads );
  vector<_q17_noopt_task> tasks( NumThreads );
  vector<SqlRowGroup> groups;
  SplitRowsIntoGroups(groups, res.rows, NumThreads);

  for (size_t i = 0; i < NumThreads; i++) {
    tasks[i].cm        = &cm;
    tasks[i].state     = &states[i];
    states[i].rows     = groups[i];
  }

  Exec<_q17_noopt_task>::DefaultExecutor exec( NumThreads );
  exec.start();
  for (size_t i = 0; i < NumThreads; i++) {
    exec.submit(tasks[i]);
  }
  exec.stop();

  double sum = 0.0;
  for (size_t i = 0; i < NumThreads; i++) {
    sum += states[i].local_sum;
  }

  results.push_back( sum / 7.0 );
}

static void do_query_q18(ConnectNew &conn,
                         uint64_t threshold,
                         vector<q18entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s0;
    s0 <<
        "select "
        "    l_orderkey "
        "from "
        "    LINEITEM_INT "
        "group by "
        "    l_orderkey having "
        "        sum(l_quantity) > " << (threshold * 100);
    cerr << s0.str() << endl;

    vector<string> l_orderkeys;
    {
        DBResultNew * dbres;
        {
            NamedTimer t(__func__, "execute");
            conn.execute(s0.str(), dbres);
        }
        ResType res;
        {
            NamedTimer t(__func__, "unpack");
            res = dbres->unpack();
            assert(res.ok);
        }
        l_orderkeys.reserve(res.rows.size());
        for (auto row : res.rows) {
            l_orderkeys.push_back(row[0].data);
        }
    }
    if (l_orderkeys.empty()) return;

    ostringstream s;
    s <<
        "select "
        "    c_name, c_custkey, o_orderkey, "
        "    o_orderdate, o_totalprice, sum(l_quantity) "
        "from "
        "    CUSTOMER_INT, ORDERS_INT, LINEITEM_INT "
        "where "
        "    o_orderkey in ( "
        << join(l_orderkeys, ",") <<
        "    ) "
        "    and c_custkey = o_custkey "
        "    and o_orderkey = l_orderkey "
        "group by "
        "    c_name, "
        "    c_custkey, "
        "    o_orderkey, "
        "    o_orderdate, "
        "    o_totalprice "
        "order by "
        "    o_totalprice desc, "
        "    o_orderdate "
        "limit 100";

    DBResultNew * dbres;
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

    results.reserve(res.rows.size());
    for (auto row : res.rows) {
        results.push_back(
            q18entry(
                row[0].data,
                resultFromStr<uint64_t>(row[1].data),
                resultFromStr<uint64_t>(row[2].data),
                0,
                resultFromStr<double>(row[4].data) / 100.0,
                resultFromStr<double>(row[5].data) / 100.0));
    }
}

struct _q18_noopt_task_state {
  _q18_noopt_task_state()  {}
  vector< vector< SqlItem > > rows;
  vector<string> l_orderkeys;
};

struct _q18_noopt_task {
  void operator()() {
    static const double L_QUANTITY_MAX = 50.0; // comes from statistics
    for (auto row : state->rows) {
      vector<string> ciphers;
      tokenize(row[1].data, ",", ciphers);
      assert(!ciphers.empty());

      double sum = 0.0;
      for (vector<string>::iterator it = ciphers.begin();
           it != ciphers.end(); ++it) {
          uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
                  *it,
                  12345,
                  fieldname(lineitem::l_quantity, "DET"),
                  TYPE_INTEGER,
                  oDET,
                  *cm);
          double l_quantity = ((double)l_quantity_int)/100.0;
          sum += l_quantity;

          // short circuit evaluations:

          // if we are already past the threshold, then quit
          if (sum > (double) threshold) break;

          // if its impossible to reach the threshold even if assuming
          // the remaining entries are all L_QUANTITY_MAX, then we can
          // just quit now, w/o decrypting the rest of the entries
          if ((sum + ((double)(ciphers.end() - (it + 1))) * L_QUANTITY_MAX) <=
              (double) threshold) break;
      }

      if (sum > (double) threshold) state->l_orderkeys.push_back(row[0].data);
    }
  }
  CryptoManager* cm;
  _q18_noopt_task_state* state;
  double threshold;
};

static void do_query_q18_crypt(ConnectNew &conn,
                               CryptoManager& cm,
                               uint64_t threshold,
                               vector<q18entry> &results) {
    NamedTimer fcnTimer(__func__);

    static const double L_QUANTITY_MAX = 50.0; // comes from statistics

    size_t minGroupCount = (size_t)ceil(double(threshold)/L_QUANTITY_MAX);
    assert(minGroupCount >= 1);

    ostringstream s;

    // query 1
    s <<
        "select l_orderkey_DET, group_concat(l_quantity_DET) "
        "from lineitem_enc_rowid "
        "group by l_orderkey_DET "
        "having count(*) >= " << minGroupCount;
    cerr  << s.str() << endl;

    DBResultNew * dbres;
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

    vector<string> l_orderkeys;
    if (DoParallel) {
        NamedTimer t(__func__, "aggregating");

        using namespace exec_service;

        vector<_q18_noopt_task_state> states( NumThreads );
        vector<_q18_noopt_task> tasks( NumThreads );
        vector<SqlRowGroup> groups;
        SplitRowsIntoGroups(groups, res.rows, NumThreads);

        for (size_t i = 0; i < NumThreads; i++) {
          tasks[i].cm        = &cm;
          tasks[i].state     = &states[i];
          tasks[i].threshold = threshold;
          states[i].rows     = groups[i];
        }

        Exec<_q18_noopt_task>::DefaultExecutor exec( NumThreads );
        exec.start();
        for (size_t i = 0; i < NumThreads; i++) {
          exec.submit(tasks[i]);
        }
        exec.stop(); // blocks until completion

        for (size_t i = 0; i < NumThreads; i++) {
          l_orderkeys.insert(l_orderkeys.end(),
                             states[i].l_orderkeys.begin(),
                             states[i].l_orderkeys.end());
        }
    } else {
        NamedTimer t(__func__, "aggregating");
        for (auto row : res.rows) {
            vector<string> ciphers;
            tokenize(row[1].data, ",", ciphers);
            assert(!ciphers.empty());

            double sum = 0.0;
            for (vector<string>::iterator it = ciphers.begin();
                 it != ciphers.end(); ++it) {
                uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
                        *it,
                        12345,
                        fieldname(lineitem::l_quantity, "DET"),
                        TYPE_INTEGER,
                        oDET,
                        cm);
                double l_quantity = ((double)l_quantity_int)/100.0;
                sum += l_quantity;

                // short circuit evaluations:

                // if we are already past the threshold, then quit
                if (sum > (double) threshold) break;

                // if its impossible to reach the threshold even if assuming
                // the remaining entries are all L_QUANTITY_MAX, then we can
                // just quit now, w/o decrypting the rest of the entries
                if ((sum + ((double)(ciphers.end() - (it + 1))) * L_QUANTITY_MAX) <=
                    (double) threshold) break;
            }

            if (sum > (double) threshold) l_orderkeys.push_back(row[0].data);
        }
    }

    ostringstream s1;
    // query 2

    if (l_orderkeys.empty()) return;

    //string pkinfo = marshallBinary(cm.getPKInfo());
    s1 <<
        "select "
        "    c_name_DET, c_custkey_DET, o_orderkey_DET, "
        "    o_orderdate_DET, o_totalprice_DET, group_concat(l_quantity_DET) "
        "from "
        "    customer_enc_rowid, orders_enc, lineitem_enc_rowid "
        "where "
        "    o_orderkey_DET in ( "
        << join(l_orderkeys, ",") <<
        "    ) "
        "    and c_custkey_DET = o_custkey_DET "
        "    and o_orderkey_DET = l_orderkey_DET "
        "group by "
        "    c_name_DET, "
        "    c_custkey_DET, "
        "    o_orderkey_DET, "
        "    o_orderdate_DET, "
        "    o_totalprice_DET "
        "order by "
        "    o_totalprice_OPE desc, "
        "    o_orderdate_OPE "
        "limit 100";

    {
        DBResultNew * dbres;
        {
            NamedTimer t(__func__, "execute");
            conn.execute(s1.str(), dbres);
        }
        ResType res;
        {
            NamedTimer t(__func__, "unpack");
            res = dbres->unpack();
            assert(res.ok);
        }

        for (auto row : res.rows) {

            string c_name = decryptRow<string>(
                    row[0].data,
                    12345,
                    fieldname(customer::c_name, "DET"),
                    TYPE_TEXT,
                    oDET,
                    cm);

            uint64_t c_custkey = decryptRowFromTo<uint64_t, 4>(
                    row[1].data,
                    12345,
                    fieldname(customer::c_custkey, "DET"),
                    TYPE_INTEGER,
                    SECLEVEL::DETJOIN,
                    getMin(oDET),
                    cm);

            uint64_t o_orderkey = decryptRowFromTo<uint64_t, 4>(
                    row[2].data,
                    12345,
                    fieldname(customer::c_custkey, "DET"),
                    TYPE_INTEGER,
                    SECLEVEL::DETJOIN,
                    getMin(oDET),
                    cm);

            uint64_t o_orderdate = decryptRow<uint64_t, 3>(
                    row[3].data,
                    12345,
                    fieldname(orders::o_orderdate, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    cm);

            uint64_t o_totalprice_int = decryptRow<uint64_t, 7>(
                    row[4].data,
                    12345,
                    fieldname(orders::o_totalprice, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    cm);
            double o_totalprice = ((double)o_totalprice_int)/100.0;

            //ZZ z;
            //cm.decrypt_Paillier(row[5].data, z);
            //long sum_qty_int = extract_from_slot(z, 0);
            //double sum_qty = ((double)sum_qty_int)/100.0;

            double sum_qty = 0.0;
            vector<string> ciphers;
            tokenize(row[5].data, ",", ciphers);
            for (vector<string>::iterator it = ciphers.begin();
                 it != ciphers.end(); ++it) {
              uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
                      *it,
                      12345,
                      fieldname(lineitem::l_quantity, "DET"),
                      TYPE_INTEGER,
                      oDET,
                      cm);
              sum_qty += ((double)l_quantity_int)/100.0;
            }

            results.push_back(
                q18entry(
                    c_name,
                    c_custkey,
                    o_orderkey,
                    o_orderdate,
                    o_totalprice,
                    sum_qty));
        }
    }
}

static void do_query_q18_crypt_opt2(ConnectNew &conn,
                                    CryptoManager& cm,
                                    uint64_t threshold,
                                    vector<q18entry> &results) {
    NamedTimer fcnTimer(__func__);

    double L_QUANTITY_MAX = 50.0; // comes from statistics

    size_t minGroupCount = (size_t)ceil(double(threshold)/L_QUANTITY_MAX);
    assert(minGroupCount >= 1);

    ostringstream s;

    // query 1
    string pkinfo = marshallBinary(cm.getPKInfo());
    s <<
        "select "
            "l_orderkey_DET, "
            "agg(l_bitpacked_AGG, " << pkinfo << ", " << minGroupCount << ") END "
        "from lineitem_enc "
        "group by l_orderkey_DET "
        "having count(*) >= " << minGroupCount;

    DBResultNew * dbres;
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

    vector<string> l_orderkeys;
    {
        NamedTimer t(__func__, "decrypt-part1");
        for (auto row : res.rows) {
            ZZ z;
            cm.decrypt_Paillier(row[1].data, z);
            long sum_qty_int = extract_from_slot(z, 0);
            double sum_qty = ((double)sum_qty_int)/100.0;

            if (sum_qty > (double) threshold) {
                l_orderkeys.push_back(row[0].data);
            }
        }
    }

    cerr << join(l_orderkeys, ",") << endl;

    ostringstream s1;
    // query 2

    assert(!l_orderkeys.empty());
    s1 <<
        "select "
        "    c_name_DET, c_custkey_DET, o_orderkey_DET, "
        "    o_orderdate_DET, o_totalprice_DET, agg(l_bitpacked_AGG, " << pkinfo << ") "
        "from "
        "    customer_enc_rowid, orders_enc, lineitem_enc "
        "where "
        "    o_orderkey_DET in ( "
        << join(l_orderkeys, ",") <<
        "    ) "
        "    and c_custkey_DET = o_custkey_DET "
        "    and o_orderkey_DET = l_orderkey_DET "
        "group by "
        "    c_name_DET, "
        "    c_custkey_DET, "
        "    o_orderkey_DET, "
        "    o_orderdate_DET, "
        "    o_totalprice_DET "
        "order by "
        "    o_totalprice_OPE desc, "
        "    o_orderdate_OPE "
        "limit 100";

    {
        DBResultNew * dbres;
        {
            NamedTimer t(__func__, "execute");
            conn.execute(s1.str(), dbres);
        }
        ResType res;
        {
            NamedTimer t(__func__, "unpack");
            res = dbres->unpack();
            assert(res.ok);
        }

        for (auto row : res.rows) {

            string c_name = decryptRow<string>(
                    row[0].data,
                    12345,
                    fieldname(customer::c_name, "DET"),
                    TYPE_TEXT,
                    oDET,
                    cm);

            uint64_t c_custkey = decryptRowFromTo<uint64_t, 4>(
                    row[1].data,
                    12345,
                    fieldname(customer::c_custkey, "DET"),
                    TYPE_INTEGER,
                    SECLEVEL::DETJOIN,
                    getMin(oDET),
                    cm);

            uint64_t o_orderkey = decryptRowFromTo<uint64_t, 4>(
                    row[2].data,
                    12345,
                    fieldname(customer::c_custkey, "DET"),
                    TYPE_INTEGER,
                    SECLEVEL::DETJOIN,
                    getMin(oDET),
                    cm);

            uint64_t o_orderdate = decryptRow<uint64_t>(
                    row[3].data,
                    12345,
                    fieldname(orders::o_orderdate, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    cm);

            uint64_t o_totalprice_int = decryptRow<uint64_t, 7>(
                    row[4].data,
                    12345,
                    fieldname(orders::o_totalprice, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    cm);
            double o_totalprice = ((double)o_totalprice_int)/100.0;

            ZZ z;
            cm.decrypt_Paillier(row[5].data, z);
            long sum_qty_int = extract_from_slot(z, 0);
            double sum_qty = ((double)sum_qty_int)/100.0;

            results.push_back(
                q18entry(
                    c_name,
                    c_custkey,
                    o_orderkey,
                    o_orderdate,
                    o_totalprice,
                    sum_qty));
        }
    }
}

static void do_query_q18_crypt_opt(ConnectNew &conn,
                                   CryptoManager& cm,
                                   uint64_t threshold,
                                   vector<q18entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;

    // query 1
    string pkinfo = marshallBinary(cm.getPKInfo());
    s <<
        "select "
            "l_orderkey_DET, "
            "CASE count(*) WHEN 1 THEN l_quantity_DET ELSE NULL END, "
            "CASE count(*) WHEN 1 THEN NULL ELSE agg(l_bitpacked_AGG, " << pkinfo << ") END "
        "from lineitem_enc group by l_orderkey_DET";

    DBResultNew * dbres;
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

    vector<string> l_orderkeys;
    for (auto row : res.rows) {
        if (!row[1].null) {
            assert(row[2].null);

            uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
                    row[1].data,
                    12345,
                    fieldname(lineitem::l_quantity, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    cm);
            double l_quantity = ((double)l_quantity_int)/100.0;

            if (l_quantity > (double) threshold) {
                l_orderkeys.push_back(row[0].data);
            }
        } else {
            assert(!row[2].null);

            ZZ z;
            cm.decrypt_Paillier(row[2].data, z);
            long sum_qty_int = extract_from_slot(z, 0);
            double sum_qty = ((double)sum_qty_int)/100.0;

            if (sum_qty > (double) threshold) {
                l_orderkeys.push_back(row[0].data);
            }
        }
    }


    ostringstream s1;
    // query 2

    assert(!l_orderkeys.empty());
    s1 <<
        "select "
        "    c_name_DET, c_custkey_DET, o_orderkey_DET, "
        "    o_orderdate_DET, o_totalprice_DET, agg(l_bitpacked_AGG, " << pkinfo << ") "
        "from "
        "    customer_enc_rowid, orders_enc, lineitem_enc "
        "where "
        "    o_orderkey_DET in ( "
        << join(l_orderkeys, ",") <<
        "    ) "
        "    and c_custkey_DET = o_custkey_DET "
        "    and o_orderkey_DET = l_orderkey_DET "
        "group by "
        "    c_name_DET, "
        "    c_custkey_DET, "
        "    o_orderkey_DET, "
        "    o_orderdate_DET, "
        "    o_totalprice_DET "
        "order by "
        "    o_totalprice_OPE desc, "
        "    o_orderdate_OPE "
        "limit 100";

    {
        DBResultNew * dbres;
        {
            NamedTimer t(__func__, "execute");
            conn.execute(s1.str(), dbres);
        }
        ResType res;
        {
            NamedTimer t(__func__, "unpack");
            res = dbres->unpack();
            assert(res.ok);
        }

        for (auto row : res.rows) {

            string c_name = decryptRow<string>(
                    row[0].data,
                    12345,
                    fieldname(customer::c_name, "DET"),
                    TYPE_TEXT,
                    oDET,
                    cm);

            uint64_t c_custkey = decryptRowFromTo<uint64_t, 4>(
                    row[1].data,
                    12345,
                    fieldname(customer::c_custkey, "DET"),
                    TYPE_INTEGER,
                    SECLEVEL::DETJOIN,
                    getMin(oDET),
                    cm);

            uint64_t o_orderkey = decryptRowFromTo<uint64_t, 4>(
                    row[2].data,
                    12345,
                    fieldname(customer::c_custkey, "DET"),
                    TYPE_INTEGER,
                    SECLEVEL::DETJOIN,
                    getMin(oDET),
                    cm);

            uint64_t o_orderdate = decryptRow<uint64_t>(
                    row[3].data,
                    12345,
                    fieldname(orders::o_orderdate, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    cm);

            uint64_t o_totalprice_int = decryptRow<uint64_t, 7>(
                    row[4].data,
                    12345,
                    fieldname(orders::o_totalprice, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    cm);
            double o_totalprice = ((double)o_totalprice_int)/100.0;

            ZZ z;
            cm.decrypt_Paillier(row[5].data, z);
            long sum_qty_int = extract_from_slot(z, 0);
            double sum_qty = ((double)sum_qty_int)/100.0;

            results.push_back(
                q18entry(
                    c_name,
                    c_custkey,
                    o_orderkey,
                    o_orderdate,
                    o_totalprice,
                    sum_qty));
        }
    }
}

static void do_query_q20(ConnectNew &conn,
                         uint64_t year,
                         const string &p_name,
                         const string &n_name,
                         vector<q20entry> &results) {
    NamedTimer fcnTimer(__func__);

    // mysql can't handle nested queries very well , so go ahead
    // and flatten everything

    //ostringstream s;
    //s <<
    //"select SQL_NO_CACHE s_name, s_address from SUPPLIER, NATION "
    //"where s_suppkey in ( "
    //    "select ps_suppkey from ( "
    //    "select ps_suppkey, ps_availqty from PARTSUPP, LINEITEM "
    //    " where "
    //    "     ps_partkey = l_partkey and "
    //    "     ps_suppkey = l_suppkey and "
    //    "     ps_partkey in ( "
    //    // TODO: this is slight deviation from TPC-H query, but it makes
    //    // the comparison more fair (like 'token%' is probably implemented
    //    // efficiently as a prefix scan)
    //    "       select p_partkey from PART where p_name like '%" << p_name << "%' "
    //    "     ) "
    //    "     and l_shipdate >= date '" << year << "-01-01'"
    //    "     and l_shipdate < date '" << year << "-01-01' + interval '1' year"
    //    " group by ps_partkey, ps_suppkey "
    //    " having ps_availqty > 0.5 * sum(l_quantity) "
    //    ") as __anon__ "
    //") "
    //"and s_nationkey = n_nationkey "
    //"and n_name = '" << n_name << "' "
    //"order by s_name";
    //cerr << s.str() << endl;

    //DBResultNew * dbres;
    //{
    //  NamedTimer t(__func__, "execute");
    //  conn.execute(s.str(), dbres);
    //}
    //ResType res;
    //{
    //  NamedTimer t(__func__, "unpack");
    //  res = dbres->unpack();
    //  assert(res.ok);
    //}

    //results.reserve(res.rows.size());
    //for (auto row : res.rows) {
    //  results.push_back(q20entry(row[0].data, row[1].data));
    //}


    vector<string> partkeys;
    {
        // TODO: this is slight deviation from TPC-H query, but it makes
        // the comparison more fair (like 'token%' is probably implemented
        // efficiently as a prefix scan)
        ostringstream s;
        s << "select SQL_NO_CACHE p_partkey from PART_INT where p_name like '%" << p_name << "%'";

        DBResultNew * dbres;
        {
          NamedTimer t(__func__, "execute-1");
          conn.execute(s.str(), dbres);
        }
        ResType res;
        {
          NamedTimer t(__func__, "unpack-1");
          res = dbres->unpack();
          assert(res.ok);
        }

        partkeys.reserve(res.rows.size());
        for (auto row : res.rows) {
            partkeys.push_back(row[0].data);
        }
    }
    assert(!partkeys.empty());

    vector<string> suppkeys;
    {
        ostringstream s;
        s <<
        "select SQL_NO_CACHE ps_suppkey, ps_availqty from PARTSUPP_INT, LINEITEM_INT "
        " where "
        "     ps_partkey = l_partkey and "
        "     ps_suppkey = l_suppkey and "
        "     ps_partkey in ( "
        << join(partkeys, ",") <<
        "     ) "
        "     and l_shipdate >= date '" << year << "-01-01'"
        "     and l_shipdate < date '" << year << "-01-01' + interval '1' year"
        " group by ps_partkey, ps_suppkey "
        " having ps_availqty * 100 > 0.5 * sum(l_quantity)";

        DBResultNew * dbres;
        {
          NamedTimer t(__func__, "execute-2");
          conn.execute(s.str(), dbres);
        }
        ResType res;
        {
          NamedTimer t(__func__, "unpack-2");
          res = dbres->unpack();
          assert(res.ok);
        }

        suppkeys.reserve(res.rows.size());
        for (auto row : res.rows) {
            suppkeys.push_back(row[0].data);
        }

        std::set<string> unique(suppkeys.begin(), suppkeys.end());
        suppkeys.clear();
        suppkeys.insert(suppkeys.begin(), unique.begin(), unique.end());
    }
    assert(!suppkeys.empty());

    {
        ostringstream s;
        s <<
        "select SQL_NO_CACHE s_name, s_address from SUPPLIER_INT, NATION_INT where s_suppkey in ( "
        << join(suppkeys ,",") <<
        ") and s_nationkey = n_nationkey and n_name = '" << n_name << "' order by s_name";

        DBResultNew * dbres;
        {
          NamedTimer t(__func__, "execute-3");
          conn.execute(s.str(), dbres);
        }
        ResType res;
        {
          NamedTimer t(__func__, "unpack-3");
          res = dbres->unpack();
          assert(res.ok);
        }
        for (auto row : res.rows) {
          results.push_back(q20entry(row[0].data, row[1].data));
        }
    }
}

static void do_query_q20_opt_noagg(ConnectNew &conn,
                                   CryptoManager& cm,
                                   uint64_t year,
                                   const string &p_name,
                                   const string &n_name,
                                   vector<q20entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    assert(n_name.size() <= 25);

    string lowerpname(lower_s(p_name));

    bool isBin = false;
    string encDATE_START = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);
    string encDATE_END = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year + 1)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);

    string encNAME = cm_stub.crypt(cm.getmkey(), n_name, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(n_name.size() == encNAME.size());
    assert(isBin);

    // right-pad encNAME with 0's
    encNAME.resize(25);

    Binary key(cm.getKey(cm.getmkey(), fieldname(part::p_name, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary(lowerpname));

    vector<string> partkeyDETs;
    {
        ostringstream s;
        s <<
        "select SQL_NO_CACHE p_partkey_DET from part_enc where searchSWP("
          << marshallBinary(string((char *)t.ciph.content, t.ciph.len))
          << ", "
          << marshallBinary(string((char *)t.wordKey.content, t.wordKey.len))
          << ", p_name_SWP) = 1";

        DBResultNew * dbres;
        {
            NamedTimer t(__func__, "execute-1");
            conn.execute(s.str(), dbres);
        }
        ResType res;
        {
            NamedTimer t(__func__, "unpack-1");
            res = dbres->unpack();
            assert(res.ok);
        }

        partkeyDETs.reserve(res.rows.size());
        for (auto row : res.rows) {
            partkeyDETs.push_back(row[0].data);
        }
    }
    assert(!partkeyDETs.empty());

    ostringstream s;
    //s <<
    //    "select SQL_NO_CACHE ps_partkey_DET, ps_suppkey_DET, ps_availqty_DET, l_quantity_DET "
    //    //"from partsupp_enc_noopt, lineitem_enc_noagg "
    //    "from lineitem_enc_noagg STRAIGHT_JOIN partsupp_enc_noopt "
    //    "where "
    //    "    ps_partkey_DET = l_partkey_DET and "
    //    "    ps_suppkey_DET = l_suppkey_DET and "
    //    "    ps_partkey_DET in ( "
    //    << join(partkeyDETs, ",") <<
    //    "    ) "
    //    "    and l_shipdate_OPE >= " << encDATE_START <<
    //    "    and l_shipdate_OPE < " << encDATE_END;
    //cerr << s.str() << endl;

    // this strategy seems to perform better than the one above
    s <<
        "select SQL_NO_CACHE ps_partkey_DET, ps_suppkey_DET, ps_availqty_DET, group_concat(l_quantity_DET) "
        "from partsupp_enc, lineitem_enc_rowid "
        "where "
        "    ps_partkey_DET = l_partkey_DET and "
        "    ps_suppkey_DET = l_suppkey_DET and "
        "    ps_partkey_DET in ( "
        << join(partkeyDETs, ",") <<
        "    ) "
        "    and l_shipdate_OPE >= " << encDATE_START <<
        "    and l_shipdate_OPE < " << encDATE_END <<
        "    group by ps_partkey_DET, ps_suppkey_DET";
    //cerr << s.str() << endl;

    DBResultNew * dbres;
    {
        NamedTimer t(__func__, "execute-2");
        conn.execute(s.str(), dbres);
    }
    ResType res;
    {
        NamedTimer t(__func__, "unpack-2");
        res = dbres->unpack();
        assert(res.ok);
    }

    typedef pair<string, string> MapKey;
    typedef pair<uint64_t, double> MapValue;
    typedef map<MapKey, MapValue> AggMap;
    AggMap groupBy;
    {
        NamedTimer t(__func__, "decrypt");
        for (auto row : res.rows) {

            // decrypt availqty
            uint64_t ps_availqty = decryptRow<uint64_t>(
                row[2].data,
                12345,
                fieldname(partsupp::ps_availqty, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

            vector<string> l_quantities;
            tokenize(row[3].data, ",", l_quantities);

            double l_quantity_sum = 0.0;
            for (vector<string>::iterator it = l_quantities.begin();
                 it != l_quantities.end(); ++it) {
              // decrypt l_quantity
              uint64_t l_quantity_int = decryptRow<uint64_t, 7>(
                      *it,
                      12345,
                      fieldname(lineitem::l_quantity, "DET"),
                      TYPE_INTEGER,
                      oDET,
                      cm);
              double l_quantity = ((double)l_quantity_int)/100.0;
              l_quantity_sum += l_quantity;
            }

            groupBy[make_pair(row[0].data, row[1].data)] =
              make_pair(ps_availqty, l_quantity_sum);
        }
    }

    // filter out the necessary ps_suppkeys
    vector<string> s_suppkeys;
    s_suppkeys.reserve(groupBy.size());
    for (AggMap::iterator it = groupBy.begin();
         it != groupBy.end(); ++it) {
        if ((double)it->second.first > 0.5 * it->second.second) {
            s_suppkeys.push_back(it->first.second);
        }
    }

    std::set<string> s_suppkeys_set(s_suppkeys.begin(), s_suppkeys.end());
    s_suppkeys.clear();
    s_suppkeys.insert(s_suppkeys.begin(), s_suppkeys_set.begin(), s_suppkeys_set.end());

    //assert(!s_suppkeys.empty());
    if (s_suppkeys.empty()) return;

    ostringstream s1;
    s1 <<
        "select s_name_DET, s_address_DET "
        "from supplier_enc, nation_enc "
        "where "
        "  s_suppkey_DET in (" << join(s_suppkeys, ", ") << ") and "
        "  s_nationkey_DET = n_nationkey_DET and "
        "  n_name_DET = " << marshallBinary(encNAME) << " "
        "order by s_name_OPE";
    //cerr << s1.str() << endl;

    {
      NamedTimer t(__func__, "execute-3");
      conn.execute(s1.str(), dbres);
    }
    {
      NamedTimer t(__func__, "unpack-3");
      res = dbres->unpack();
      assert(res.ok);
    }

    results.reserve(res.rows.size());
    for (auto row : res.rows) {
        string s_name = decryptRow<string>(
                row[0].data,
                12345,
                fieldname(supplier::s_name, "DET"),
                TYPE_TEXT,
                oDET,
                cm);
        string s_address = decryptRow<string>(
                row[1].data,
                12345,
                fieldname(supplier::s_address, "DET"),
                TYPE_TEXT,
                oDET,
                cm);
        results.push_back(q20entry(s_name, s_address));
    }
}

static void do_query_q20_opt(ConnectNew &conn,
                             CryptoManager& cm,
                             uint64_t year,
                             const string &p_name,
                             const string &n_name,
                             vector<q20entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    assert(n_name.size() <= 25);

    string lowerpname(lower_s(p_name));

    bool isBin = false;
    string encDATE_START = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);
    string encDATE_END = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year + 1)),
                              TYPE_INTEGER, "ope_join",
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);

    string encNAME = cm_stub.crypt(cm.getmkey(), n_name, TYPE_TEXT,
                              fieldname(nation::n_name, "DET"),
                              getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(n_name.size() == encNAME.size());
    assert(isBin);

    // right-pad encNAME with 0's
    encNAME.resize(25);

    Binary key(cm.getKey(cm.getmkey(), fieldname(part::p_name, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary(lowerpname));

    vector<string> partkeyDETs;
    {
        ostringstream s;
        s <<
        "select SQL_NO_CACHE p_partkey_DET from part_enc where searchSWP("
          << marshallBinary(string((char *)t.ciph.content, t.ciph.len))
          << ", "
          << marshallBinary(string((char *)t.wordKey.content, t.wordKey.len))
          << ", p_name_SWP) = 1";

        DBResultNew * dbres;
        {
            NamedTimer t(__func__, "execute-1");
            conn.execute(s.str(), dbres);
        }
        ResType res;
        {
            NamedTimer t(__func__, "unpack-1");
            res = dbres->unpack();
            assert(res.ok);
        }

        partkeyDETs.reserve(res.rows.size());
        for (auto row : res.rows) {
            partkeyDETs.push_back(row[0].data);
        }
    }

    string pkinfo = marshallBinary(cm.getPKInfo());
    ostringstream s;
    s <<
        "select SQL_NO_CACHE ps_suppkey_DET, ps_availqty_DET, agg(l_bitpacked_AGG, " << pkinfo << ") "
        "from partsupp_enc, lineitem_enc_rowid "
        "where "
        "    ps_partkey_DET = l_partkey_DET and "
        "    ps_suppkey_DET = l_suppkey_DET and "
        "    ps_partkey_DET in ( "
        << join(partkeyDETs, ",") <<
        "    ) "
        "    and l_shipdate_OPE >= " << encDATE_START <<
        "    and l_shipdate_OPE < " << encDATE_END << " "
        "group by ps_partkey_DET, ps_suppkey_DET";
    //cerr << s.str() << endl;

    DBResultNew * dbres;
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

    vector<string> s_suppkeys;
    {
        NamedTimer t(__func__, "decrypt");
        for (auto row : res.rows) {
            // decrypt availqty
            uint64_t ps_availqty = decryptRow<uint64_t>(
                row[1].data,
                12345,
                fieldname(partsupp::ps_availqty, "DET"),
                TYPE_INTEGER,
                oDET,
                cm);

            // decrypt l_quantity
            ZZ z;
            cm.decrypt_Paillier(row[2].data, z);
            long sum_qty_int = extract_from_slot(z, 0);
            double sum_qty = ((double)sum_qty_int)/100.0;

            if ((double)ps_availqty > 0.5 * sum_qty) {
                s_suppkeys.push_back(row[0].data);
            }
        }
    }

    std::set<string> s_suppkeys_set(s_suppkeys.begin(), s_suppkeys.end());
    s_suppkeys.clear();
    s_suppkeys.insert(s_suppkeys.begin(), s_suppkeys_set.begin(), s_suppkeys_set.end());

    //assert(!s_suppkeys.empty());
    if (s_suppkeys.empty()) return;

    ostringstream s1;
    s1 <<
        "select s_name_DET, s_address_DET "
        "from supplier_enc, nation_enc "
        "where "
        "  s_suppkey_DET in (" << join(s_suppkeys, ", ") << ") and "
        "  s_nationkey_DET = n_nationkey_DET and "
        "  n_name_DET = " << marshallBinary(encNAME) << " "
        "order by s_name_OPE";
    //cerr << s1.str() << endl;

    {
      NamedTimer t(__func__, "execute");
      conn.execute(s1.str(), dbres);
    }
    {
      NamedTimer t(__func__, "unpack");
      res = dbres->unpack();
      assert(res.ok);
    }

    results.reserve(res.rows.size());
    for (auto row : res.rows) {
        string s_name = decryptRow<string>(
                row[0].data,
                12345,
                fieldname(supplier::s_name, "DET"),
                TYPE_TEXT,
                oDET,
                cm);
        string s_address = decryptRow<string>(
                row[1].data,
                12345,
                fieldname(supplier::s_address, "DET"),
                TYPE_TEXT,
                oDET,
                cm);
        results.push_back(q20entry(s_name, s_address));
    }
}

static void do_query_q22(ConnectNew &conn,
                         const string& c1,
                         const string& c2,
                         const string& c3,
                         const string& c4,
                         const string& c5,
                         const string& c6,
                         const string& c7,
                         vector<q22entry> &results) {
  NamedTimer fcnTimer(__func__);

  // precompute inner query
  double avg;
  {
    ostringstream s;
    s <<
      "select "
      "  avg(c_acctbal) "
      "from "
      "  CUSTOMER_INT "
      "where "
      //"  c_acctbal > 0 and "
      "  substring(c_phone from 1 for 2) in "
      "('" << c1 << "', '" << c2 << "', '" << c3 << "', '" << c4 << "', '"
      << c5 << "', '" << c6 << "', '" << c7 << "') ";
    cerr << s.str() << endl;

    DBResultNew * dbres;
    {
      NamedTimer t(__func__, "execute-1");
      conn.execute(s.str(), dbres);
    }
    ResType res= dbres->unpack();
    assert(res.ok);
    assert(res.rows.size() == 1);
    avg = resultFromStr<double>(res.rows[0][0].data);
  }

  ostringstream s;
  s <<
    "select "
    "  sum_hash_agg_int("
    "    substring(c_phone from 1 for 2) as cntrycode, "
    "    c_acctbal) "
    "from "
    "  CUSTOMER_INT "
    "where "
    "  substring(c_phone from 1 for 2) in "
    "    ('" << c1 << "', '" << c2 << "', '" << c3 << "', '" << c4 << "', '"
    << c5 << "', '" << c6 << "', '" << c7 << "') "
    "  and c_acctbal > ( " << avg <<
    "  ) "
    "  and not exists ( "
    "    select "
    "      * "
    "    from "
    "      ORDERS_INT "
    "    where "
    "      o_custkey = c_custkey "
    "  ) "
  ;
  cerr << s.str() << endl;

  DBResultNew * dbres;
  {
    NamedTimer t(__func__, "execute-2");
    conn.execute(s.str(), dbres);
  }
  ResType res;
  {
    NamedTimer t(__func__, "unpack-2");
    res = dbres->unpack();
    assert(res.ok);
  }

  assert(res.rows.size() == 1);
  const string& data = res.rows[0][0].data;
  const uint8_t *p   = (const uint8_t *) data.data();
  const uint8_t *end = (const uint8_t *) data.data() + data.size();
  while (p < end) {
    string cntrycode = ReadKeyElem<string>(p);

    uint64_t cnt = *p;
    p += sizeof(uint64_t);

    const uint64_t *dp = (const uint64_t *) p;
    uint64_t value = *dp;
    p += sizeof(uint64_t);

    results.push_back(
        q22entry(
          cntrycode,
          cnt,
          value/100.0));
  }
}

static void do_query_crypt_q22(ConnectNew &conn,
                               CryptoManager& cm,
                               const string& c0,
                               const string& c1,
                               const string& c2,
                               const string& c3,
                               const string& c4,
                               const string& c5,
                               const string& c6,
                               vector<q22entry> &results,
                               const string& db) {
  crypto_manager_stub cm_stub(&cm, UseOldOpe);
  NamedTimer fcnTimer(__func__);

  // compute avg

  vector<string> codes = {c0, c1, c2, c3, c4, c5, c6};
  vector<string> codes_cts;
  codes_cts.reserve(codes.size());
  for (auto c : codes) {
    codes_cts.push_back(
      marshallBinary(
      enc_fixed_len_str(
          c, fieldname(customer::c_phone, "DET"), cm, 2)));
  }

  string filename =
    "/space/stephentu/data/" + db + "/customer_enc/row_pack/acctbal";
  string pkinfo = marshallBinary(cm.getPKInfo());

  ostringstream s;
  s <<
    "select"
    " agg_hash_agg_row_col_pack(0, row_id, "
      << pkinfo << ", " << "\"" << filename << "\", "
      << NumThreads << ", 256, 12, 1) "
    "from"
    "  customer_enc_rowid "
    "where c_phone_prefix_DET IN (" << join(codes_cts, ",") << ")"
    ;
  cerr << s.str() << endl;

  DBResultNew * dbres;
  {
    NamedTimer t(__func__, "execute-1");
    conn.execute(s.str(), dbres);
  }
  ResType res;
  {
    NamedTimer t(__func__, "unpack-1");
    res = dbres->unpack();
    assert(res.ok);
  }

  static const size_t BitsPerAggField = 83;
  ZZ mask = to_ZZ(1); mask <<= BitsPerAggField; mask -= 1;
  assert(NumBits(mask) == (int)BitsPerAggField);
  double sum = 0.0;
  uint64_t cnt = 0;
  {
    assert(res.rows.size() == 1);
    string data = res.rows[0][0].data;

    const uint8_t *p   = (const uint8_t *) data.data();
    const uint8_t *end = (const uint8_t *) data.data() + data.size();
    while (p < end) {

      cnt += *((uint64_t*)p);
      p += sizeof(uint64_t);

#define TAKE_FROM_SLOT(z, slot) \
      (to_long(((z) >> (BitsPerAggField * (slot))) & mask))

      const uint32_t *u32p = (const uint32_t *) p;
      uint32_t n_aggs = *u32p;
      p += sizeof(uint32_t);

      for (size_t group_i = 0; group_i < n_aggs; group_i++) {
        // interest mask
        const uint32_t *u32p = (const uint32_t *) p;
        uint32_t interest_mask = *u32p;
        p += sizeof(uint32_t);

        string ct = string((const char *) p, (size_t) 256);
        ZZ m;
        cm.decrypt_Paillier(ct, m);
        p += 256;

        for (size_t i = 0; i < 12; i++) {
          if (!(interest_mask & (0x1 << i))) continue;
          long l = TAKE_FROM_SLOT(m, i);
          sum += ((double)l)/100.0;
        }
      }
    }
  }

  double avg = sum / double(cnt);
  // encrypt avg in OPE
  cerr << "cnt: " << cnt << ", avg: " << avg << endl;

  bool isBin = false;
  string avgENC = cm_stub.crypt<7>(
      cm.getmkey(), to_s(roundToLong(avg * 100.0)),
      TYPE_INTEGER, fieldname(customer::c_acctbal, "OPE"),
      getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
  assert(isBin);
  avgENC.resize( 16 ); avgENC = str_reverse( avgENC );

  {
    ostringstream s;
    s <<
      "select "
      "  agg_hash_agg_row_col_pack(1, c_phone_prefix_DET, row_id, "
        << pkinfo << ", " << "\"" << filename << "\", "
        << NumThreads << ", 256, 12, 1) "
      "from "
      "  customer_enc_rowid "
      "where "
      "  c_phone_prefix_DET IN (" << join(codes_cts, ",") << ") "
      "  and c_acctbal_OPE > " << marshallBinary(avgENC) <<
      "  and not exists ( "
      "    select "
      "      * "
      "    from "
      "      orders_enc "
      "    where "
      "      o_custkey_DET = c_custkey_DET "
      "  ) "
      ;
    cerr << s.str() << endl;

    DBResultNew * dbres;
    {
      NamedTimer t(__func__, "execute-2");
      conn.execute(s.str(), dbres);
    }
    ResType res;
    {
      NamedTimer t(__func__, "unpack-2");
      res = dbres->unpack();
      assert(res.ok);
    }

    {
      assert(res.rows.size() == 1);
      const string& data = res.rows[0][0].data;
      const uint8_t *p   = (const uint8_t *) data.data();
      const uint8_t *end = (const uint8_t *) data.data() + data.size();
      while (p < end) {
        string c_phone_prefix_DET = ReadKeyElem<string>(p);

        string c_phone_prefix = decryptRow<string>(
                c_phone_prefix_DET,
                12345,
                fieldname(customer::c_phone, "DET"),
                TYPE_TEXT,
                oDET,
                cm);

        uint64_t cnt = *((uint64_t*)p);
        p += sizeof(uint64_t);

        const uint32_t *u32p = (const uint32_t *) p;
        uint32_t n_aggs = *u32p;
        p += sizeof(uint32_t);

        double sum = 0.0;
        for (size_t group_i = 0; group_i < n_aggs; group_i++) {
          // interest mask
          const uint32_t *u32p = (const uint32_t *) p;
          uint32_t interest_mask = *u32p;
          p += sizeof(uint32_t);

          string ct = string((const char *) p, (size_t) 256);
          ZZ m;
          cm.decrypt_Paillier(ct, m);
          p += 256;

          for (size_t i = 0; i < 12; i++) {
            if (!(interest_mask & (0x1 << i))) continue;
            long l = TAKE_FROM_SLOT(m, i);
            sum += ((double)l)/100.0;
          }
        }

        results.push_back(q22entry(c_phone_prefix, cnt, sum));
      }
    }
  }
}

static inline uint32_t random_year() {
    static const uint32_t gap = 1999 - 1993 + 1;
    return 1993 + (rand() % gap);

}

static void usage(char **argv) {
    cerr << "[USAGE]: " << argv[0] << " "
         << "(query_flag args) num_queries db_name (old_ope|new_ope) hostname"
         << endl;
}

static const bool PrintResults = true;

enum query_selection {
  query1,
  query2,
  query3,
  query4,
  query5,
  query6,
  query7,
  query8,
  query9,
  query10,
  query11,
  query12,

  query14,
  query15,

  query17,
  query18,
  query20,

  query22,
};

int main(int argc, char **argv) {
    srand(time(NULL));
    //if (argc != 6 && argc != 7 && argc != 8 && argc != 9 && argc != 10) {
    //    usage(argv);
    //    return 1;
    //}

    static const char * Query1Strings[] = {
        "--orig-query1",
        "--orig-nosort-int-query1",
        "--orig-nosort-double-query1",
        "--crypt-query1",
        "--crypt-opt-query1",
        "--crypt-opt-packed-query1",
        "--crypt-opt-nosort-query1",
        "--crypt-opt-row-packed-query1",
        "--crypt-opt-row-col-packed-query1",
    };
    std::set<string> Query1Modes
      (Query1Strings, Query1Strings + NELEMS(Query1Strings));

#define REGULAR_QUERY_IMPL(n) \
    static const char * Query ## n ## Strings[] = { \
        "--orig-query" #n, \
        "--crypt-query" #n, \
    }; \
    std::set<string> Query ## n ## Modes \
      (Query ## n ## Strings, Query ## n ## Strings + NELEMS(Query ## n ## Strings));

    REGULAR_QUERY_IMPL(2)
    REGULAR_QUERY_IMPL(3)
    REGULAR_QUERY_IMPL(4)
    REGULAR_QUERY_IMPL(5)
    REGULAR_QUERY_IMPL(6)
    REGULAR_QUERY_IMPL(7)
    REGULAR_QUERY_IMPL(8)
    REGULAR_QUERY_IMPL(9)
    REGULAR_QUERY_IMPL(10)

    REGULAR_QUERY_IMPL(12)
    REGULAR_QUERY_IMPL(15)
    REGULAR_QUERY_IMPL(17)
    REGULAR_QUERY_IMPL(22)

#undef REGULAR_QUERY_IMPL

    static const char * Query11Strings[] = {
        "--orig-query11",
        "--orig-query11-nosubquery",
        "--crypt-query11",
        "--crypt-opt-query11",
        "--crypt-opt-proj-query11",
    };
    std::set<string> Query11Modes
      (Query11Strings, Query11Strings + NELEMS(Query11Strings));

    static const char * Query14Strings[] = {
        "--orig-query14",
        "--crypt-query14",
        "--crypt-opt-tables-query14",
        "--crypt-opt-query14",
        "--crypt-opt2-query14",
    };
    std::set<string> Query14Modes
      (Query14Strings, Query14Strings + NELEMS(Query14Strings));

    static const char * Query18Strings[] = {
        "--orig-query18",
        "--crypt-query18",
        "--crypt-opt-query18",
        "--crypt-opt2-query18",
    };
    std::set<string> Query18Modes
      (Query18Strings, Query18Strings + NELEMS(Query18Strings));

    static const char * Query20Strings[] = {
        "--orig-query20",
        "--crypt-noagg-query20",
        "--crypt-agg-query20",
    };
    std::set<string> Query20Modes
      (Query20Strings, Query20Strings + NELEMS(Query20Strings));

    enum query_selection q;

    pair< std::set<string>*, enum query_selection > modes[] = {
#define CASE_IMPL(n) { &Query ## n ## Modes, query ## n }
      CASE_IMPL(1), CASE_IMPL(2), CASE_IMPL(3), CASE_IMPL(4),
      CASE_IMPL(5), CASE_IMPL(6), CASE_IMPL(7), CASE_IMPL(8),
      CASE_IMPL(9), CASE_IMPL(10), CASE_IMPL(11), CASE_IMPL(12),

      CASE_IMPL(14), CASE_IMPL(15),
      CASE_IMPL(17), CASE_IMPL(18), CASE_IMPL(20), CASE_IMPL(22),
#undef CASE_IMPL
    };

    for (size_t i = 0; i < NELEMS(modes); i++) {
      if (modes[i].first->find(argv[1]) != modes[i].first->end()) {
        q = modes[i].second;
        break;
      }
      if (i + 1 == NELEMS(modes)) {
        usage(argv);
        return 1;
      }
    }

    int input_nruns = 0;
    string db_name;
    string ope_type;
    string do_par;
    string hostname;
    string db_type;
    switch (q) {

#define CASE_IMPL(n, o) \
    case query ## n: { \
      input_nruns = atoi(argv[o + 2]); \
      db_name     =      argv[o + 3]; \
      ope_type    =      argv[o + 4]; \
      do_par      =      argv[o + 5]; \
      hostname    =      argv[o + 6]; \
      db_type     =      argv[o + 7]; \
      break; \
    }

    CASE_IMPL( 1, 1)
    CASE_IMPL( 2, 3)
    CASE_IMPL( 3, 2)
    CASE_IMPL( 4, 1)
    CASE_IMPL( 5, 2)
    CASE_IMPL( 6, 3)
    CASE_IMPL( 7, 2)
    CASE_IMPL( 8, 3)
    CASE_IMPL( 9, 1)
    CASE_IMPL(10, 1)
    CASE_IMPL(11, 2)
    CASE_IMPL(12, 3)
    CASE_IMPL(14, 1)
    CASE_IMPL(15, 1)
    CASE_IMPL(17, 2)
    CASE_IMPL(18, 1)
    CASE_IMPL(20, 3)
    CASE_IMPL(22, 7)

#undef CASE_IMPL

    }
    uint32_t nruns = (uint32_t) input_nruns;

    assert(ope_type == "old_ope" || ope_type == "new_ope");
    UseOldOpe = ope_type == "old_ope";

    assert(do_par == "no_parallel" || do_par == "parallel");
    DoParallel = do_par == "parallel";

    assert(db_type == "mysql" || db_type == "postgres");
    UseMySQL = db_type == "mysql";

    unsigned long ctr = 0;

    ConnectNew* _conn =
      (UseMySQL) ? (ConnectNew*) new MySQLConnect(hostname, "root", "", db_name, 3307) :
                   (ConnectNew*) new PGConnect(hostname, "stephentu", "letmein", db_name, 5432);
    ConnectNew& conn = *_conn;

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
          } else if (mode == "orig-nosort-int-query1") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q1_nosort_int(conn, year, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "orig-nosort-double-query1") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q1_nosort_double(conn, year, results);
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
          } else if (mode == "crypt-opt-nosort-query1") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q1_opt_nosort(conn, cm, year, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-opt-row-col-packed-query1") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q1_opt_row_col_pack(conn, cm, year, results, db_name);
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

      case query3:
        {
          string mktsegment = argv[2];
          string d = argv[3];
          vector<q3entry> results;
          if (mode == "orig-query3") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q3(conn, mktsegment, d, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query3") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q3_crypt(conn, cm, mktsegment, d, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query4:
        {
          string d = argv[2];
          vector<q4entry> results;
          if (mode == "orig-query4") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q4(conn, d, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query4") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q4(conn, cm, d, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query5:
        {
          string name = argv[2];
          string d = argv[3];
          vector<q5entry> results;
          if (mode == "orig-query5") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q5(conn, name, d, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query5") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q5(conn, cm, name, d, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query6:
        {
          string d = argv[2];
          double discount = atof(argv[3]);
          uint64_t quantity = atoi(argv[4]);
          vector<q6entry> results;
          if (mode == "orig-query6") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q6(conn, d, discount, quantity, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query6") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q6(conn, cm, d, discount, quantity, results, db_name);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query7:
        {
          string n_a = argv[2];
          string n_b = argv[3];
          vector<q7entry> results;
          if (mode == "orig-query7") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q7(conn, n_a, n_b, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query7") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q7(conn, cm, n_a, n_b, results, db_name);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query8:
        {
          string n_a = argv[2];
          string r_a = argv[3];
          string p_a = argv[4];
          vector<q8entry> results;
          if (mode == "orig-query8") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q8(conn, n_a, r_a, p_a, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query8") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q8(conn, cm, n_a, r_a, p_a, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query9:
        {
          string p_a = argv[2];
          vector<q9entry> results;
          if (mode == "orig-query9") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q9(conn, p_a, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query9") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q9(conn, cm, p_a, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query10:
        {
          string o_a = argv[2];
          vector<q10entry> results;
          if (mode == "orig-query10") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q10(conn, o_a, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query10") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q10(conn, cm, o_a, results);
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
          } else if (mode == "orig-query11-nosubquery") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q11_nosubquery(conn, nation, fraction, results);
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

      case query12:
        {
          string l_a = argv[2];
          string l_b = argv[3];
          string l_c = argv[4];
          vector<q12entry> results;
          if (mode == "orig-query12") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q12(conn, l_a, l_b, l_c, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query12") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q12(conn, cm, l_a, l_b, l_c, results);
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
          } else if (mode == "crypt-query14") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q14_noopt(conn, cm, year, results, false);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-opt-tables-query14") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q14_noopt(conn, cm, year, results, true);
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
          } else if (mode == "crypt-opt2-query14") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q14_opt2(conn, cm, year, results, db_name);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query15:
        {
          string l_a = argv[2];
          vector<q15entry> results;
          if (mode == "orig-query15") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q15(conn, l_a, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query15") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q15(conn, cm, l_a, results, db_name);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query17:
        {
          string p_a = argv[2];
          string p_b = argv[3];
          vector<q17entry> results;
          if (mode == "orig-query17") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q17(conn, p_a, p_b, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query17") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q17(conn, cm, p_a, p_b, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query18:
        {
          int input_threshold = atoi(argv[2]);
          assert(input_threshold >= 0);
          uint32_t threshold = (uint32_t) input_threshold;

          vector<q18entry> results;

          if (mode == "orig-query18") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q18(conn, threshold, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query18") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q18_crypt(conn, cm, threshold, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-opt-query18") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q18_crypt_opt(conn, cm, threshold, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-opt2-query18") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q18_crypt_opt2(conn, cm, threshold, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;
      case query20:
        {
          int input_year = atoi(argv[2]);
          assert(input_year >= 0);
          uint32_t year = (uint32_t) input_year;
          string p_name = argv[3];
          string n_name = argv[4];
          vector<q20entry> results;

          if (mode == "orig-query20") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q20(conn, year, p_name, n_name, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-noagg-query20") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q20_opt_noagg(conn, cm, year, p_name, n_name, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-agg-query20") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q20_opt(conn, cm, year, p_name, n_name, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else assert(false);
        }
        break;

      case query22:
        {
          string c0 = argv[2];
          string c1 = argv[3];
          string c2 = argv[4];
          string c3 = argv[5];
          string c4 = argv[6];
          string c5 = argv[7];
          string c6 = argv[8];
          vector<q22entry> results;
          if (mode == "orig-query22") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_q22(conn, c0, c1, c2, c3, c4, c5, c6, results);
              ctr += results.size();
              PRINT_RESULTS();
              results.clear();
            }
          } else if (mode == "crypt-query22") {
            for (size_t i = 0; i < nruns; i++) {
              do_query_crypt_q22(conn, cm, c0, c1, c2, c3, c4, c5, c6, results, db_name);
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
