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
#include <crypto/paillier.hh>
#include <util/executor.hpp>
#include <util/util.hh>

using namespace std;
using namespace NTL;

static bool UseOldOpe = false;
static bool DoParallel = true;
static bool UseMYISAM = false;
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
  date_t(
    uint32_t day,
    uint32_t month,
    uint32_t year) {}

  inline uint32_t encode() const { return EncodeDate(month, day, year); }

  uint32_t day;
  uint32_t month;
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

  static void q1_impl(Connect& conn, uint32_t year, vector<q1entry>& results) {
    NamedTimer fcnTimer(__func__);

    const char *agg_name = Q1NoSortImplInfo<T>::agg_func_name();
    const char *unit = Q1NoSortImplInfo<T>::unit();

    ostringstream buf;
    buf << "select SQL_NO_CACHE " <<  agg_name << "(l_returnflag, l_linestatus, l_quantity) as sum_qty, " <<  agg_name << "(l_returnflag, l_linestatus, l_extendedprice) as sum_base_price, " <<  agg_name << "(l_returnflag, l_linestatus, l_extendedprice * (" << unit << " - l_discount)) as sum_disc_price, " <<  agg_name << "(l_returnflag, l_linestatus, l_extendedprice * (" << unit << " - l_discount) * (" << unit << " + l_tax)) as sum_charge, " <<  agg_name << "(l_returnflag, l_linestatus, l_discount) as sum_disc from " << Q1NoSortImplInfo<T>::table_name()
        << " where l_shipdate <= date '" << year << "-1-1'"
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

static void do_query_q1_nosort_int(Connect &conn,
                                   uint32_t year,
                                   vector<q1entry> &results) {
  Q1NoSortImpl<uint64_t>::q1_impl(conn, year, results);
}

static void do_query_q1_nosort_double(Connect &conn,
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

static void do_query_q1_packed_opt(Connect &conn,
                                   CryptoManager &cm,
                                   uint32_t year,
                                   vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
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

static void do_query_q1_opt_nosort(Connect &conn,
                                   CryptoManager &cm,
                                   uint32_t year,
                                   vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    DBResult * dbres;

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

static void do_query_q1_opt_row_col_pack(Connect &conn,
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
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    DBResult * dbres;

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
      ") FROM " << (UseMYISAM ? "lineitem_enc_noagg_rowid_MYISAM" : "lineitem_enc_noagg_rowid")
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

static void do_query_q1_opt(Connect &conn,
                            CryptoManager &cm,
                            uint32_t year,
                            vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
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

static void do_query_q1_noopt(Connect &conn,
                              CryptoManager &cm,
                              uint32_t year,
                              vector<q1entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    // l_shipdate <= date '[year]-01-01'
    bool isBin;
    string encDATE = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year)),
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

      << "FROM " << (UseMYISAM ? "lineitem_enc_noagg_rowid_MYISAM" : "lineitem_enc_noagg_rowid")
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
      << "FROM part_enc, supplier_enc, partsupp_enc_noopt, nation_enc, region_enc "
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
          << "FROM partsupp_enc_noopt, supplier_enc, nation_enc, region_enc "
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

struct _q3_noopt_task_state {
  _q3_noopt_task_state() {}
  vector< vector< SqlItem > > rows;

  list<q3entry_enc> bestSoFar;
  static const size_t TopN = 10;
};

struct q3entry_enc_cmp {
  inline bool operator()(const q3entry_enc& a,
                         const q3entry_enc& b) const {
    // TODO: floating point equality errors
    return (a.revenue > b.revenue) ||
           (a.revenue == b.revenue && a.o_orderdate_OPE < b.o_orderdate_OPE);
  }
};

struct _q3_noopt_task {
  void operator()() {
      static q3entry_enc_cmp q3cmp;
      for (auto row : state->rows) {
          vector<string> ext_price_ciphers;
          tokenize(row[1].data, ",", ext_price_ciphers);

          vector<string> discount_ciphers;
          tokenize(row[2].data, ",", discount_ciphers);
          assert(!ext_price_ciphers.empty());
          assert(ext_price_ciphers.size() == discount_ciphers.size());

          double sum = 0.0;
          for (size_t i = 0; i < ext_price_ciphers.size(); i++) {
            uint64_t l_extendedprice_int = decryptRow<uint64_t, 7>(
                    ext_price_ciphers[i],
                    12345,
                    fieldname(lineitem::l_extendedprice, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    *cm);
            double l_extendedprice = ((double)l_extendedprice_int)/100.0;

            uint64_t l_discount_int = decryptRow<uint64_t, 7>(
                    discount_ciphers[i],
                    12345,
                    fieldname(lineitem::l_discount, "DET"),
                    TYPE_INTEGER,
                    oDET,
                    *cm);
            double l_discount = ((double)l_discount_int)/100.0;

            sum += (l_extendedprice * (1.0 - l_discount));
          }

          q3entry_enc cur_group(
                  resultFromStr<uint64_t>(row[0].data),
                  sum,
                  resultFromStr<uint64_t>(row[3].data),
                  resultFromStr<uint64_t>(row[4].data),
                  resultFromStr<uint64_t>(row[5].data));

          if (state->bestSoFar.empty()) {
            state->bestSoFar.push_back(cur_group);
            continue;
          }

          // check to see if we can discard this candidate
          // (linear search bestSoFar list, since topN size is small)
          auto it = state->bestSoFar.begin();
          for (; it != state->bestSoFar.end(); ++it) {
            if (q3cmp(cur_group, *it)) {
              break;
            }
          }

          if (it == state->bestSoFar.end()) {
            // cur group is smaller than every element on list
            if (state->bestSoFar.size() < _q3_noopt_task_state::TopN) {
              state->bestSoFar.push_back(cur_group);
            } else { /* drop */ }
          } else {
            state->bestSoFar.insert(it, cur_group);
            if (state->bestSoFar.size() > _q3_noopt_task_state::TopN) {
              state->bestSoFar.resize(_q3_noopt_task_state::TopN);
            }
          }
      }
  }
  CryptoManager* cm;
  _q3_noopt_task_state* state;
};


static void do_query_q3(Connect &conn,
                        const string& mktsegment,
                        const string& d,
                        vector<q3entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select l_orderkey, sum(l_extendedprice * (100 - l_discount)) as revenue, o_orderdate, o_shippriority, group_concat('a') "
      "from LINEITEM_INT straight_join ORDERS_INT on l_orderkey = o_orderkey straight_join CUSTOMER_INT on c_custkey = o_custkey "
      //"from LINEITEM_INT_MYISAM straight_join ORDERS_INT_MYISAM on l_orderkey = o_orderkey straight_join CUSTOMER_INT_MYISAM on c_custkey = o_custkey "
      //"from LINEITEM_INT_2 straight_join ORDERS_INT_INNODB on l_orderkey = o_orderkey straight_join CUSTOMER_INT_INNODB on c_custkey = o_custkey "
      "where c_mktsegment = '" << mktsegment << "' and o_orderdate < date '" << d << "' and l_shipdate > date '" << d << "' "
      "group by l_orderkey, o_orderdate, o_shippriority "
      "order by revenue desc, o_orderdate limit 10;"
      ;
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
          q3entry(
            resultFromStr<uint64_t>(row[0].data),
            resultFromStr<double>(row[1].data) / 10000.0,
            row[2].data,
            resultFromStr<uint64_t>(row[3].data)));
    }
}

static void do_query_q3_crypt(Connect &conn,
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
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);

    DBResult * dbres;

    ostringstream s;
    s << "select "
         "  l_orderkey_DET, "
         "  group_concat(l_extendedprice_DET), "
         "  group_concat(l_discount_DET), "
         "  o_orderdate_OPE, "
         "  o_orderdate_DET, "
         "  o_shippriority_DET "
         "from lineitem_enc_noagg_rowid straight_join orders_enc on l_orderkey_DET = o_orderkey_DET straight_join customer_enc on c_custkey_DET = o_custkey_DET "
         //"from lineitem_enc_noagg_rowid_MYISAM straight_join orders_enc_MYISAM on l_orderkey_DET = o_orderkey_DET straight_join customer_enc_MYISAM on c_custkey_DET = o_custkey_DET "
         //"from lineitem_enc_noagg_rowid straight_join orders_enc_INNODB on l_orderkey_DET = o_orderkey_DET straight_join customer_enc_INNODB on c_custkey_DET = o_custkey_DET "
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

      vector< list<q3entry_enc>::iterator > positions;
      positions.resize( NumThreads );
      for (size_t i = 0; i < NumThreads; i++) {
        positions[i] = states[i].bestSoFar.begin();
      }

      list< q3entry_enc > merged;
      static q3entry_enc_cmp q3cmp;
      while (merged.size() < _q3_noopt_task_state::TopN) {
        // look for next min
        q3entry_enc* minSoFar = NULL;
        size_t minSoFarIdx = 0;
        for (size_t i = 0; i < NumThreads; i++) {
          if (positions[i] != states[i].bestSoFar.end()) {
            if (!minSoFar || q3cmp(*positions[i], *minSoFar)) {
              minSoFar = &(*positions[i]);
              minSoFarIdx = i;
            }
          }
        }
        if (!minSoFar) break;

        positions[minSoFarIdx]++;

        if (merged.empty()) {
          merged.push_back(*minSoFar);
          continue;
        }

        auto it = merged.begin();
        for (; it != merged.end(); ++it) {
          if (q3cmp(*minSoFar, *it)) {
            break;
          }
        }

        if (it == merged.end()) {
          merged.push_back(*minSoFar);
        } else {
          merged.insert(it, *minSoFar);
        }
      }

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

static void do_query_q4(Connect &conn,
                        const string& d,
                        vector<q4entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select o_orderpriority, count(*) as order_count "
      "from ORDERS "
      "where o_orderdate >= date '" << d << "' "
      "and o_orderdate < date '" << d << "' + interval '3' month "
      "and exists ( "
      "    select * from "
      "      LINEITEM where l_orderkey = o_orderkey and l_commitdate < l_receiptdate) "
      "group by o_orderpriority "
      "order by o_orderpriority"
      ;
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
          q4entry(
            row[0].data,
            resultFromStr<uint64_t>(row[1].data)));
    }
}

static void do_query_crypt_q4(Connect &conn,
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

    string d1 =
      cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d) + (3 << 5) /* + 3 months */),
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
      "      lineitem_enc_noagg_rowid where l_orderkey_DET = o_orderkey_DET and l_commitdate_OPE < l_receiptdate_OPE) "
      "group by o_orderpriority_DET "
      ;
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

static void do_query_q5(Connect &conn,
                        const string& name,
                        const string& d,
                        vector<q5entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select"
      "	n_name,"
      "	sum(l_extendedprice * (100 - l_discount)) as revenue "
      "from"
      "	CUSTOMER,"
      "	ORDERS,"
      "	LINEITEM_INT,"
      "	SUPPLIER,"
      "	NATION,"
      "	REGION "
      "where"
      "	c_custkey = o_custkey"
      "	and l_orderkey = o_orderkey"
      "	and l_suppkey = s_suppkey"
      "	and c_nationkey = s_nationkey"
      "	and s_nationkey = n_nationkey"
      "	and n_regionkey = r_regionkey"
      "	and r_name = '" << name << "'"
      "	and o_orderdate >= date '" << d << "'"
      "	and o_orderdate < date '" << d << "' + interval '1' year "
      "group by"
      "	n_name "
      "order by"
      "	revenue desc;"
      ;
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
          q5entry(
            row[0].data,
            resultFromStr<double>(row[1].data) / 10000.0));
    }
}

struct _q5_noopt_task_state {
  _q5_noopt_task_state() {}
  vector< string > xps;
  vector< string > dcs;
  vector< double > revenue;
};

struct _q5_noopt_task {
  void operator()() {
    for (size_t i = 0; i < state->xps.size(); i++) {

        // l_extendedprice
        uint64_t l_extendedprice_int = decryptRow<uint64_t, 7>(
                state->xps[i],
                12345,
                fieldname(lineitem::l_extendedprice, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);
        double l_extendedprice = ((double)l_extendedprice_int)/100.0;

        // l_discount
        uint64_t l_discount_int = decryptRow<uint64_t, 7>(
                state->dcs[i],
                12345,
                fieldname(lineitem::l_discount, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);
        double l_discount = ((double)l_discount_int)/100.0;

        state->revenue.push_back(l_extendedprice * (1.0 - l_discount));
    }
  }
  CryptoManager* cm;
  _q5_noopt_task_state* state;
};

static void do_query_crypt_q5(Connect &conn,
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
      "	n_name_DET,"
      " group_concat(l_extendedprice_DET),"
      " group_concat(l_discount_DET) "
      "from"
      "	customer_enc,"
      "	orders_enc,"
      "	lineitem_enc_noagg_rowid,"
      "	supplier_enc,"
      "	nation_enc,"
      "	region_enc "
      "where"
      "	c_custkey_DET = o_custkey_DET"
      "	and l_orderkey_DET = o_orderkey_DET"
      "	and l_suppkey_DET = s_suppkey_DET"
      "	and c_nationkey_DET = s_nationkey_DET"
      "	and s_nationkey_DET = n_nationkey_DET"
      "	and n_regionkey_DET = r_regionkey_DET"
      "	and r_name_DET = " << marshallBinary(encNAME) << " "
      "	and o_orderdate_OPE >= " << d0 << " "
      "	and o_orderdate_OPE < " << d1 << " "
      "group by"
      "	n_name_DET"
      ;
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

    assert(DoParallel);
    vector<string> l_extendedprice_cts;
    vector<string> l_discount_cts;
    vector< pair<size_t, size_t> > offsets;
    for (auto row : res.rows) {
      size_t offset = l_extendedprice_cts.size();
      tokenize(row[1].data, ",", l_extendedprice_cts);
      tokenize(row[2].data, ",", l_discount_cts);

      offsets.push_back(
          make_pair(offset, l_extendedprice_cts.size() - offset));
    }

    assert(l_extendedprice_cts.size() ==
           l_discount_cts.size());

    vector< vector<string> > xp_groups;
    vector< vector<string> > dc_groups;
    SplitRowsIntoGroups(xp_groups, l_extendedprice_cts, NumThreads);
    SplitRowsIntoGroups(dc_groups, l_discount_cts, NumThreads);

    assert(xp_groups.size() == dc_groups.size());

    vector<_q5_noopt_task_state> states( NumThreads );
    vector<_q5_noopt_task> tasks( NumThreads );

    for (size_t i = 0; i < NumThreads; i++) {
      tasks[i].cm    = &cm;
      tasks[i].state = &states[i];
      states[i].xps  = xp_groups[i];
      states[i].dcs  = dc_groups[i];
      assert(states[i].xps.size() == states[i].dcs.size());
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

static void do_query_q6(Connect &conn,
                        const string& d,
                        double discount,
                        uint64_t quantity,
                        vector<q6entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select"
      "	sum(l_extendedprice * l_discount) as revenue "
      "from"
      "	LINEITEM_INT "
      "where"
      "	l_shipdate >= date '" << d << "'"
      "	and l_shipdate < date '" << d << "' + interval '1' year"
      "	and l_discount between " << roundToLong(discount*100.0)
        << " - 1 and " << roundToLong(discount*100.0) << " + 1.0"
      "	and l_quantity < " << roundToLong (quantity * 100.0 )
      ;
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
          q6entry(
            resultFromStr<double>(row[0].data) / 10000.0));
    }
}

static void do_query_crypt_q6(Connect &conn,
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
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);

    string d1 = cm_stub.crypt<3>(cm.getmkey(), to_s(encode_yyyy_mm_dd(d) + (1 << 9) /* + 1 yr */),
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
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
      "	lineitem_enc_noagg_rowid "
      "where"
      "	l_shipdate_OPE >= " << d0 << " "
      "	and l_shipdate_OPE < " << d1 << " "
      "	and l_discount_OPE >= " << marshallBinary(dsc0) << " "
      " and l_discount_OPE <= " << marshallBinary(dsc1) << " "
      "	and l_quantity_OPE < " << marshallBinary(qty)
      ;
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

static void do_query_q7(Connect &conn,
                        const string& n_a,
                        const string& n_b,
                        vector<q7entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
    "select"
    "	sum_hash_agg_int("
    "   n1.n_name as supp_nation,"
    "   n2.n_name as cust_nation,"
    "   extract(year from l_shipdate) as l_year,"
    "	  l_extendedprice * (100 - l_discount) as volume) "
    "from"
    "	SUPPLIER,"
    "	LINEITEM_INT,"
    "	ORDERS,"
    "	CUSTOMER,"
    "	NATION n1,"
    "	NATION n2 "
    "where"
    "	s_suppkey = l_suppkey"
    "	and o_orderkey = l_orderkey"
    "	and c_custkey = o_custkey"
    "	and s_nationkey = n1.n_nationkey"
    "	and c_nationkey = n2.n_nationkey"
    "	and ("
    "		(n1.n_name = '" << n_a << "' and n2.n_name = '" << n_b << "')"
    "		or (n1.n_name = '" << n_b << "' and n2.n_name = '" << n_a << "')"
    "	)"
    "	and l_shipdate between date '1995-01-01' and date '1996-12-31'";
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

static void do_query_crypt_q7(Connect &conn,
                              CryptoManager &cm,
                              const string& n_a,
                              const string& n_b,
                              vector<q7entry> &results,
                              const string& db) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin = false;
    string d0 = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, 1995)),
                                   TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);
    string d1 = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(12, 31, 1996)),
                                   TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
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
    "	agg_hash_agg_row_col_pack("
    "   3,"
    "   n1.n_name_DET,"
    "   n2.n_name_DET,"
    "   l_shipdate_year_DET,"
    "   row_id," << pkinfo << ", " <<
    "\"" << filename << "\", " << NumThreads << ", 256, 12, "
    "	  1) "
    "from"
    "	supplier_enc,"
    "	lineitem_enc_noagg_rowid,"
    "	orders_enc,"
    "	customer_enc,"
    "	nation_enc n1,"
    "	nation_enc n2 "
    "where"
    "	s_suppkey_DET = l_suppkey_DET"
    "	and o_orderkey_DET = l_orderkey_DET"
    "	and c_custkey_DET = o_custkey_DET"
    "	and s_nationkey_DET = n1.n_nationkey_DET"
    "	and c_nationkey_DET = n2.n_nationkey_DET"
    "	and ("
    "		(n1.n_name_DET = " << marshallBinary(n_a_enc)
    << " and n2.n_name_DET = " << marshallBinary(n_b_enc) << ")"
    "		or (n1.n_name_DET = " << marshallBinary(n_b_enc)
    << " and n2.n_name_DET = " << marshallBinary(n_a_enc) << ")"
    "	)"
    "	and l_shipdate_OPE >= " << d0 << " and l_shipdate_OPE <= " << d1;
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

static void do_query_q8(Connect &conn,
                        const string& n_a,
                        const string& r_a,
                        const string& p_a,
                        vector<q8entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s;
    s <<
      "select"
      "	o_year,"
      "	sum(case"
      "		when nation = '" << n_a << "' then volume"
      "		else 0"
      "	end) / sum(volume) as mkt_share "
      "from"
      "	("
      "		select"
      "			extract(year from o_orderdate) as o_year,"
      "			l_extendedprice * (100 - l_discount) as volume,"
      "			n2.n_name as nation"
      "		from"
      "			PART,"
      "			SUPPLIER,"
      "			LINEITEM_INT,"
      "			ORDERS,"
      "			CUSTOMER,"
      "			NATION n1,"
      "			NATION n2,"
      "			REGION"
      "		where"
      "			p_partkey = l_partkey"
      "			and s_suppkey = l_suppkey"
      "			and l_orderkey = o_orderkey"
      "			and o_custkey = c_custkey"
      "			and c_nationkey = n1.n_nationkey"
      "			and n1.n_regionkey = r_regionkey"
      "			and r_name = '" << r_a << "'"
      "			and s_nationkey = n2.n_nationkey"
      "			and o_orderdate between date '1995-01-01' and date '1996-12-31'"
      "			and p_type = '" << p_a <<"'"
      "	) as all_nations "
      "group by"
      "	o_year "
      "order by"
      "	o_year;";
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

        uint64_t l_extendedprice_int = decryptRow<uint64_t, 7>(
                row[1].data,
                12345,
                fieldname(lineitem::l_extendedprice, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);
        double l_extendedprice = ((double)l_extendedprice_int)/100.0;

        uint64_t l_discount_int = decryptRow<uint64_t, 7>(
                row[2].data,
                12345,
                fieldname(lineitem::l_discount, "DET"),
                TYPE_INTEGER,
                oDET,
                *cm);
        double l_discount = ((double)l_discount_int)/100.0;

        double value = l_extendedprice * (1.0 - l_discount);

        bool flag = row[3].data == "1";

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

static void do_query_crypt_q8(Connect &conn,
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
      "	IF(o_orderdate_OPE < " << boundary1 << ", 0, 1),"
      "	l_extendedprice_DET,"
      " l_discount_DET,"
      "	n2.n_name_DET = " << marshallBinary(n_a_enc) << " "
      "from"
      "	part_enc,"
      "	supplier_enc,"
      "	lineitem_enc_noagg_rowid,"
      "	orders_enc,"
      "	customer_enc,"
      "	nation_enc n1,"
      "	nation_enc n2,"
      "	region_enc "
      "where"
      "	p_partkey_DET = l_partkey_DET"
      "	and s_suppkey_DET = l_suppkey_DET"
      "	and l_orderkey_DET = o_orderkey_DET"
      "	and o_custkey_DET = c_custkey_DET"
      "	and c_nationkey_DET = n1.n_nationkey_DET"
      "	and n1.n_regionkey_DET = r_regionkey_DET"
      "	and r_name_DET = " << marshallBinary(r_a_enc) << " "
      "	and s_nationkey_DET = n2.n_nationkey_DET"
      "	and o_orderdate_OPE >= " << d0 << " and o_orderdate_OPE <= " << d1 <<
      "	and p_type_DET = " << marshallBinary(p_a_enc)
      ;
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

static void do_query_q11_noopt(Connect &conn,
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
      << "FROM partsupp_enc_noopt, supplier_enc, nation_enc "
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
          q11entry(resultFromStr<uint64_t>(row[0].data),
                   resultFromStr<double>(row[1].data)));
    }
}

static void do_query_q11_nosubquery(Connect &conn,
                                    const string &name,
                                    double fraction,
                                    vector<q11entry> &results) {
    NamedTimer fcnTimer(__func__);

    // compute threshold
    double threshold = 0.0;
    {
        ostringstream s;
        s << "select sum(ps_supplycost * ps_availqty) * " << fraction << " "
          << "from PARTSUPP, SUPPLIER, NATION "
          << "where ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = '"
          << name << "'";

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

        assert(res.rows.size() == 1);
        threshold = resultFromStr<double>(res.rows[0][0].data);
    }

    ostringstream s;
    s << "select SQL_NO_CACHE ps_partkey, sum(ps_supplycost * ps_availqty) as value "
      << "from PARTSUPP, SUPPLIER, NATION "
      << "where ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = '" << name << "' "
      << "group by ps_partkey having sum(ps_supplycost * ps_availqty) > " << threshold
      //<< "order by value desc"
      ;
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
          q11entry(resultFromStr<uint64_t>(row[0].data),
                   resultFromStr<double>(row[1].data)));
    }
}

static void do_query_q14_opt(Connect &conn,
                             CryptoManager &cm,
                             uint32_t year,
                             vector<q14entry> &results) {
    crypto_manager_stub cm_stub(&cm, UseOldOpe);
    NamedTimer fcnTimer(__func__);

    bool isBin;
    string encDateLower = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(7, 1, year)),
                                   TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDateUpper = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(8, 1, year)),
                                   TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
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

static void do_query_q14_opt2(Connect &conn,
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
                                   TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                                   getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDateUpper = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(8, 1, year)),
                                   TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
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
      << "FROM lineitem_enc_noagg_rowid, part_enc_tmp "
      << "WHERE "
        << "l_partkey_DET = p_partkey_DET AND "
        << "l_shipdate_OPE >= " << encDateLower << " AND "
        << "l_shipdate_OPE < " << encDateUpper;

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
      // l_extendedprice * (1 - l_discount)
      uint64_t l_extendedprice_int = decryptRow<uint64_t, 7>(
          row[1].data,
          12345,
          fieldname(lineitem::l_extendedprice, "DET"),
          TYPE_INTEGER,
          oDET,
          *cm);
      double l_extendedprice = ((double)l_extendedprice_int)/100.0;

      uint64_t l_discount_int = decryptRow<uint64_t, 7>(
          row[2].data,
          12345,
          fieldname(lineitem::l_discount, "DET"),
          TYPE_INTEGER,
          oDET,
          *cm);
      double l_discount = ((double)l_discount_int)/100.0;

      double value = l_extendedprice * (1.0 - l_discount);
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

static void do_query_q14_noopt(Connect &conn,
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
            TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
            getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    string encDateUpper = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(8, 1, year)),
            TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
            getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    ostringstream s;
    s << "SELECT SQL_NO_CACHE p_type_DET, l_extendedprice_DET, l_discount_DET "
        << "FROM " << string(use_opt_table ? "lineitem_enc" : "lineitem_enc_noagg")
          << ", part_enc_tmp "
        << "WHERE l_partkey_DET = p_partkey_DET AND "
        << "l_shipdate_OPE >= " << encDateLower << " AND "
        << "l_shipdate_OPE < " << encDateUpper;
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

static void do_query_q14(Connect &conn,
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

      conn.execute("INSERT INTO part_tmp SELECT p_partkey, p_type FROM PART");

      conn.execute("SET GLOBAL innodb_old_blocks_time = 1000");
    }

    ostringstream s;
    s << "select SQL_NO_CACHE 100.00 * sum(case when p_type like 'PROMO%' then l_extendedprice * (1 - l_discount) else 0 end) / sum(l_extendedprice * (1 - l_discount)) as promo_revenue from LINEITEM, part_tmp where l_partkey = p_partkey and l_shipdate >= date '" << year << "-07-01' and l_shipdate < date '" << year << "-07-01' + interval '1' month";
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
      results.push_back(resultFromStr<double>(row[0].data));
    }
}

static void do_query_q11_opt_proj(Connect &conn,
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

//static void do_query_q11_opt(Connect &conn,
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
//    DBResult * dbres;
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

static void do_query_q18(Connect &conn,
                         uint64_t threshold,
                         vector<q18entry> &results) {
    NamedTimer fcnTimer(__func__);

    ostringstream s0;
    s0 <<
        "select "
        "    l_orderkey "
        "from "
        "    LINEITEM "
        "group by "
        "    l_orderkey having "
        "        sum(l_quantity) > " << threshold;

    vector<string> l_orderkeys;
    {
        DBResult * dbres;
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

    ostringstream s;
    s <<
        "select "
        "    c_name, c_custkey, o_orderkey, "
        "    o_orderdate, o_totalprice, sum(l_quantity) "
        "from "
        "    CUSTOMER, ORDERS, LINEITEM "
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

    results.reserve(res.rows.size());
    for (auto row : res.rows) {
        results.push_back(
            q18entry(
                row[0].data,
                resultFromStr<uint64_t>(row[1].data),
                resultFromStr<uint64_t>(row[2].data),
                0,
                resultFromStr<double>(row[4].data),
                resultFromStr<double>(row[5].data)));
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

static void do_query_q18_crypt(Connect &conn,
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
        "from lineitem_enc_noagg "
        "group by l_orderkey_DET "
        "having count(*) >= " << minGroupCount;
    cerr  << s.str() << endl;

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

    assert(!l_orderkeys.empty());
    //string pkinfo = marshallBinary(cm.getPKInfo());
    s1 <<
        "select "
        "    c_name_DET, c_custkey_DET, o_orderkey_DET, "
        "    o_orderdate_DET, o_totalprice_DET, group_concat(l_quantity_DET) "
        "from "
        "    customer_enc, orders_enc, lineitem_enc_noagg "
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
        DBResult * dbres;
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

static void do_query_q18_crypt_opt2(Connect &conn,
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
        "    customer_enc, orders_enc, lineitem_enc "
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
        DBResult * dbres;
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

static void do_query_q18_crypt_opt(Connect &conn,
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
        "    customer_enc, orders_enc, lineitem_enc "
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
        DBResult * dbres;
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

static void do_query_q20(Connect &conn,
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

    //DBResult * dbres;
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
        s << "select SQL_NO_CACHE p_partkey from PART where p_name like '%" << p_name << "%'";

        DBResult * dbres;
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
        "select SQL_NO_CACHE ps_suppkey, ps_availqty from PARTSUPP, LINEITEM "
        " where "
        "     ps_partkey = l_partkey and "
        "     ps_suppkey = l_suppkey and "
        "     ps_partkey in ( "
        << join(partkeys, ",") <<
        "     ) "
        "     and l_shipdate >= date '" << year << "-01-01'"
        "     and l_shipdate < date '" << year << "-01-01' + interval '1' year"
        " group by ps_partkey, ps_suppkey "
        " having ps_availqty > 0.5 * sum(l_quantity)";

        DBResult * dbres;
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
        "select SQL_NO_CACHE s_name, s_address from SUPPLIER, NATION where s_suppkey in ( "
        << join(suppkeys ,",") <<
        ") and s_nationkey = n_nationkey and n_name = '" << n_name << "' order by s_name";

        DBResult * dbres;
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

static void do_query_q20_opt_noagg(Connect &conn,
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
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);
    string encDATE_END = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year + 1)),
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
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

        DBResult * dbres;
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
        "from partsupp_enc_noopt, lineitem_enc_noagg "
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

    DBResult * dbres;
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

static void do_query_q20_opt(Connect &conn,
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
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
                              getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
    assert(!isBin);
    string encDATE_END = cm_stub.crypt<3>(cm.getmkey(), strFromVal(EncodeDate(1, 1, year + 1)),
                              TYPE_INTEGER, fieldname(lineitem::l_shipdate, "OPE"),
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

        DBResult * dbres;
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
        "from partsupp_enc_noopt, lineitem_enc "
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
  query11,
  query14,
  query18,
  query20,
};

int main(int argc, char **argv) {
    srand(time(NULL));
    if (argc != 6 && argc != 7 && argc != 8 && argc != 9 && argc != 10) {
        usage(argv);
        return 1;
    }

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

    static const char * Query2Strings[] = {
        "--orig-query2",
        "--crypt-query2",
    };
    std::set<string> Query2Modes
      (Query2Strings, Query2Strings + NELEMS(Query2Strings));

    static const char * Query3Strings[] = {
        "--orig-query3",
        "--crypt-query3",
    };
    std::set<string> Query3Modes
      (Query3Strings, Query3Strings + NELEMS(Query3Strings));

    static const char * Query4Strings[] = {
        "--orig-query4",
        "--crypt-query4",
    };
    std::set<string> Query4Modes
      (Query4Strings, Query4Strings + NELEMS(Query4Strings));

    static const char * Query5Strings[] = {
        "--orig-query5",
        "--crypt-query5",
    };
    std::set<string> Query5Modes
      (Query5Strings, Query5Strings + NELEMS(Query5Strings));

    static const char * Query6Strings[] = {
        "--orig-query6",
        "--crypt-query6",
    };
    std::set<string> Query6Modes
      (Query6Strings, Query6Strings + NELEMS(Query6Strings));

    static const char * Query7Strings[] = {
        "--orig-query7",
        "--crypt-query7",
    };
    std::set<string> Query7Modes
      (Query7Strings, Query7Strings + NELEMS(Query7Strings));

    static const char * Query8Strings[] = {
        "--orig-query8",
        "--crypt-query8",
    };
    std::set<string> Query8Modes
      (Query8Strings, Query8Strings + NELEMS(Query8Strings));

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

    if (Query1Modes.find(argv[1]) != Query1Modes.end()) {
        q = query1;
    } else if (Query2Modes.find(argv[1]) != Query2Modes.end()) {
        q = query2;
    } else if (Query3Modes.find(argv[1]) != Query3Modes.end()) {
        q = query3;
    } else if (Query4Modes.find(argv[1]) != Query4Modes.end()) {
        q = query4;
    } else if (Query5Modes.find(argv[1]) != Query5Modes.end()) {
        q = query5;
    } else if (Query6Modes.find(argv[1]) != Query6Modes.end()) {
        q = query6;
    } else if (Query7Modes.find(argv[1]) != Query7Modes.end()) {
        q = query7;
    } else if (Query8Modes.find(argv[1]) != Query8Modes.end()) {
        q = query8;
    } else if (Query11Modes.find(argv[1]) != Query11Modes.end()) {
        q = query11;
    } else if (Query14Modes.find(argv[1]) != Query14Modes.end()) {
        q = query14;
    } else if (Query18Modes.find(argv[1]) != Query18Modes.end()) {
        q = query18;
    } else if (Query20Modes.find(argv[1]) != Query20Modes.end()) {
        q = query20;
    } else {
        usage(argv);
        return 1;
    }


    int input_nruns;
    string db_name;
    string ope_type;
    string do_par;
    string hostname;
    switch (q) {
    case query1:  input_nruns = atoi(argv[3]); db_name = argv[4]; ope_type = argv[5]; do_par = argv[6]; hostname = argv[7]; break;
    case query2:  input_nruns = atoi(argv[5]); db_name = argv[6]; ope_type = argv[7]; do_par = argv[8]; hostname = argv[9]; break;
    case query3:  input_nruns = atoi(argv[4]); db_name = argv[5]; ope_type = argv[6]; do_par = argv[7]; hostname = argv[8]; break;
    case query4:  input_nruns = atoi(argv[3]); db_name = argv[4]; ope_type = argv[5]; do_par = argv[6]; hostname = argv[7]; break;
    case query5:  input_nruns = atoi(argv[4]); db_name = argv[5]; ope_type = argv[6]; do_par = argv[7]; hostname = argv[8]; break;
    case query6:  input_nruns = atoi(argv[5]); db_name = argv[6]; ope_type = argv[7]; do_par = argv[8]; hostname = argv[9]; break;
    case query7:  input_nruns = atoi(argv[4]); db_name = argv[5]; ope_type = argv[6]; do_par = argv[7]; hostname = argv[8]; break;
    case query8:  input_nruns = atoi(argv[5]); db_name = argv[6]; ope_type = argv[7]; do_par = argv[8]; hostname = argv[9]; break;

    case query11: input_nruns = atoi(argv[4]); db_name = argv[5]; ope_type = argv[6]; do_par = argv[7]; hostname = argv[8]; break;
    case query14: input_nruns = atoi(argv[3]); db_name = argv[4]; ope_type = argv[5]; do_par = argv[6]; hostname = argv[7]; break;
    case query18: input_nruns = atoi(argv[3]); db_name = argv[4]; ope_type = argv[5]; do_par = argv[6]; hostname = argv[7]; break;
    case query20: input_nruns = atoi(argv[5]); db_name = argv[6]; ope_type = argv[7]; do_par = argv[8]; hostname = argv[9]; break;
    }
    uint32_t nruns = (uint32_t) input_nruns;

    assert(ope_type == "old_ope" || ope_type == "new_ope");
    UseOldOpe = ope_type == "old_ope";

    assert(do_par == "no_parallel" || do_par == "parallel");
    DoParallel = do_par == "parallel";

    unsigned long ctr = 0;

    Connect conn(hostname, "root", "", db_name, 3307);
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
      default: assert(false);
    }
    cerr << ctr << endl;
    return 0;
}
