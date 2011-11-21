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

#include <unistd.h>
#include <parser/cdb_rewrite.hh>

using namespace std;
using namespace NTL;

template <typename R>
R resultFromStr(const string &r) {
    stringstream ss(r);
    R val;
    ss >> val;
    return val;
}

template <>
string resultFromStr(const string &r) { return r; }

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

static inline string join(const vector<string> &tokens,
                          const string &sep) {
    ostringstream s;
    for (size_t i = 0; i < tokens.size(); i++) {
        s << tokens[i];
        if (i != tokens.size() - 1) s << sep;
    }
    return s.str();
}

typedef enum datatypes {
    DT_INTEGER,
    DT_FLOAT,
    DT_CHAR,
    DT_STRING,
    DT_DATE,
} datatypes;

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

template <typename T>
inline string to_hex(const T& t) {
    ostringstream s;
    s << hex << t;
    return s.str();
}

static const char* const lut = "0123456789ABCDEF";

template <>
inline string to_hex(const string& input) {
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = (unsigned char) input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

template <typename T>
static inline string to_mysql_hex(T t) {
    ostringstream buf;
    buf << "X'" << to_hex(t) << "'";
    return buf.str();
}

static inline long roundToLong(double x) {
    return ((x)>=0?(long)((x)+0.5):(long)((x)-0.5));
}

enum onion_bitmask {
    ONION_DET     = 0x1,
    ONION_DETJOIN = 0x1 << 1,
    ONION_OPE     = 0x1 << 2,
    ONION_AGG     = 0x1 << 3,
};

static const int DET_BITMASK = ONION_DET | ONION_DETJOIN;

static inline bool DoDET(int m) { return m & DET_BITMASK; }
static inline bool DoOPE(int m) { return m & ONION_OPE; }
static inline bool DoAGG(int m) { return m & ONION_AGG; }
static inline bool DoAny(int m) { return DoDET(m) || DoOPE(m) || DoAGG(m); }

static inline bool OnlyOneBit(int m) { return m && !(m & (m-1)); }

static inline string to_mysql_escape_varbin(
        const string &buf, char escape, char fieldTerm, char newlineTerm) {
    ostringstream s;
    for (size_t i = 0; i < buf.size(); i++) {
        char cur = buf[i];
        if (cur == escape || cur == fieldTerm || cur == newlineTerm) {
            s << escape;
        }
        s << cur;
    }
    return s.str();
}

/** assumes slot currently un-occupied */
static void insert_into_slot(ZZ &z, uint64_t value, size_t slot) {
  static const size_t wordbits = sizeof(uint64_t) * 8;
  static const size_t slotbits = wordbits * 2;
  z |= (to_ZZ(value) << (slotbits * slot));
}

static void do_encrypt(size_t i,
                       datatypes dt,
                       int onions,
                       const string &plaintext,
                       vector<string> &enccols,
                       CryptoManager &cm,
                       bool usenull = true) {

    switch (dt) {
    case DT_INTEGER:
        {
            // DET, OPE, AGG, SALT
            bool isBin;
            if (DoDET(onions)) {
                assert(OnlyOneBit(onions & DET_BITMASK));
                SECLEVEL max = (onions & ONION_DET) ? SECLEVEL::DET : SECLEVEL::DETJOIN;
                string encDET = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         fieldname(i, "DET"),
                                         getMin(oDET), max, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encDET)));
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            if (DoOPE(onions)) {
                string encOPE = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         fieldname(i, "OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encOPE)));
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            if (DoAGG(onions)) {
                string encAGG = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         fieldname(i, "AGG"),
                                         getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
                enccols.push_back(to_mysql_escape_varbin(encAGG, '\\', '|', '\n'));
            } else {
                if (usenull) enccols.push_back("NULL");;
            }

            // salt
            if (DoAny(onions)) {
                enccols.push_back(to_s(12345));
            } else {
                if (usenull) enccols.push_back("NULL");
            }
        }
        break;
    case DT_FLOAT:
        {
            // TODO: fix precision
            double f = strtod(plaintext.c_str(), NULL);
            long t = roundToLong(f * 100.0);
            do_encrypt(i, DT_INTEGER, onions, to_s(t), enccols, cm, usenull);
        }
        break;
    case DT_CHAR:
        {
            unsigned int c = plaintext[0];
            do_encrypt(i, DT_INTEGER, onions, to_s(c), enccols, cm, usenull);
        }
        break;
    case DT_STRING:
        {
            // DET, OPE, SALT
            bool isBin;
            if (DoDET(onions)) {
                assert(OnlyOneBit(onions & DET_BITMASK));
                SECLEVEL max = (onions & ONION_DET) ? SECLEVEL::DET : SECLEVEL::DETJOIN;
                string encDET = cm.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                         fieldname(i, "DET"),
                                         getMin(oDET), max, isBin, 12345);
                assert((encDET.size() % 16) == 0);
                enccols.push_back(to_mysql_hex(encDET));
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            if (DoOPE(onions)) {
                string encOPE = cm.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                         fieldname(i, "OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encOPE)));
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            // salt
            if (DoAny(onions)) {
                enccols.push_back(to_s(12345));
            } else {
                if (usenull) enccols.push_back("NULL");
            }
        }
        break;
    case DT_DATE:
        {
            // TODO: don't assume yyyy-mm-dd format
            int year, month, day;
            int ret = sscanf(plaintext.c_str(), "%d-%d-%d", &year, &month, &day);
            assert(ret == 3);

#define __IMPL_FIELD_ENC(field) \
            do { \
                bool isBin; \
                if (DoDET(onions)) { \
                    assert(!(onions & ONION_DETJOIN)); \
                    string encDET = cm.crypt(cm.getmkey(), to_s(field), TYPE_INTEGER, \
                                             fieldname(i, #field "_DET"), \
                                             getMin(oDET), SECLEVEL::DET, isBin, 12345); \
                    enccols.push_back(to_s(valFromStr(encDET))); \
                } else { \
                    if (usenull) enccols.push_back("NULL"); \
                } \
                if (DoOPE(onions)) { \
                    string encOPE = cm.crypt(cm.getmkey(), to_s(field), TYPE_INTEGER, \
                                             fieldname(i, #field "_OPE"), \
                                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345); \
                    enccols.push_back(to_s(valFromStr(encOPE))); \
                } else { \
                    if (usenull) enccols.push_back("NULL"); \
                } \
                if (DoAGG(onions)) { \
                    string encAGG = cm.crypt(cm.getmkey(), to_s(field), TYPE_INTEGER, \
                                             fieldname(i, #field "_AGG"), \
                                             getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345); \
                    enccols.push_back(to_mysql_hex(encAGG)); \
                } else { \
                    if (usenull) enccols.push_back("NULL"); \
                } \
            } while (0)

            // year_DET, year_OPE, year_AGG
            __IMPL_FIELD_ENC(year);
            // month_DET, month_OPE, month_AGG
            __IMPL_FIELD_ENC(month);
            // day_DET, day_OPE, day_AGG
            __IMPL_FIELD_ENC(day);

#undef __IMPL_FIELD_ENC

            // salt
            if (DoAny(onions)) {
                enccols.push_back(to_s(12345));
            } else {
                if (usenull) enccols.push_back("NULL");
            }
        }
        break;
    }
}

class table_encryptor {
protected:
  // allows subclasses to do initialization in c-tor
  table_encryptor() {}
public:
  table_encryptor(const vector<datatypes> &schema,
                  const vector<int>       &onions,
                  bool                     usenull)
    : schema(schema), onions(onions), usenull(usenull) {
    assert(schema.size() == onions.size());
  }

  void encrypt(const vector<string> &tokens,
               vector<string>       &enccols,
               CryptoManager        &cm) {
    assert(tokens.size() >= schema.size());
    for (size_t i = 0; i < schema.size(); i++) {
      do_encrypt(i, schema[i], onions[i], tokens[i], enccols, cm, usenull);
    }
    postprocess(tokens, enccols, cm);
  }

protected:

  virtual
  void postprocess(const vector<string> &tokens,
                   vector<string>       &enccols,
                   CryptoManager        &cm) {}

  vector<datatypes> schema;
  vector<int>       onions;
  bool              usenull;
};

//----------------------------------------------------------------------------
// lineitem

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
};

class lineitem_encryptor : public table_encryptor {
public:
  enum opt_type {
      none,
      normal,
      sort_key,
      compact_table,
      all,
  };

  static vector<datatypes> Schema;
  static vector<int>       NoneOnions;
  static vector<int>       OptOnions;
  static vector<int>       OptSkOnions;
  static vector<int>       OptAllOnions;

  lineitem_encryptor(enum opt_type tpe)
    : table_encryptor() , tpe(tpe) {

    schema = Schema;
    usenull = true;
    switch (tpe) {
      case none:
        onions = NoneOnions;
        break;
      case normal:
        onions = OptOnions;
        break;
      case sort_key:
        onions = OptSkOnions;
        break;
      case compact_table:
        onions = OptSkOnions;
        usenull = false;
        break;
      case all:
        onions = OptAllOnions;
        usenull = false;
        break;
      default: assert(false);
    }
  }

protected:

  void precomputeExprs(const vector<string> &tokens,
                       vector<string>       &enccols,
                       CryptoManager        &cm) {
    // l_disc_price = l_extendedprice * (1 - l_discount)
    double l_extendedprice  = resultFromStr<double>(tokens[lineitem::l_extendedprice]);
    double l_discount       = resultFromStr<double>(tokens[lineitem::l_discount]);
    double l_disc_price     = l_extendedprice * (1.0 - l_discount);
    long   l_disc_price_int = roundToLong(l_disc_price * 100.0);
    do_encrypt(schema.size(), DT_INTEGER, ONION_AGG,
               to_s(l_disc_price_int), enccols, cm, usenull);

    // l_charge = l_extendedprice * (1 - l_discount) * (1 + l_tax)
    double l_tax        = resultFromStr<double>(tokens[lineitem::l_tax]);
    double l_charge     = l_extendedprice * (1.0 - l_discount) * (1.0 + l_tax);
    long   l_charge_int = roundToLong(l_charge * 100.0);
    do_encrypt(schema.size() + 1, DT_INTEGER, ONION_AGG,
               to_s(l_charge_int), enccols, cm, usenull);
  }

  virtual
  void postprocess(const vector<string> &tokens,
                   vector<string>       &enccols,
                   CryptoManager        &cm) {

    switch (tpe) {
      case opt_type::normal:
      case opt_type::sort_key:
      case opt_type::compact_table:
        precomputeExprs(tokens, enccols, cm);
        break;
      case opt_type::all:
        {
          precomputeExprs(tokens, enccols, cm);

          ZZ z;

          // l_quantity_AGG
          long l_quantity_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_quantity]) * 100.0);
          insert_into_slot(z, l_quantity_int, 0);

          // l_extendedprice_AGG
          long l_extendedprice_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_extendedprice]) * 100.0);
          insert_into_slot(z, l_extendedprice_int, 1);

          // l_discount_AGG
          long l_discount_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_discount]) * 100.0);
          insert_into_slot(z, l_discount_int, 2);

          // l_tax_AGG
          long l_tax_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_tax]) * 100.0);
          insert_into_slot(z, l_tax_int, 3);

          // l_disc_price = l_extendedprice * (1 - l_discount)
          double l_extendedprice  = resultFromStr<double>(tokens[lineitem::l_extendedprice]);
          double l_discount       = resultFromStr<double>(tokens[lineitem::l_discount]);
          double l_disc_price     = l_extendedprice * (1.0 - l_discount);
          long   l_disc_price_int = roundToLong(l_disc_price * 100.0);
          insert_into_slot(z, l_disc_price_int, 4);

          // l_charge = l_extendedprice * (1 - l_discount) * (1 + l_tax)
          double l_tax        = resultFromStr<double>(tokens[lineitem::l_tax]);
          double l_charge     = l_extendedprice * (1.0 - l_discount) * (1.0 + l_tax);
          long   l_charge_int = roundToLong(l_charge * 100.0);
          insert_into_slot(z, l_charge_int, 5);

          string enc = cm.encrypt_Paillier(z);
          enccols.push_back(to_mysql_escape_varbin(enc, '\\', '|', '\n'));
          break;
        }
      default: break;
    }

  }

private:
  enum opt_type tpe;

};

vector<datatypes> lineitem_encryptor::Schema = {
  DT_INTEGER,
  DT_INTEGER,
  DT_INTEGER,
  DT_INTEGER,

  DT_FLOAT,
  DT_FLOAT,
  DT_FLOAT,
  DT_FLOAT,

  DT_STRING,
  DT_STRING,

  DT_DATE,
  DT_DATE,
  DT_DATE,

  DT_STRING,
  DT_STRING,
  DT_STRING,
};

vector<int> lineitem_encryptor::NoneOnions = {
  0,
  0,
  0,
  0,

  ONION_DET,
  ONION_DET,
  ONION_DET,
  ONION_DET,

  ONION_DET | ONION_OPE,
  ONION_DET | ONION_OPE,

  ONION_OPE,
  0,
  0,

  0,
  0,
  0,
};

vector<int> lineitem_encryptor::OptOnions = {
  0,
  0,
  0,
  0,

  ONION_AGG,
  ONION_AGG,
  ONION_AGG,
  ONION_AGG,

  ONION_DET | ONION_OPE,
  ONION_DET | ONION_OPE,

  ONION_OPE,
  0,
  0,

  0,
  0,
  0,
};

vector<int> lineitem_encryptor::OptSkOnions =
  lineitem_encryptor::OptOnions;

vector<int> lineitem_encryptor::OptAllOnions = {
  0,
  0,
  0,
  0,

  0,
  0,
  0,
  0,

  ONION_DET | ONION_OPE,
  ONION_DET | ONION_OPE,

  ONION_OPE,
  0,
  0,

  0,
  0,
  0,
};

//----------------------------------------------------------------------------
// partsupp

enum partsupp {
  ps_partkey,
  ps_suppkey,
  ps_availqty,
  ps_supplycost,
  ps_comment,
};

class partsupp_encryptor : public table_encryptor {
public:
  enum opt_type {
      none,
      normal,
  };

  static vector<datatypes> PartSuppSchema;
  static vector<int> PartSuppOnions;

  partsupp_encryptor(enum opt_type tpe)
    : table_encryptor(PartSuppSchema, PartSuppOnions, true), tpe(tpe) {}

protected:

  void precomputeExprs(const vector<string> &tokens,
                       vector<string>       &enccols,
                       CryptoManager        &cm) {
    // ps_value = ps_supplycost * ps_availqty
    double ps_supplycost = resultFromStr<double>(tokens[partsupp::ps_supplycost]);
    double ps_availqty   = resultFromStr<double>(tokens[partsupp::ps_availqty]);
    double ps_value      = ps_supplycost * ps_availqty;
    long   ps_value_int  = roundToLong(ps_value * 100.0);
    do_encrypt(schema.size(), DT_INTEGER, ONION_AGG,
               to_s(ps_value_int), enccols, cm, usenull);
  }

  virtual
  void postprocess(const vector<string> &tokens,
                   vector<string>       &enccols,
                   CryptoManager        &cm) {
    switch (tpe) {
      case opt_type::normal:
        precomputeExprs(tokens, enccols, cm);
        break;
      default: break;
    }
  }

private:
  enum opt_type tpe;

};

vector<datatypes> partsupp_encryptor::PartSuppSchema = {
  DT_INTEGER,
  DT_INTEGER,
  DT_INTEGER,
  DT_FLOAT,
  DT_STRING,
};

vector<int> partsupp_encryptor::PartSuppOnions = {
  ONION_DETJOIN,
  ONION_DETJOIN,
  ONION_DET,
  ONION_DET,
  0,
};

//----------------------------------------------------------------------------
// supplier

enum supplier {
  s_suppkey,
  s_name,
  s_address,
  s_nationkey,
  s_phone,
  s_acctbal,
  s_comment,
};

static const vector<datatypes> SupplierSchema = {
  DT_INTEGER,
  DT_STRING,
  DT_STRING,
  DT_INTEGER,
  DT_STRING,
  DT_FLOAT,
  DT_STRING,
};

static const vector<int> SupplierOnions = {
  ONION_DETJOIN,
  0,
  0,
  ONION_DETJOIN,
  0,
  0,
  0,
};

//----------------------------------------------------------------------------
// nation

enum nation {
  n_nationkey,
  n_name,
  n_regionkey,
  n_comment,
};

static const vector<datatypes> NationSchema = {
  DT_INTEGER,
  DT_STRING,
  DT_INTEGER,
  DT_STRING,
};

static const vector<int> NationOnions = {
  ONION_DETJOIN,
  ONION_DET,
  ONION_DETJOIN,
  0,
};

//----------------------------------------------------------------------------

static map<string, table_encryptor *> EncryptorMap = {
  {"lineitem-none", new lineitem_encryptor(lineitem_encryptor::none)},
  {"lineitem-normal", new lineitem_encryptor(lineitem_encryptor::normal)},
  {"lineitem-sort-key", new lineitem_encryptor(lineitem_encryptor::sort_key)},
  {"lineitem-compact-table", new lineitem_encryptor(lineitem_encryptor::compact_table)},
  {"lineitem-all", new lineitem_encryptor(lineitem_encryptor::all)},

  {"partsupp-none", new partsupp_encryptor(partsupp_encryptor::none)},
  {"partsupp-normal", new partsupp_encryptor(partsupp_encryptor::normal)},

  {"supplier-none", new table_encryptor(SupplierSchema, SupplierOnions, true)},

  {"nation-none", new table_encryptor(NationSchema, NationOnions, true)},
};



static inline string process_input(const string &s, CryptoManager &cm, table_encryptor *tenc) {
  vector<string> tokens;
  tokenize(s, "|", tokens);
  vector<string> columns;
  tenc->encrypt(tokens, columns, cm);
  return join(columns, "|");
}

template <typename A, typename B>
static inline vector<A> KeyExtract(const map<A, B> &m) {
  vector<A> k;
  for (typename map<A, B>::const_iterator it = m.begin();
       it != m.end();
       ++it) {
    k.push_back(it->first);
  }
  return k;
}

static inline vector<string> GetEncryptorOptions() {
  vector<string> keys = KeyExtract(EncryptorMap);
  vector<string> opts;
  for (auto k : keys) {
    opts.push_back(string("--") + k);
  }
  return opts;
}

static void usage(char **argv) {
    cerr << "[USAGE]: " << argv[0] << " ("
         << join(GetEncryptorOptions(), "|")
         <<")" << endl;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv);
        return 1;
    }

    vector<string> opts = GetEncryptorOptions();
    if (find(opts.begin(), opts.end(), argv[1]) == opts.end()) {
        usage(argv);
        return 1;
    }

    string key = string(argv[1]).substr(2);
    table_encryptor *tenc = EncryptorMap[key];
    assert(tenc != NULL);

    CryptoManager cm("12345");
    for (;;) {
        string s;
        getline(cin, s);
        if (!cin.good())
            break;
        try {
            cout << process_input(s, cm, tenc) << endl;
        } catch (...) {
            cerr << "Input line failed:" << endl
                 << "  " << s << endl;
        }
    }
    return 0;
}
