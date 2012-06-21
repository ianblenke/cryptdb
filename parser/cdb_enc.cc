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

#include <crypto/paillier.hh>

#include <util/cryptdb_log.hh>

using namespace std;
using namespace NTL;

typedef enum datatypes {
    DT_INTEGER,
    DT_FLOAT,
    DT_CHAR,
    DT_STRING,
    DT_DATE,
} datatypes;

template <typename T>
static inline string to_mysql_hex(T t) {
    ostringstream buf;
    buf << "X'" << to_hex(t) << "'";
    return buf.str();
}

enum onion_bitmask {
    ONION_DET     = 0x1,
    ONION_DETJOIN = 0x1 << 1,
    ONION_OPE     = 0x1 << 2,
    ONION_OPEJOIN = 0x1 << 3,
    ONION_AGG     = 0x1 << 4,
    ONION_SEARCH  = 0x1 << 5,
};

static const int DET_BITMASK = ONION_DET | ONION_DETJOIN;
static const int OPE_BITMASK = ONION_OPE | ONION_OPEJOIN;

static inline bool DoDET(int m)    { return m & DET_BITMASK;  }
static inline bool DoOPE(int m)    { return m & OPE_BITMASK;  }
static inline bool DoAGG(int m)    { return m & ONION_AGG;    }
static inline bool DoSEARCH(int m) { return m & ONION_SEARCH; }
static inline bool DoAny(int m)    { return DoDET(m) || DoOPE(m) || DoAGG(m) || DoSEARCH(m); }

static inline bool OnlyOneBit(int m) { return m && !(m & (m-1)); }

static inline void push_binary_string(
        vector<string>& enccols, const string& binary, size_t n_slices = 1) {
    // old strategy- it all goes into a single column
    //enccols.push_back(to_mysql_escape_varbin(binary, '\\', '|', '\n'));

    // new strategy- break up the binary into n_slices slots
    assert(n_slices > 0);
    assert((binary.size() % n_slices) == 0); // simplifying assumption
    size_t slice_size = binary.size() / n_slices;
    for (size_t i = 0; i < n_slices; i++) {
        string slice = binary.substr(i * slice_size, slice_size);
        enccols.push_back(to_mysql_escape_varbin(slice));
    }
}

template <size_t SlotSize>
static inline void insert_into_slot(ZZ& z, long value, size_t slot) {
    z |= (to_ZZ(value) << (SlotSize * slot));
}

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

static void do_encrypt(size_t i,
                       datatypes dt,
                       int onions,
                       const string &plaintext,
                       vector<string> &enccols,
                       crypto_manager_stub &cm_stub,
                       bool usenull = true) {

    CryptoManager& cm = *cm_stub.cm;
    switch (dt) {
    case DT_INTEGER:
        {
            // DET, OPE, AGG
            bool isBin = false;
            if (DoDET(onions)) {
                assert(OnlyOneBit(onions & DET_BITMASK));
                SECLEVEL max = (onions & ONION_DET) ? SECLEVEL::DET : SECLEVEL::DETJOIN;
                string encDET = cm_stub.crypt<4>(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         fieldname(i, "DET"),
                                         getMin(oDET), max, isBin, 12345);
                assert(!isBin);
                enccols.push_back(encDET);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoOPE(onions)) {
                assert(OnlyOneBit(onions & OPE_BITMASK));
                string encOPE = cm_stub.crypt<4>(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         (onions & ONION_OPEJOIN) ?
                                          "ope_join" :
                                          fieldname(i, "OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                assert(!isBin);
                enccols.push_back(encOPE);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoAGG(onions)) {
                string encAGG = cm_stub.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         fieldname(i, "AGG"),
                                         getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
                assert(isBin);
                assert(encAGG.size() <= 256);
                push_binary_string(enccols, encAGG);
            } else {
                if (usenull) enccols.push_back("NULL");;
            }
        }
        break;
    case DT_FLOAT:
        {
            // TPC-H uses DECIMAL(15, 2) for all its floats
            // which means the largest number (integer) we would have
            // to store is
            //   x = 999999999999999
            // it requires 50 bits to represent the largest value. rounded
            // to the nearest byte gives us a 7-byte number (which is actually
            // the MySQL storage requirements of DECIMAL(15, 2)). We currently
            // use a bigint (8-byte int) to store this in mysql, so we don't have
            // to deal with binary types. but this means we waste 1 byte per decimal

            // TODO: fix precision
            double f = strtod(plaintext.c_str(), NULL);
            int64_t t = roundToLong(f * 100.0);

            //assert(t >= 0);
            t = t < 0 ? -t : t; // TODO: supplier has an s_acctbal field which
                                // contains neg #s. We should handle this properly
                                // eventually, but for now just drop the neg...
            assert(t < 999999999999999L); // see above

            // DET, OPE, AGG
            bool isBin = false;
            if (DoDET(onions)) {
                assert(OnlyOneBit(onions & DET_BITMASK));
                SECLEVEL max = (onions & ONION_DET) ? SECLEVEL::DET : SECLEVEL::DETJOIN;
                string encDET = cm_stub.crypt<7>(cm.getmkey(), to_s(t), TYPE_INTEGER,
                                         fieldname(i, "DET"),
                                         getMin(oDET), max, isBin, 12345);
                assert(!isBin);
                enccols.push_back(encDET);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoOPE(onions)) {
                assert(OnlyOneBit(onions & OPE_BITMASK));
                string encOPE = cm_stub.crypt<7>(cm.getmkey(), to_s(t), TYPE_INTEGER,
                                         (onions & ONION_OPEJOIN) ?
                                          "ope_join" :
                                          fieldname(i, "OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                assert(isBin);
                assert(encOPE.size() <= (7 * 2));
                // pad, b/c mysql repr will be binary(16)
                encOPE.resize(16);

                // reverse, because ZZ gives bytes in little endian order
                // (and we want the most significant bytes at the beginning
                // of the binary string, so that mysql's comparator does the
                // correct thing)
                encOPE = str_reverse(encOPE);
                push_binary_string(enccols, encOPE);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoAGG(onions)) {
                string encAGG = cm_stub.crypt(cm.getmkey(), to_s(t), TYPE_INTEGER,
                                         fieldname(i, "AGG"),
                                         getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
                assert(isBin);
                assert(encAGG.size() <= 256);
                push_binary_string(enccols, encAGG);
            } else {
                if (usenull) enccols.push_back("NULL");;
            }

        }
        break;
    case DT_CHAR:
        {
            // we map char to a mysql unsigned tinyint
            unsigned int c = plaintext[0];

            string pt = to_s(c);

            bool isBin = false;
            if (DoDET(onions)) {
                assert(OnlyOneBit(onions & DET_BITMASK));
                SECLEVEL max = (onions & ONION_DET) ? SECLEVEL::DET : SECLEVEL::DETJOIN;
                string encDET = cm_stub.crypt<1>(cm.getmkey(), pt, TYPE_INTEGER,
                                         fieldname(i, "DET"),
                                         getMin(oDET), max, isBin, 12345);
                assert(!isBin);
                enccols.push_back(encDET);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoOPE(onions)) {
                assert(OnlyOneBit(onions & OPE_BITMASK));
                string encOPE = cm_stub.crypt<1>(cm.getmkey(), pt, TYPE_INTEGER,
                                         (onions & ONION_OPEJOIN) ?
                                          "ope_join" :
                                          fieldname(i, "OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                assert(!isBin);
                enccols.push_back(encOPE);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoAGG(onions)) {
                string encAGG = cm_stub.crypt(cm.getmkey(), pt, TYPE_INTEGER,
                                         fieldname(i, "AGG"),
                                         getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
                assert(isBin);
                push_binary_string(enccols, encAGG);
            } else {
                if (usenull) enccols.push_back("NULL");;
            }

        }
        break;
    case DT_STRING:
        {
            // DET, OPE, SWP
            bool isBin = false;
            if (DoDET(onions)) {
                assert(OnlyOneBit(onions & DET_BITMASK));
                SECLEVEL max = (onions & ONION_DET) ? SECLEVEL::DET : SECLEVEL::DETJOIN;
                string encDET = cm_stub.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                         fieldname(i, "DET"),
                                         getMin(oDET), max, isBin, 12345);
                //assert((encDET.size() % 16) == 0);
                assert(encDET.size() == plaintext.size()); // length preserving
                assert(isBin);
                push_binary_string(enccols, encDET);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoOPE(onions)) {
                assert(OnlyOneBit(onions & OPE_BITMASK));
                string encOPE = cm_stub.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                         (onions & ONION_OPEJOIN) ?
                                          "ope_join" :
                                          fieldname(i, "OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                // NOT BINARY
                assert(!isBin);
                enccols.push_back(encOPE);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoSEARCH(onions)) {
                string encSWP = cm_stub.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                         fieldname(i, "SWP"),
                                         getMin(oSWP), SECLEVEL::SWP, isBin, 12345);
                assert(isBin);
                push_binary_string(enccols, encSWP);
            } else {
                if (usenull) enccols.push_back("NULL");
            }
        }
        break;
    case DT_DATE:
        {
            // TODO: don't assume yyyy-mm-dd format
            int encoding = encode_yyyy_mm_dd(plaintext);

            bool isBin = false;
            if (DoDET(onions)) {
                assert(!(onions & ONION_DETJOIN));
                string encDET = cm_stub.crypt<3>(cm.getmkey(), to_s(encoding), TYPE_INTEGER,
                        fieldname(i, "DET"),
                        getMin(oDET), SECLEVEL::DET, isBin, 12345);
                assert(!isBin);
                enccols.push_back(encDET);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            isBin = false;
            if (DoOPE(onions)) {
                assert(OnlyOneBit(onions & OPE_BITMASK));
                string encOPE = cm_stub.crypt<3>(cm.getmkey(), to_s(encoding), TYPE_INTEGER,
                       (onions & ONION_OPEJOIN) ?
                        "ope_join" :
                        fieldname(i, "OPE"),
                        getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                assert(!isBin);
                enccols.push_back(encOPE);
            } else {
                if (usenull) enccols.push_back("NULL");
            }

            // agg over dates makes no sense for now...
            assert(!DoAGG(onions));

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
                  bool                     usenull,
                  bool                     processrow)
    : schema(schema), onions(onions),
      usenull(usenull), processrow(processrow) {
    assert(schema.size() == onions.size());
  }

  virtual ~table_encryptor() {}

  inline bool shouldProcessRow() const { return processrow; }

  virtual
  void encryptRow(const vector<string> &tokens,
                  vector<string>       &enccols,
                  crypto_manager_stub        &cm) {
    assert(tokens.size() >= schema.size());
    preprocessRow(tokens, enccols, cm);
    for (size_t i = 0; i < schema.size(); i++) {
      do_encrypt(i, schema[i], onions[i], tokens[i], enccols, cm, usenull);
    }
    postprocessRow(tokens, enccols, cm);
    //writeSalt(tokens, enccols, cm);
  }

  virtual
  void encryptBatch(const vector<vector<string> > &tokens,
                    vector<vector<string> >       &enccols,
                    crypto_manager_stub &cm) {
    for (size_t i = 0; i < tokens.size(); i++) {
      const vector<string> &v = tokens[i];
      vector<string> e;
      encryptRow(v, e, cm);
      enccols.push_back(e);
    }
    postprocessBatch(tokens, enccols, cm);
  }

  virtual void writeSalt(const vector<string> &tokens,
                         vector<string>       &enccols,
                         crypto_manager_stub        &cm) {
    enccols.push_back("12345");
  }

protected:

  virtual
  void preprocessRow(const vector<string> &tokens,
                     vector<string>       &enccols,
                     crypto_manager_stub        &cm) {}

  virtual
  void postprocessRow(const vector<string> &tokens,
                      vector<string>       &enccols,
                      crypto_manager_stub        &cm) {}

  virtual
  void postprocessBatch(const vector<vector<string> > &tokens,
                        vector<vector<string> >       &enccols,
                        crypto_manager_stub &cm) {}

  vector<datatypes> schema;
  vector<int>       onions;
  bool              usenull;
  bool              processrow;
};

//----------------------------------------------------------------------------
// customer

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

static const vector<datatypes> CustSchema = {
    DT_INTEGER,
    DT_STRING,
    DT_STRING,
    DT_INTEGER,
    DT_STRING,
    DT_FLOAT,
    DT_STRING,
    DT_STRING,
};

static const vector<int> CustOnions = {
    ONION_DETJOIN,
    ONION_DET,
    ONION_DET,
    ONION_DETJOIN,
    ONION_DET,
    ONION_DET | ONION_OPE,
    ONION_DET,
    ONION_DET,
};

class customer_encryptor : public table_encryptor {
public:
  enum opt_type {
    normal,
    row_packed_acctbal,

    normal_vldb_orig,
    normal_vldb_greedy,
  };

  customer_encryptor(enum opt_type tpe)
    : tpe(tpe) {
    schema = CustSchema;
    onions = CustOnions;
    usenull = false;
    processrow = (tpe == normal ||
                  tpe == normal_vldb_orig ||
                  tpe == normal_vldb_greedy);
  }

  virtual
  void postprocessRow(const vector<string> &tokens,
                      vector<string>       &enccols,
                      crypto_manager_stub        &cm) {
    assert(tpe == normal ||
           tpe == normal_vldb_orig ||
           tpe == normal_vldb_greedy);

    switch (tpe) {
      case normal: {
        string phone_prefix = tokens[customer::c_phone].substr(0, 2);
        do_encrypt(customer::c_phone, DT_STRING, ONION_DET,
                   phone_prefix, enccols, cm, false);
        break;
      }

      case normal_vldb_orig: {
        long c_acctbal_int =
          roundToLong(fabs(resultFromStr<double>(tokens[customer::c_acctbal])) * 100.0);
        push_binary_string(enccols, cm.cm->encrypt_u64_paillier(c_acctbal_int));
        break;
      }

      case normal_vldb_greedy: {
        string phone_prefix = tokens[customer::c_phone].substr(0, 2);
        do_encrypt(customer::c_phone, DT_STRING, ONION_DET | ONION_OPE,
                   phone_prefix, enccols, cm, false);
        break;
      }

      default: assert(false);
    }
  }

protected:
  virtual
  void encryptBatch(const vector<vector<string> > &tokens,
                    vector<vector<string> >       &enccols,
                    crypto_manager_stub &cm) {
    assert(tpe != normal);
    do_row_pack(tokens, enccols, cm);
  }

private:
  static const size_t BitsPerAggField = 83;
  static const size_t FieldsPerAgg = 1024 / BitsPerAggField;

  void do_row_pack(const vector<vector<string> > &rows,
                   vector<vector<string> > &enccols,
                   crypto_manager_stub &cm) {
    assert(tpe != normal);

    _static_assert(FieldsPerAgg == 12); // this is hardcoded various places
    size_t nAggs = rows.size() / FieldsPerAgg +
        (rows.size() % FieldsPerAgg ? 1 : 0);
    for (size_t i = 0; i < nAggs; i++) {
      size_t base = i * FieldsPerAgg;
      ZZ z0 = to_ZZ(0);
      for (size_t j = 0; j < min(FieldsPerAgg, rows.size() - base); j++) {
        size_t row_id = base + j;
        const vector<string>& tokens = rows[row_id];
        long c_acctbal_int =
          roundToLong(fabs(resultFromStr<double>(tokens[customer::c_acctbal])) * 100.0);
        insert_into_slot<BitsPerAggField>(z0, c_acctbal_int, j);
      }
      string e0 = cm.encrypt_Paillier(z0);
      e0.resize(256);
      cout << e0;
    }
  }

  enum opt_type tpe;
};

//----------------------------------------------------------------------------
// orders

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

static const vector<datatypes> OrdersSchema = {
    DT_INTEGER,
    DT_INTEGER,
    DT_CHAR,
    DT_FLOAT,
    DT_DATE,
    DT_STRING,
    DT_STRING,
    DT_INTEGER,
    DT_STRING,
};

static const vector<int> OrdersOnions = {
    ONION_DETJOIN,
    ONION_DETJOIN,
    ONION_DET,
    ONION_DET | ONION_OPE,
    ONION_DET | ONION_OPE,
    ONION_DET,
    ONION_DET,
    ONION_DET,
    ONION_DET,
};

static const vector<int> OrdersVldbCdbGreedyOnions = {
    ONION_DETJOIN,
    ONION_DETJOIN,
    ONION_DET,
    ONION_DET | ONION_OPE,
    ONION_DET | ONION_OPE,
    ONION_DET | ONION_OPE,
    ONION_DET,
    ONION_DET,
    ONION_DET,
};

class orders_encryptor : public table_encryptor {
public:
  enum opt_type {
    normal,

    normal_vldb_greedy,

  };

  orders_encryptor(enum opt_type tpe) : tpe(tpe) {

    switch (tpe) {
      case normal:
        schema = OrdersSchema;
        onions = OrdersOnions;
        usenull = false;
        processrow = true;
        break;

      case normal_vldb_greedy:
        schema = OrdersSchema;
        onions = OrdersVldbCdbGreedyOnions;
        usenull = false;
        processrow = true;
        break;

      default: assert(false);
    }
  }

private:
  opt_type tpe;

protected:
  virtual
  void postprocessRow(const vector<string> &tokens,
                      vector<string>       &enccols,
                      crypto_manager_stub        &cm) {
    // write o_orderdate_YEAR_DET

    const string& orderdate = tokens[orders::o_orderdate];
    int year, month, day;
    int ret = sscanf(orderdate.c_str(), "%d-%d-%d", &year, &month, &day);
    assert(ret == 3);
    assert(1 <= day && day <= 31);
    assert(1 <= month && month <= 12);
    assert(year >= 0);

    bool isBin = false;
    string encDET = cm.crypt<2>(cm.cm->getmkey(), to_s(year), TYPE_INTEGER,
                             fieldname(0, "DET"),
                             getMin(oDET), SECLEVEL::DET, isBin, 12345);
    assert(!isBin);
    enccols.push_back(encDET);

    if (tpe == normal_vldb_greedy) {
      bool isBin = false;
      string encOPE = cm.crypt<2>(cm.cm->getmkey(), to_s(year), TYPE_INTEGER,
                               fieldname(0, "OPE"),
                               getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
      assert(!isBin);
      enccols.push_back(encOPE);
    }
  }
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
      normal_agg,
      packed,

      row_packed_disc_price,
      row_packed_revenue,
      row_packed_quantity,

      row_col_packed,

      // the following below are for the VLDB experiments
      normal_vldb_orig, // what we use for the "state-of-the-art" before us
      normal_vldb_greedy, // cryptdb-greedy
  };

  static vector<datatypes> Schema;
  static vector<int>       Onions;
  static vector<int>       PackedOnions;

  static vector<int> VldbCdbGreedyOnions;

  lineitem_encryptor(enum opt_type tpe)
    : tpe(tpe) {

    schema = Schema;
    switch (tpe) {
      case none:
        onions = Onions;
        usenull = true;
        processrow = true;
        break;
      case normal:
      case normal_agg:
      case normal_vldb_orig:
        onions = Onions;
        usenull = false;
        processrow = true;
        break;

      case normal_vldb_greedy:
        onions = VldbCdbGreedyOnions;
        usenull = false;
        processrow = true;
        break;

      case packed:
      case row_packed_disc_price:
      case row_packed_revenue:
      case row_packed_quantity:
      case row_col_packed:
        onions = PackedOnions;
        usenull = false;
        processrow = false;
        break;
      default:
        assert(false);
        break;
    }
  }

protected:

  /*
  void precomputeExprs(const vector<string> &tokens,
                       vector<string>       &enccols,
                       crypto_manager_stub        &cm) {
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
  */

  void do_group_pack(const vector<vector<string> > &tokens,
                     vector<vector<string> >       &enccols,
                     crypto_manager_stub &cm) {
    // sort into (l_returnflag, l_linestatus)
    typedef pair<string, string> Key;
    typedef vector<string> Row;
    typedef map< Key, vector<Row> > group_map;
    group_map groups;

    for (vector<Row>::const_iterator it = tokens.begin();
         it != tokens.end(); ++it) {

      const Row &r = *it;
      Key k(r[lineitem::l_returnflag], r[lineitem::l_linestatus]);

      group_map::iterator i = groups.find(k);
      if (i == groups.end()) {
        vector<Row> init;
        init.push_back(r);
        groups[k] = init;
      } else {
        vector<Row> &rows = i->second;
        rows.push_back(r);
      }
    }

    struct pack_functor_s {
      inline bool operator()(const Row &lhs, const Row &rhs) const {
        assert(lhs.size() > lineitem::l_comment);
        assert(rhs.size() > lineitem::l_comment);

        const string &lhs_shipdate_s = lhs[lineitem::l_shipdate];
        const string &rhs_shipdate_s = rhs[lineitem::l_shipdate];

        int lhs_encoding = encode_yyyy_mm_dd(lhs_shipdate_s);
        int rhs_encoding = encode_yyyy_mm_dd(rhs_shipdate_s);

        uint32_t lhs_orderkey = resultFromStr<uint32_t>(lhs[lineitem::l_orderkey]);
        uint32_t rhs_orderkey = resultFromStr<uint32_t>(rhs[lineitem::l_orderkey]);

        uint32_t lhs_linenumber = resultFromStr<uint32_t>(lhs[lineitem::l_linenumber]);
        uint32_t rhs_linenumber = resultFromStr<uint32_t>(rhs[lineitem::l_linenumber]);

        return lhs_encoding < rhs_encoding || (
            lhs_encoding == rhs_encoding && (
              lhs_orderkey < rhs_orderkey || (
                lhs_orderkey == rhs_orderkey &&
                  lhs_linenumber < rhs_linenumber
              )
            )
          );
      }
    } pack_functor;

    // sort each group by l_shipdate (asc). break ties by (l_orderkey, l_linenumber)
    for (group_map::iterator it = groups.begin();
         it != groups.end(); ++it) {
      vector<Row> &rows = it->second;
      sort(rows.begin(), rows.end(), pack_functor);
    }

    // for each group, we write out a variable number of blocks.
    for (group_map::iterator it = groups.begin();
         it != groups.end(); ++it) {
      const vector<Row> &rows = it->second;
      typedef PallierSlotManager<uint32_t, 2> PSM;

      vector<string> e_group_key;
      do_encrypt(lineitem::l_returnflag, DT_CHAR, ONION_DET,
                 it->first.first, e_group_key, cm, false);
      do_encrypt(lineitem::l_linestatus, DT_CHAR, ONION_DET,
                 it->first.second, e_group_key, cm, false);

      size_t nbatches = rows.size() / PSM::NumSlots +
                        ((rows.size() % PSM::NumSlots) ? 1 : 0);

      for (size_t i = 0; i < nbatches; i++) {
        ZZ z1 = to_ZZ(0);
        ZZ z2 = to_ZZ(0);
        ZZ z3 = to_ZZ(0);
        ZZ z4 = to_ZZ(0);
        ZZ z5 = to_ZZ(0);
        vector<Key> keys;

        size_t remaining = rows.size() - i * PSM::NumSlots;
        assert(remaining > 0);
        size_t limit = min(remaining, PSM::NumSlots);

        int mindate = 0, maxdate = 0;
        for (size_t j = 0; j < limit; j++) {
          size_t idx = i * PSM::NumSlots + j;
          assert(idx < rows.size());
          const Row &row = rows[idx];

          if (j == 0) {
            mindate = encode_yyyy_mm_dd(row[lineitem::l_shipdate]);
          } else if (j == limit - 1) {
            maxdate = encode_yyyy_mm_dd(row[lineitem::l_shipdate]);
          }

          // l_quantity_AGG
          long l_quantity_int = roundToLong(resultFromStr<double>(row[lineitem::l_quantity]) * 100.0);
          PSM::insert_into_slot(z1, l_quantity_int, j);

          // l_extendedprice_AGG
          long l_extendedprice_int = roundToLong(resultFromStr<double>(row[lineitem::l_extendedprice]) * 100.0);
          PSM::insert_into_slot(z2, l_extendedprice_int, j);

          // l_discount_AGG
          long l_discount_int = roundToLong(resultFromStr<double>(row[lineitem::l_discount]) * 100.0);
          PSM::insert_into_slot(z3, l_discount_int, j);

          // l_tax_AGG
          //long l_tax_int = roundToLong(resultFromStr<double>(row[lineitem::l_tax]) * 100.0);
          //PSM::insert_into_slot(z, l_tax_int, 3);

          // l_disc_price = l_extendedprice * (1 - l_discount)
          double l_extendedprice  = resultFromStr<double>(row[lineitem::l_extendedprice]);
          double l_discount       = resultFromStr<double>(row[lineitem::l_discount]);
          double l_disc_price     = l_extendedprice * (1.0 - l_discount);
          long   l_disc_price_int = roundToLong(l_disc_price * 100.0);
          PSM::insert_into_slot(z4, l_disc_price_int, j);

          // l_charge = l_extendedprice * (1 - l_discount) * (1 + l_tax)
          double l_tax        = resultFromStr<double>(row[lineitem::l_tax]);
          double l_charge     = l_extendedprice * (1.0 - l_discount) * (1.0 + l_tax);
          long   l_charge_int = roundToLong(l_charge * 100.0);
          PSM::insert_into_slot(z5, l_charge_int, j);

          Key k(row[lineitem::l_orderkey], row[lineitem::l_linenumber]);
          keys.push_back(k);
        }

        // (l_returnflag, l_linestatus), start_shipdate, end_shipdate,
        //    k1, k2, ..., k16, packed_count,
        //    z1, z2, z3, z4, z5

        Row enc_row(e_group_key.begin(), e_group_key.end());
        do_encrypt(lineitem::l_shipdate, DT_INTEGER, ONION_OPE,
                   to_s(mindate), enc_row, cm, false);
        do_encrypt(lineitem::l_shipdate, DT_INTEGER, ONION_OPE,
                   to_s(maxdate), enc_row, cm, false);

        for (vector<Key>::const_iterator it = keys.begin();
             it != keys.end(); ++it) {
          do_encrypt(lineitem::l_orderkey, DT_INTEGER, ONION_DETJOIN,
                     it->first, enc_row, cm, false);
          do_encrypt(lineitem::l_linenumber, DT_INTEGER, ONION_DETJOIN,
                     it->second, enc_row, cm, false);
        }

        for (size_t e_i = 0; e_i < PSM::NumSlots - keys.size(); e_i++) {
          enc_row.push_back("NULL");
          enc_row.push_back("NULL");
        }

        enc_row.push_back(to_s(keys.size()));

        ZZ * zs[] = {&z1, &z2, &z3, &z4, &z5};
        for (size_t zs_i = 0; zs_i < NELEMS(zs); zs_i++) {
          string enc = cm.encrypt_Paillier(*zs[zs_i]);
          push_binary_string(enc_row, enc);
        }

        enccols.push_back(enc_row);
      }
    }
  }

  static const size_t BitsPerAggField = 83;
  static const size_t FieldsPerAgg = 1024 / BitsPerAggField;

  static const size_t RowColPackPlainSize = 1256;
  static const size_t RowColPackCipherSize = RowColPackPlainSize * 2;

  void do_row_col_pack(const vector<vector<string> > &rows,
                       vector<vector<string> >       &enccols,
                       crypto_manager_stub &cm) {
      // ASSUMES ROWS SORTED BY ENCRYPTED PRIMARY KEY

      _static_assert( RowColPackCipherSize % 8 == 0 );
      _static_assert( BitsPerAggField * 5 * 3 <= RowColPackPlainSize );

      auto sk = Paillier_priv::keygen(RowColPackCipherSize / 2,
                                      RowColPackCipherSize / 8);
      Paillier_priv pp(sk);

      auto pk = pp.pubkey();
      Paillier p(pk);

      size_t nAggs = rows.size() / 3 +
          (rows.size() % 3 ? 1 : 0);
      for (size_t i = 0; i < nAggs; i++) {
        size_t base = i * 3;
        ZZ z = to_ZZ(0);
        for (size_t j = 0; j < min(3, rows.size() - base); j++) {
          size_t row_id = base + j;

          const vector<string>& tokens = rows[row_id];

          // l_quantity_AGG
          long l_quantity_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_quantity]) * 100.0);
          insert_into_slot<BitsPerAggField>(z, l_quantity_int, 5 * j);

          // l_extendedprice_AGG
          long l_extendedprice_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_extendedprice]) * 100.0);
          insert_into_slot<BitsPerAggField>(z, l_extendedprice_int, 5 * j + 1);

          // l_discount_AGG
          long l_discount_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_discount]) * 100.0);
          insert_into_slot<BitsPerAggField>(z, l_discount_int, 5 * j + 2);

          // l_disc_price = l_extendedprice * (1 - l_discount)
          double l_extendedprice  = resultFromStr<double>(tokens[lineitem::l_extendedprice]);
          double l_discount       = resultFromStr<double>(tokens[lineitem::l_discount]);
          double l_disc_price     = l_extendedprice * (1.0 - l_discount);
          long   l_disc_price_int = roundToLong(l_disc_price * 100.0);
          insert_into_slot<BitsPerAggField>(z, l_disc_price_int, 5 * j + 3);

          // l_charge = l_extendedprice * (1 - l_discount) * (1 + l_tax)
          double l_tax        = resultFromStr<double>(tokens[lineitem::l_tax]);
          double l_charge     = l_extendedprice * (1.0 - l_discount) * (1.0 + l_tax);
          long   l_charge_int = roundToLong(l_charge * 100.0);
          insert_into_slot<BitsPerAggField>(z, l_charge_int, 5 * j + 4);
        }

        ZZ ct = p.encrypt(z);
        string e = StringFromZZ(ct);
        assert(e.size() <= (RowColPackCipherSize / 8));
        e.resize(RowColPackCipherSize / 8);
        cout << e;
      }
  }

  void do_row_pack(const vector<vector<string> > &rows,
                   vector<vector<string> > &enccols,
                   uint32_t fields_mask,
                   crypto_manager_stub &cm) {
      // ASSUMES ROWS SORTED BY ENCRYPTED PRIMARY KEY

      assert(OnlyOneBit(fields_mask)); // only handles 1 bit for now

      bool do_l_quantity = fields_mask & 0x1;
      bool do_l_extendedprice = fields_mask & (0x1 << 1);
      bool do_l_discount = fields_mask & (0x1 << 2);
      bool do_l_disc_price = fields_mask & (0x1 << 3);
      bool do_l_charge = fields_mask & (0x1 << 4);
      bool do_l_revenue = fields_mask & (0x1 << 5);

      _static_assert(FieldsPerAgg == 12); // this is hardcoded various places
      size_t nAggs = rows.size() / FieldsPerAgg +
          (rows.size() % FieldsPerAgg ? 1 : 0);
      for (size_t i = 0; i < nAggs; i++) {
          size_t base = i * FieldsPerAgg;
          ZZ z0, z1, z2, z3, z4, z5;
          z0 = to_ZZ(0); z1 = to_ZZ(0); z2 = to_ZZ(0);
          z3 = to_ZZ(0); z4 = to_ZZ(0); z5 = to_ZZ(0);
          for (size_t j = 0; j < min(FieldsPerAgg, rows.size() - base); j++) {
              size_t row_id = base + j;
              const vector<string>& tokens = rows[row_id];

              // l_quantity_AGG
              if (do_l_quantity) {
                long l_quantity_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_quantity]) * 100.0);
                insert_into_slot<BitsPerAggField>(z0, l_quantity_int, j);
              }

              // l_extendedprice_AGG
              if (do_l_extendedprice) {
                long l_extendedprice_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_extendedprice]) * 100.0);
                insert_into_slot<BitsPerAggField>(z1, l_extendedprice_int, j);
              }

              // l_discount_AGG
              if (do_l_discount) {
                long l_discount_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_discount]) * 100.0);
                insert_into_slot<BitsPerAggField>(z2, l_discount_int, j);
              }

              // l_disc_price = l_extendedprice * (1 - l_discount)
              if (do_l_disc_price) {
                double l_extendedprice  = resultFromStr<double>(tokens[lineitem::l_extendedprice]);
                double l_discount       = resultFromStr<double>(tokens[lineitem::l_discount]);
                double l_disc_price     = l_extendedprice * (1.0 - l_discount);
                long   l_disc_price_int = roundToLong(l_disc_price * 100.0);
                insert_into_slot<BitsPerAggField>(z3, l_disc_price_int, j);
              }

              // l_charge = l_extendedprice * (1 - l_discount) * (1 + l_tax)
              if (do_l_charge) {
                double l_tax        = resultFromStr<double>(tokens[lineitem::l_tax]);
                double l_charge     = l_extendedprice * (1.0 - l_discount) * (1.0 + l_tax);
                long   l_charge_int = roundToLong(l_charge * 100.0);
                insert_into_slot<BitsPerAggField>(z4, l_charge_int, j);
              }

              // l_revenue = l_extendedprice * l_discount
              if (do_l_revenue) {
                double l_extendedprice = resultFromStr<double>(tokens[lineitem::l_extendedprice]);
                double l_discount    = resultFromStr<double>(tokens[lineitem::l_discount]);
                double l_revenue     = l_extendedprice * l_discount;
                long   l_revenue_int = roundToLong(l_revenue * 100.0);
                insert_into_slot<BitsPerAggField>(z5, l_revenue_int, j);
              }
          }

          // write the block out
          string e0;
          if (do_l_quantity) {
            e0 = cm.encrypt_Paillier(z0);
            e0.resize(256);
          }

          string e1;
          if (do_l_extendedprice) {
            e1 = cm.encrypt_Paillier(z1);
            e1.resize(256);
          }

          string e2;
          if (do_l_discount) {
            e2 = cm.encrypt_Paillier(z2);
            e2.resize(256);
          }

          string e3;
          if (do_l_disc_price) {
            e3 = cm.encrypt_Paillier(z3);
            e3.resize(256);
          }

          string e4;
          if (do_l_charge) {
            e4 = cm.encrypt_Paillier(z4);
            e4.resize(256);
          }

          string e5;
          if (do_l_revenue) {
            e5 = cm.encrypt_Paillier(z5);
            e5.resize(256);
          }

          cout << e0 << e1 << e2 << e3 << e4 << e5;
      }
  }

  virtual
  void encryptBatch(const vector<vector<string> > &tokens,
                    vector<vector<string> >       &enccols,
                    crypto_manager_stub &cm) {
    assert(tpe == opt_type::packed ||
           tpe == opt_type::row_packed_disc_price ||
           tpe == opt_type::row_packed_revenue ||
           tpe == opt_type::row_packed_quantity ||
           tpe == opt_type::row_col_packed);
    switch (tpe) {
    case opt_type::packed:                do_group_pack  (tokens, enccols, cm); break;
    case opt_type::row_packed_disc_price: do_row_pack    (tokens, enccols, (0x1 << 3), cm); break;
    case opt_type::row_packed_revenue:    do_row_pack    (tokens, enccols, (0x1 << 5), cm); break;
    case opt_type::row_packed_quantity:   do_row_pack    (tokens, enccols, (0x1), cm); break;
    case opt_type::row_col_packed:        do_row_col_pack(tokens, enccols, cm); break;
    default: assert(false);
    }
  }

  virtual
  void postprocessRow(const vector<string> &tokens,
                      vector<string>       &enccols,
                      crypto_manager_stub        &cm) {

    switch (tpe) {
      case opt_type::normal:
        {
          // l_shipdate's year
          const string& l_shipdate = tokens[lineitem::l_shipdate];
          int year, month, day;
          int ret = sscanf(l_shipdate.c_str(), "%d-%d-%d", &year, &month, &day);
          assert(ret == 3);
          assert(year >= 0);
          bool isBin = false;
          string enc = cm.crypt<2>(
              cm.cm->getmkey(), to_s(year), TYPE_INTEGER,
              fieldname(0, "DET"), getMin(oDET), SECLEVEL::DET, isBin, 12345);
          assert(!isBin);
          enccols.push_back(enc);

          // l_disc_price = l_extendedprice * (1 - l_discount)
          double l_extendedprice  = resultFromStr<double>(tokens[lineitem::l_extendedprice]);
          double l_discount       = resultFromStr<double>(tokens[lineitem::l_discount]);
          double l_disc_price     = l_extendedprice * (1.0 - l_discount);
          do_encrypt(0, DT_FLOAT, ONION_DET, to_s(l_disc_price),
                     enccols, cm, false);
        }
        break;

      case opt_type::normal_vldb_greedy:
        {
          // l_shipdate's year
          const string& l_shipdate = tokens[lineitem::l_shipdate];
          int year, month, day;
          int ret = sscanf(l_shipdate.c_str(), "%d-%d-%d", &year, &month, &day);
          assert(ret == 3);
          assert(year >= 0);

          {
            bool isBin = false;
            string enc = cm.crypt<2>(
                cm.cm->getmkey(), to_s(year), TYPE_INTEGER,
                fieldname(0, "DET"), getMin(oDET), SECLEVEL::DET, isBin, 12345);
            assert(!isBin);
            enccols.push_back(enc);
          }

          {
            bool isBin = false;
            string enc = cm.crypt<2>(
                cm.cm->getmkey(), to_s(year), TYPE_INTEGER,
                fieldname(0, "OPE"), getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
            assert(!isBin);
            enccols.push_back(enc);
          }

          // l_disc_price = l_extendedprice * (1 - l_discount)
          double l_extendedprice  = resultFromStr<double>(tokens[lineitem::l_extendedprice]);
          double l_discount       = resultFromStr<double>(tokens[lineitem::l_discount]);
          double l_disc_price     = l_extendedprice * (1.0 - l_discount);
          do_encrypt(0, DT_FLOAT, ONION_DET, to_s(l_disc_price),
                     enccols, cm, false);
        }

        break;
      case opt_type::normal_agg:
        {
          ZZ z;
          typedef PallierSlotManager<uint64_t, 2> PSM;

          // l_quantity_AGG
          long l_quantity_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_quantity]) * 100.0);
          PSM::insert_into_slot(z, l_quantity_int, 0);

          // l_extendedprice_AGG
          long l_extendedprice_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_extendedprice]) * 100.0);
          PSM::insert_into_slot(z, l_extendedprice_int, 1);

          // l_discount_AGG
          long l_discount_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_discount]) * 100.0);
          PSM::insert_into_slot(z, l_discount_int, 2);

          // l_tax_AGG
          long l_tax_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_tax]) * 100.0);
          PSM::insert_into_slot(z, l_tax_int, 3);

          // l_disc_price = l_extendedprice * (1 - l_discount)
          double l_extendedprice  = resultFromStr<double>(tokens[lineitem::l_extendedprice]);
          double l_discount       = resultFromStr<double>(tokens[lineitem::l_discount]);
          double l_disc_price     = l_extendedprice * (1.0 - l_discount);
          long   l_disc_price_int = roundToLong(l_disc_price * 100.0);
          PSM::insert_into_slot(z, l_disc_price_int, 4);

          // l_charge = l_extendedprice * (1 - l_discount) * (1 + l_tax)
          double l_tax        = resultFromStr<double>(tokens[lineitem::l_tax]);
          double l_charge     = l_extendedprice * (1.0 - l_discount) * (1.0 + l_tax);
          long   l_charge_int = roundToLong(l_charge * 100.0);
          PSM::insert_into_slot(z, l_charge_int, 5);

          string enc = cm.encrypt_Paillier(z);
          assert(enc.size() <= 256);
          push_binary_string(enccols, enc);
          break;
        }

      case opt_type::normal_vldb_orig:
        {
          // l_quantity agg
          long l_quantity_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_quantity]) * 100.0);

          // l_extendedprice agg
          long l_extendedprice_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_extendedprice]) * 100.0);

          // l_discount agg
          long l_discount_int = roundToLong(resultFromStr<double>(tokens[lineitem::l_discount]) * 100.0);

          string e1 = cm.cm->encrypt_u64_paillier(l_quantity_int);
          string e2 = cm.cm->encrypt_u64_paillier(l_extendedprice_int);
          string e3 = cm.cm->encrypt_u64_paillier(l_discount_int);

          push_binary_string(enccols, e1);
          push_binary_string(enccols, e2);
          push_binary_string(enccols, e3);

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

  DT_CHAR,
  DT_CHAR,

  DT_DATE,
  DT_DATE,
  DT_DATE,

  DT_STRING,
  DT_STRING,
  DT_STRING,
};

vector<int> lineitem_encryptor::Onions = {
  ONION_DETJOIN,
  ONION_DETJOIN,
  ONION_DETJOIN,
  ONION_DETJOIN,

  ONION_DET | ONION_OPE,
  ONION_DET,
  ONION_DET | ONION_OPE,
  ONION_DET,

  ONION_DET,
  ONION_DET,

  ONION_DET | ONION_OPEJOIN,
  ONION_DET | ONION_OPEJOIN,
  ONION_DET | ONION_OPEJOIN,

  ONION_DET,
  ONION_DET,
  ONION_DET,
};

vector<int> lineitem_encryptor::VldbCdbGreedyOnions = {
  ONION_DETJOIN,
  ONION_DETJOIN,
  ONION_DETJOIN,
  ONION_DETJOIN,

  ONION_DET | ONION_OPE,
  ONION_DET,
  ONION_OPE,
  ONION_DET,

  ONION_DET | ONION_OPE,
  ONION_DET | ONION_OPE,

  ONION_OPEJOIN,
  ONION_OPEJOIN,
  ONION_OPEJOIN,

  ONION_DET,
  ONION_DET | ONION_OPE,
  ONION_DET,
};

vector<int> lineitem_encryptor::PackedOnions = {
  0,
  0,
  0,
  0,

  0,
  0,
  0,
  0,

  0,
  0,

  0,
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
      normal_agg,
      projection,

      row_packed_volume,
  };

  static vector<datatypes> PartSuppSchema;
  static vector<int> PartSuppOnions;
  static vector<int> PartSuppProjOnions;

  partsupp_encryptor(enum opt_type tpe)
    : table_encryptor(), tpe(tpe) {

    schema = PartSuppSchema;
    switch (tpe) {
      case none:
      case normal:
        onions = PartSuppOnions;
        usenull = false;
        processrow = true;
        break;
      case projection:
        onions = PartSuppProjOnions;
        usenull = false;
        processrow = false;
        break;
      case row_packed_volume:
        onions = PartSuppProjOnions;
        usenull = false;
        processrow = false;
        break;
      default:
        assert(false);
        break;
    }
  }

  static std::set<uint32_t> IraqNationKeySet;

  struct remover {
    inline bool operator()(const vector<string> &v) const {
      uint32_t suppkey = resultFromStr<uint32_t>(v[partsupp::ps_suppkey]);
      return IraqNationKeySet.find(suppkey) == IraqNationKeySet.end();
    }
  };

  virtual
  void encryptBatch(const vector<vector<string> > &tokens,
                    vector<vector<string> >       &enccols,
                    crypto_manager_stub &cm) {
    typedef PallierSlotManager<uint32_t, 2> PSM;
    assert(tpe == projection || tpe == row_packed_volume);

    switch (tpe) {
      case projection: {
        assert(!IraqNationKeySet.empty());

        vector<vector<string> > filtered(tokens);
        filtered.erase(
            remove_if(filtered.begin(), filtered.end(), remover()),
            filtered.end());

        size_t nbatches = filtered.size() / PSM::NumSlots +
                          ((filtered.size() % PSM::NumSlots) ? 1 : 0);

        for (size_t i = 0; i < nbatches; i++) {
          ZZ z;
          size_t remaining = filtered.size() - i * PSM::NumSlots;
          assert(remaining > 0);
          size_t limit = min(remaining, PSM::NumSlots);
          for (size_t j = 0; j < limit; j++) {
            size_t idx = i * PSM::NumSlots + j;
            assert(idx < filtered.size());
            const vector<string> &v = filtered[idx];
            double ps_value = psValueFromRow(v);
            uint32_t ps_value_int = (uint32_t) roundToLong(ps_value * 100.0);
            PSM::insert_into_slot(z, ps_value_int, j);
            //assert(ps_value_int == PSM::extract_from_slot(z, j));
          }
          string enc = cm.encrypt_Paillier(z);
          vector<string> e;
          do_encrypt(0, DT_INTEGER, ONION_DETJOIN,
                     to_s(IraqNationKey), e, cm, false);
          push_binary_string(e, enc);
          enccols.push_back(e);
        }

        break;
      }

      case row_packed_volume: {
        do_row_pack(tokens, enccols, cm);
        break;
      }

      default: assert(false);
    }
  }

private:
  static const size_t BitsPerAggField = 83;
  static const size_t FieldsPerAgg = 1024 / BitsPerAggField;

  void do_row_pack(const vector<vector<string> > &rows,
                   vector<vector<string> > &enccols,
                   crypto_manager_stub &cm) {
    _static_assert(FieldsPerAgg == 12); // this is hardcoded various places
    size_t nAggs = rows.size() / FieldsPerAgg +
      (rows.size() % FieldsPerAgg ? 1 : 0);
    for (size_t i = 0; i < nAggs; i++) {
      size_t base = i * FieldsPerAgg;
      ZZ z0 = to_ZZ(0);
      for (size_t j = 0; j < min(FieldsPerAgg, rows.size() - base); j++) {
        size_t row_id = base + j;
        const vector<string>& tokens = rows[row_id];
        long ps_volume_int = roundToLong(psValueFromRow(tokens) * 100.0);
        insert_into_slot<BitsPerAggField>(z0, ps_volume_int, j);
      }
      string e0 = cm.encrypt_Paillier(z0);
      e0.resize(256);
      cout << e0;
    }
  }

protected:

  double psValueFromRow(const vector<string> &tokens) {
    // ps_value = ps_supplycost * ps_availqty
    double ps_supplycost = resultFromStr<double>(tokens[partsupp::ps_supplycost]);
    double ps_availqty   = resultFromStr<double>(tokens[partsupp::ps_availqty]);
    double ps_value      = ps_supplycost * ps_availqty;
    return ps_value;
  }

  void precomputeExprs(const vector<string> &tokens,
                       vector<string>       &enccols,
                       crypto_manager_stub        &cm) {
    double ps_value = psValueFromRow(tokens);
    do_encrypt(schema.size(), DT_FLOAT, (ONION_DET | ONION_OPE),
               to_s(ps_value), enccols, cm, usenull);

    // manually encrypt agg...
    long t = roundToLong(ps_value * 100.0);
    assert(t > 0);
    uint64_t ps_value_int = (uint64_t) t;
    typedef PallierSlotManager<uint64_t, 2> PSM;
    ZZ z = to_ZZ(0);
    PSM::insert_into_slot(z, ps_value_int, 0);
    string enc = cm.encrypt_Paillier(z);
    push_binary_string(enccols, enc);
  }

  void precomputeForNormal(const vector<string>& tokens,
                           vector<string>& enccols,
                           crypto_manager_stub &cm) {
    double ps_value = psValueFromRow(tokens);
    do_encrypt(0, DT_FLOAT, ONION_DET,
               to_s(ps_value), enccols, cm, usenull);
  }

  virtual
  void postprocessRow(const vector<string> &tokens,
                      vector<string>       &enccols,
                      crypto_manager_stub        &cm) {
    switch (tpe) {
      case opt_type::normal:
        precomputeForNormal(tokens, enccols, cm);
        break;
      case opt_type::normal_agg:
        precomputeExprs(tokens, enccols, cm);
        break;
      default: break;
    }
  }

private:
  enum opt_type tpe;

};



std::set<uint32_t> partsupp_encryptor::IraqNationKeySet
  (IraqNationSuppKeys, IraqNationSuppKeys + NELEMS(IraqNationSuppKeys));

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
  ONION_DET | ONION_OPE,
  ONION_DET,
};

vector<int> partsupp_encryptor::PartSuppProjOnions = {
  0,
  0,
  0,
  0,
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
  ONION_DET | ONION_OPE,
  ONION_DET,
  ONION_DETJOIN,
  ONION_DET,
  ONION_DET | ONION_OPE,
  ONION_DET,
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
  ONION_DET | ONION_OPE,
  ONION_DETJOIN,
  ONION_DET,
};

//----------------------------------------------------------------------------
// part

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

static const vector<datatypes> PartSchema = {
  DT_INTEGER,
  DT_STRING,
  DT_STRING,
  DT_STRING,
  DT_STRING,
  DT_INTEGER,
  DT_STRING,
  DT_FLOAT,
  DT_STRING,
};

static const vector<int> PartOnions = {
  ONION_DETJOIN | ONION_OPE,
  ONION_DET | ONION_SEARCH,
  ONION_DET,
  ONION_DET,
  ONION_DET | ONION_SEARCH,
  ONION_DET | ONION_OPE,
  ONION_DET,
  ONION_DET,
  ONION_DET,
};

//----------------------------------------------------------------------------
// region

enum region {
  r_regionkey,
  r_name,
  r_comment,
};

static const vector<datatypes> RegionSchema = {
  DT_INTEGER,
  DT_STRING,
  DT_STRING,
};

static const vector<int> RegionOnions = {
  ONION_DETJOIN,
  ONION_DET,
  ONION_DET,
};

//----------------------------------------------------------------------------

static map<string, table_encryptor *> EncryptorMap = {
  {"lineitem-none", new lineitem_encryptor(lineitem_encryptor::none)},
  {"lineitem-normal", new lineitem_encryptor(lineitem_encryptor::normal)},
  {"lineitem-packed", new lineitem_encryptor(lineitem_encryptor::packed)},
  {"lineitem-row-packed-disc-price", new lineitem_encryptor(lineitem_encryptor::row_packed_disc_price)},
  {"lineitem-row-packed-revenue", new lineitem_encryptor(lineitem_encryptor::row_packed_revenue)},
  {"lineitem-row-packed-quantity", new lineitem_encryptor(lineitem_encryptor::row_packed_quantity)},
  {"lineitem-row-col-packed", new lineitem_encryptor(lineitem_encryptor::row_col_packed)},

  {"lineitem-normal-vldb-orig", new lineitem_encryptor(lineitem_encryptor::normal_vldb_orig)},
  {"lineitem-normal-vldb-greedy", new lineitem_encryptor(lineitem_encryptor::normal_vldb_greedy)},

  {"partsupp-none", new partsupp_encryptor(partsupp_encryptor::none)},
  {"partsupp-normal", new partsupp_encryptor(partsupp_encryptor::normal)},
  {"partsupp-projection", new partsupp_encryptor(partsupp_encryptor::projection)},

  {"partsupp-row-packed-volume", new partsupp_encryptor(partsupp_encryptor::row_packed_volume)},

  {"supplier-none", new table_encryptor(SupplierSchema, SupplierOnions, false, true)},

  {"nation-none", new table_encryptor(NationSchema, NationOnions, false, true)},

  {"part-none", new table_encryptor(PartSchema, PartOnions, false, true)},

  {"region-none", new table_encryptor(RegionSchema, RegionOnions, false, true)},

  {"orders-none", new orders_encryptor(orders_encryptor::normal)},

  {"orders-normal-vldb-greedy", new orders_encryptor(orders_encryptor::normal_vldb_greedy)},

  {"customer-none", new customer_encryptor(customer_encryptor::normal)},
  {"customer-row-packed-acctbal", new customer_encryptor(customer_encryptor::row_packed_acctbal)},

  {"customer-normal-vldb-orig", new customer_encryptor(customer_encryptor::normal_vldb_orig)},
  {"customer-normal-vldb-greedy", new customer_encryptor(customer_encryptor::normal_vldb_greedy)},
};

static inline string process_input(const string &s, crypto_manager_stub &cm, table_encryptor *tenc) {
  vector<string> tokens;
  tokenize(s, "|", tokens);
  vector<string> columns;
  tenc->encryptRow(tokens, columns, cm);
  return join(columns, "|");
}

static inline void process_batch(const vector<string> &lines,
                                 crypto_manager_stub &cm,
                                 table_encryptor *tenc) {
  vector<vector<string> > tokens;
  for (auto it = lines.begin();
       it != lines.end();
       ++it) {
    vector<string> t;
    tokenize(*it, "|", t);
    tokens.push_back(t);
  }
  vector<vector<string> > rows;
  tenc->encryptBatch(tokens, rows, cm);
  for (auto it = rows.begin();
       it != rows.end();
       ++it) {
    vector<string> &v = *it;
    cout << join(v, "|") << endl;
  }
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

    //cryptdb_logger::enable(log_name_to_group["crypto"]);

    CryptoManager cm("12345");
    //cerr << "paillier public key: " << marshallBinary(cm.getPKInfo()) << endl;

    crypto_manager_stub stub(&cm, false);
    vector<string> lines;
    for (;;) {
        string s;
        getline(cin, s);
        if (!cin.good())
            break;
        if (tenc->shouldProcessRow()) {
          try {
              cout << process_input(s, stub, tenc) << endl;
          } catch (...) {
              cerr << "Input line failed:" << endl
                   << "  " << s << endl;
          }
        } else {
          lines.push_back(s);
        }
    }
    if (!tenc->shouldProcessRow()) {
      process_batch(lines, stub, tenc);
    }
    return 0;
}
