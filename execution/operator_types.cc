#include <cstdio>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <unordered_map>
#include <set>
#include <pthread.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <parser/cdb_helpers.hh>
#include <crypto/paillier.hh>

#include <util/likely.hh>
#include <util/serializer.hh>
#include <util/stl.hh>
#include <util/timer.hh>

#include <execution/common.hh>
#include <execution/encryption.hh>
#include <execution/operator_types.hh>
#include <execution/query_cache.hh>

using namespace std;

// XXX: TRACE_OPERATOR() is not thread-safe
#define TRACE_OPERATOR() \
  do { \
    static size_t _n = 0; \
    _n++; \
    dprintf("called %s times\n", TO_C(_n)); \
  } while (0)

#define WARN_IF_NOT_EMPTY(t)\
  do { \
    if (UNLIKELY(!t.empty())) { \
      dprintf("WARNING: input %s was not empty\n", #t); \
    } \
  } while (0)

static inline bool is_valid_param_token(char c)
{
  return isalnum(c) || c == '_' || c == '$';
}

void
remote_sql_op::open(exec_context& ctx)
{
  physical_operator::open(ctx);

  // execute all the children for side-effects first
  for (auto c : _children) c->execute_and_discard(ctx);

  // generate params
  _do_cache_write_sql = _sql;
  if (_param_generator || !_named_subselects.empty()) {
    sql_param_generator::param_map param_map = _param_generator->get_param_map(ctx);

    // this code is not smart now- we should ignore substitution sequences
    // which are contained within any string literals (but there shouldn't be any
    // actually, which is why we don't care for now)

    ostringstream buf;
    for (size_t i = 0; i < _do_cache_write_sql.size();) {
      char ch = _do_cache_write_sql[i];
      switch (ch) {
        case ':': {
          i++;
          ostringstream p;
          while (i < _do_cache_write_sql.size() &&
                 is_valid_param_token(_do_cache_write_sql[i])) {
            p << _do_cache_write_sql[i];
            i++;
          }
          string p0 = p.str();
          SANITY(!p0.empty());

          // if first char numeric, then look in pos map
          // otherwise, named subselect
          // TODO: hacky
          if (isdigit(p0[0])) {
#ifdef EXTRA_SANITY_CHECKS
            for (size_t i = 1; i < p0.size(); i++) {
              if (!isdigit(p0[i])) cerr << "bad token found: " << p0 << endl;
              SANITY(isdigit(p0[i]));
            }
#endif
            size_t idx = resultFromStr<size_t>(p0);
            SANITY(param_map.find(idx) != param_map.end());
            buf << param_map[idx].sqlify(true);
          } else {
            SANITY(_named_subselects.find(p0) != _named_subselects.end());

            // find + exec named subselect
            physical_operator* op = _named_subselects[p0];
            SANITY(op);
            dprintf("executing named subselect %s\n", TO_C(p0));

            db_tuple_vec tuples;
            op->open(ctx);
            op->slurp(ctx, tuples);
            op->close(ctx);

            if (tuples.empty()) {
              // UGLY hack to select an empty relation
              buf << "select 1 where 1=2";
            } else {
              // write the results as a comma separated list
              for (size_t i = 0; i < tuples.size(); i++) {
                db_tuple& tuple = tuples[i];
                SANITY(tuple.columns.size() == 1);
                buf << tuple.columns.front().sqlify(false);
                if ((i + 1) != tuples.size()) buf << ",";
              }
            }

            // TODO: cache the results
          }

          break;
        }
        default:
          buf << ch;
          i++;
          break;
      }
    }
    _do_cache_write_sql = buf.str();
  }

  dprintf("executing sql: %s\n", _do_cache_write_sql.c_str());

  // look in query cache first
  if (ctx.cache) {
    auto it = ctx.cache->cache.find(_do_cache_write_sql);
    if (it != ctx.cache->cache.end()) {
      _read_cursor = 0;
      _cached_results = it->second;
      SANITY(_res == NULL); // sets read
      dprintf("cache hit! %s rows\n", TO_C(_cached_results.size()));
      if (!_cached_results.empty()) {
        dprintf("  answer[0]: %s\n", TO_C(_cached_results[0]));
      }
      return;
    }
  }

  // open and execute the sql statement
  timer t;
  if (!ctx.connection->execute(_do_cache_write_sql, _res)) {
    throw runtime_error("Could not execute sql");
  }
  SANITY(_res);

  dprintf("query took %s ms to execute - expect %s rows\n",
          TO_C( double(t.lap()) / 1000.0 ),
          TO_C(_res->size()));
}

void
remote_sql_op::close(exec_context& ctx)
{
  physical_operator::close(ctx);

  // place result in cache if qualifies
  if (_do_cache_write) {
    SANITY(!_do_cache_write_sql.empty());
    if (ctx.cache) {
      ctx.cache->cache[_do_cache_write_sql] = _cached_results;
    }
    _do_cache_write = false;
  }

  _cached_results.clear();
  _do_cache_write_sql.clear();

  // close the sql statement
  if (_res) {
    delete _res;
    _res = NULL;
  }
}

bool
remote_sql_op::has_more(exec_context& ctx)
{
  return _res ? _res->has_more() : _read_cursor < _cached_results.size();
}

void
remote_sql_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);

  SANITY(has_more(ctx));

  if (_res) {
    // read from DB
    ResType res;
    _res->next(res);
    tuples.resize(res.rows.size());
    for (size_t i = 0; i < res.rows.size(); i++) {

#ifdef EXTRA_SANITY_CHECKS
      ssize_t vsize = -1;
#endif

      vector<SqlItem>& row = res.rows[i];
      db_tuple& tuple = tuples[i];
      size_t c = row.size();
      tuple.columns.reserve(c);

      for (size_t j = 0; j < c; j++) {
        SqlItem& col = row[j];
        //cerr << "col #: " << j << endl;
        //cerr << "  col.data: " << col.data << endl;
        //cerr << "  col.type: " << col.type << endl;
        if (_desc_vec[j].is_vector()) {

          // TODO: this is broken in general, but we might
          // be able to get away with it for TPC-H. there are a
          // pretty big set of assumptions here that allows this to work
          // properly. we'll fix if we need more flexibility

          // if the underlying type is string (binary), then we encode
          // it in hex. otherwise, if the underlying type is numeric, then
          // we use the ascii repr of the number.

          SANITY(db_elem::TypeFromPGOid(col.type) == db_elem::TYPE_STRING);

          // whitelist the cases we are ready to handle (add more if needed)
          SANITY(_desc_vec[j].type == db_elem::TYPE_INT ||
                 _desc_vec[j].type == db_elem::TYPE_DATE ||
                 _desc_vec[j].type == db_elem::TYPE_STRING ||
                 _desc_vec[j].type == db_elem::TYPE_DECIMAL_15_2);

          // simplifying assumption for now (relax if needed)
          SANITY(_desc_vec[j].onion_type == oDET);

          if (_desc_vec[j].type != db_elem::TYPE_STRING) {
            vector<db_elem> elems;
            const uint8_t* p = (const uint8_t *) col.data.data();
            const uint8_t* end = p + col.data.size();

            switch (_desc_vec[j].vtype) {
              case db_column_desc::vtype_normal:
                SANITY((col.data.size() % sizeof(uint64_t)) == 0);
                elems.reserve((col.data.size() / sizeof(uint64_t)));
                while (p < end) {
                  uint64_t v;
                  deserializer<uint64_t>::read(p, v);
                  // we don't do the decryption yet, so we make everything an
                  // int here
                  elems.push_back(db_elem(int64_t(v)));
                }
                break;

              // vtype_delta_compressed and vtype_dict_compressed are
              // rendered useless by using standard compression

              default: assert(false);
            }

            SANITY(p == end);
            tuple.columns.push_back(db_elem(elems));
          } else {
            SANITY(_desc_vec[j].vtype == db_column_desc::vtype_normal);

            vector<string> tokens;
            tokenize(col.data, ",", tokens); // this is always safe, because ',' is
                                             // not ever going to be part of the data
#ifdef EXTRA_SANITY_CHECKS
            // check that every vector group we pull out has the same size
            if (vsize == -1) {
              vsize = tokens.size();
            } else {
              SANITY((size_t)vsize == tokens.size());
            }
#endif
            bool needUnhex = (_desc_vec[j].type == db_elem::TYPE_STRING);
            vector<db_elem> elems;
            elems.reserve(tokens.size());
            for (auto t : tokens) {
              if (needUnhex) {
                elems.push_back(CreateFromString(from_hex(t), db_elem::TYPE_STRING));
              } else {
                elems.push_back(CreateFromString(t, db_elem::TYPE_INT));
              }
            }
            tuple.columns.push_back(db_elem(elems));
          }
        } else {
          tuple.columns.push_back(
              CreateFromString(col.data, db_elem::TypeFromPGOid(col.type)));
        }
      }
    }

    // for now, only write to query cache if 0/1 result
    if (_res->size() <= 1) {
      SANITY(tuples.size() == _res->size());
      _cached_results = tuples;
      _do_cache_write = true;
    }
  } else {
    // read from cache
    SANITY(_read_cursor == 0);
    tuples = _cached_results;
    _read_cursor = _cached_results.size();
  }
}

db_elem
remote_sql_op::CreateFromString(
    const string& data,
    db_elem::type type) {
  switch (type) {
    case db_elem::TYPE_INT:
      return (db_elem(resultFromStr<int64_t>(data)));

    case db_elem::TYPE_BOOL:
      SANITY(data == "f" || data == "t");
      return (db_elem(data == "t"));

    case db_elem::TYPE_DOUBLE:
      return (db_elem(resultFromStr<double>(data)));

    case db_elem::TYPE_STRING:
      return (db_elem(data));

    case db_elem::TYPE_DATE:
      {
        int64_t encoding = resultFromStr<int64_t>(data);
        uint32_t m, d, y;
        extract_date_from_encoding(encoding, m, d, y);
        return db_elem(y, m, d);
      }

    default:
      cerr << __PRETTY_FUNCTION__ << ": unhandled type (" << type << ")" << endl;
      throw runtime_error("should not be reached");
  }
}

local_decrypt_op::desc_vec
local_decrypt_op::tuple_desc()
{
  desc_vec v = first_child()->tuple_desc();
  desc_vec ret;
  ret.reserve(v.size());
  set<size_t> s = util::vec_to_set(_pos);
  for (size_t i = 0; i < v.size(); i++) {
    if (s.count(i) == 1) {
      ret.push_back(v[i].decrypt_desc());
    } else {
      ret.push_back(v[i]);
    }
  }
  return ret;
}

// a decrypted agg cannot exceed this many bytes
// these are the same for now
static const size_t MaxAggPtSizeOrig = 256;
static const size_t MaxAggPtSize = 256;

static db_elem
do_decrypt_hom_agg(exec_context& ctx, const string& data)
{
  const uint8_t* p = (const uint8_t *) data.data();
  //dprintf("got %s bytes data\n", TO_C(data.size()));

  assert(data.size() >= 20);

  using namespace hom_agg_constants;

  // read header
  uint32_t ct_agg_size_bytes /* *CIPHERTEXT* size in *BYTES* */, rows_per_agg;
  deserializer<uint32_t>::read(p, ct_agg_size_bytes);
  deserializer<uint32_t>::read(p, rows_per_agg);
  SANITY(ct_agg_size_bytes > 0);
  SANITY(rows_per_agg > 0);
  SANITY((ct_agg_size_bytes % 2) == 0); // ct agg size must be even

  //cerr << "ct_agg_size_bytes = " << ct_agg_size_bytes << ", rows_per_agg = " << rows_per_agg << endl;

  uint32_t dec_slots_per_row = (ct_agg_size_bytes * 8 / 2) / (BitsPerDecimalSlot * rows_per_agg);
  SANITY(dec_slots_per_row > 0);

  //cerr << "dec_slots_per_row = " << dec_slots_per_row << endl;

  NTL::ZZ row_mask = (NTL::to_ZZ(1) << (BitsPerDecimalSlot * dec_slots_per_row)) - 1;
  SANITY(NTL::NumBits(row_mask) == (int)(BitsPerDecimalSlot * dec_slots_per_row));

  // create a row mask

  // setup crypto keys
  //cerr << "keygen arg0: " << (ct_agg_size_bytes * 8 / 2) << endl;
  //cerr << "keygen arg1: " << (ct_agg_size_bytes) << endl;
  auto pp = ctx.pp_cache->get_paillier_priv(ct_agg_size_bytes * 8 / 2, ct_agg_size_bytes);

  // read group count
  uint64_t group_count;
  deserializer<uint64_t>::read(p, group_count);
  //dprintf("group count(*): %s\n", TO_C(group_count));

  // read number of aggs
  uint32_t n_aggs;
  deserializer<uint32_t>::read(p, n_aggs);

  //cerr << "n_aggs: " << n_aggs << endl;

  // TODO: if n_aggs is large enough consider parallel decryption
  NTL::ZZ accum = NTL::to_ZZ(0);
  for (uint32_t i = 0; i < n_aggs; i++) {
    // read interest mask
    uint32_t interest_mask;
    deserializer<uint32_t>::read(p, interest_mask);
    SANITY(interest_mask != 0);
    //cerr << "imask: " << interest_mask << endl;

    // read ciphertext + decrypt
    NTL::ZZ pt;
    if (ct_agg_size_bytes == 256) {
      ctx.crypto->cm->decrypt_Paillier(string((const char *)p, ct_agg_size_bytes), pt);
    } else {
      NTL::ZZ ct = NTL::ZZFromBytes((const uint8_t *) p, ct_agg_size_bytes);
      pt = pp.decrypt(ct);
    }
    p += ct_agg_size_bytes;

    // fill this agg into accum
    for (uint32_t i = 0; i < rows_per_agg; i++) {
      if (!(interest_mask & (0x1 << i))) continue;

      //cerr << "sum_qty_int = "
      //  << NTL::to_long(
      //      (pt >> (BitsPerDecimalSlot * i * dec_slots_per_row)) & mask ) << endl;

      // mask out the dec slots from the row, and add it to accum
      accum += (pt & (row_mask << (i * BitsPerDecimalSlot * dec_slots_per_row)));
    }
  }

  // assert we consumed all the data
  SANITY(p == (const uint8_t *)(data.data() + data.size()));

  // now collaspe each row into one agg (adding the slots to each other)
  NTL::ZZ accum_final = NTL::to_ZZ(0);
  for (uint32_t i = 0; i < rows_per_agg; i++) {
    accum_final += ((accum >> (i * BitsPerDecimalSlot * dec_slots_per_row)) & row_mask);
  }

  // now we write this data into a string db_elem, where the data in the string
  // is the plaintext ZZ

  ostringstream buf;
  //serializer<uint64_t>::write(buf, group_count);
  string x = StringFromZZ(accum_final);
  buf << x;

  //cerr << "x.size() = " << x.size() << endl;
  //cerr << "accum_final(slot0) = " << NTL::to_long( (accum_final & mask) ) << endl;

  return db_elem(buf.str());
}

static db_elem
do_decrypt_hom_agg_original(exec_context& ctx, const string& data)
{
  NTL::ZZ pt;
  ctx.crypto->cm->decrypt_Paillier(data, pt);
  return db_elem(StringFromZZ(pt));
}

template <typename Key, typename Value>
class random_eviction_cache {
public:
  explicit random_eviction_cache(size_t s)
    : _max_elems(s), _n_lookups(0), _n_hits(0), _n_evictions(0)
  { SANITY(s > 0); }

  bool lookup(const Key& k, Value& v) {
    _n_lookups++;
    auto it = _cache.find(k);
    if (it != _cache.end()) {
      _n_hits++;
      v = it->second;
      return true;
    } else {
      return false;
    }
  }

  void insert(const Key& k, const Value& v) {
    if (_cache.size() == _max_elems) {
      _n_evictions++;
      // TODO: make random
      _cache.erase(_cache.begin());
    }
    SANITY(_cache.size() < _max_elems);
    _cache[k] = v;
  }

  inline size_t n_lookups() const { return _n_lookups; }
  inline size_t n_hits() const { return _n_hits; }
  inline size_t n_evictions() const { return _n_evictions; }

private:
  size_t _max_elems;
  std::unordered_map<Key, Value> _cache;

  size_t _n_lookups;
  size_t _n_hits;
  size_t _n_evictions;
};

struct decrypt_cache_key {
  db_elem::type type;
  size_t size;
  onion onion_type;
  SECLEVEL level;
  size_t pos;
  string ciphertext;
};

namespace std {
  template <> struct hash<decrypt_cache_key> {
    inline size_t operator()(const decrypt_cache_key& k) const {
      // TODO: better hash function
      return
        size_t(k.type) ^ k.size ^ size_t(k.onion_type) ^
        size_t(k.level) ^ k.pos ^ hash<string>()(k.ciphertext);
    }
  };
  template <> struct equal_to<decrypt_cache_key> {
    inline bool operator()(const decrypt_cache_key& a,
                           const decrypt_cache_key& b) const {
      return a.type == b.type &&
             a.size == b.size &&
             a.onion_type == b.onion_type &&
             a.level == b.level &&
             a.pos == b.pos &&
             a.ciphertext == b.ciphertext;
    }
  };
}

typedef random_eviction_cache<decrypt_cache_key, db_elem>
        decrypt_cache;

static db_elem
do_decrypt_db_elem_op(
    exec_context& ctx,
    const db_elem& elem,
    const db_column_desc& d)
{
  SANITY(elem.get_type() != db_elem::TYPE_VECTOR);
  string s = elem.stringify();

#ifdef ALL_SAME_KEY
  size_t pos = 0;
  SECLEVEL level =
    (d.level == SECLEVEL::DETJOIN) ? SECLEVEL::DET :
      ((d.level == SECLEVEL::OPEJOIN) ? SECLEVEL:: OPE :
        d.level);
#else
  size_t pos     = d.pos;
  SECLEVEL level = d.level;
#endif /* ALL_SAME_KEY */

  switch (d.type) {
    case db_elem::TYPE_INT: {

      SANITY(d.size == 1 ||
             d.size == 2 ||
             d.size == 4 ||
             d.size == 8);

      SANITY(d.onion_type == oDET ||
             d.onion_type == oOPE);

      bool isDet = d.onion_type == oDET;

      switch (d.size) {
        case 1:
          return db_elem((int64_t)
            (isDet ? decrypt_u8_det(ctx.crypto, s, pos, level) :
                     decrypt_u8_ope(ctx.crypto, s, pos, level)));
        case 2:
          return db_elem((int64_t)
            (isDet ? decrypt_u16_det(ctx.crypto, s, pos, level) :
                     decrypt_u16_ope(ctx.crypto, s, pos, level)));
        case 4:
          return db_elem((int64_t)
            (isDet ? decrypt_u32_det(ctx.crypto, s, pos, level) :
                     decrypt_u32_ope(ctx.crypto, s, pos, level)));
        case 8:
          return db_elem((int64_t)
            (isDet ? decrypt_u64_det(ctx.crypto, s, pos, level) :
                     decrypt_u64_ope(ctx.crypto, s, pos, level)));
      }

      assert(false);
    }

    case db_elem::TYPE_CHAR: {

      SANITY(d.size == 1);
      SANITY(d.onion_type == oDET ||
             d.onion_type == oOPE);

      uint64_t x =
        (d.onion_type == oDET) ?
          decrypt_u8_det(ctx.crypto, s, pos, level) :
          decrypt_u8_ope(ctx.crypto, s, pos, level) ;

      SANITY( x <= numeric_limits<unsigned char>::max() );

      return db_elem(char(x));
    }

    case db_elem::TYPE_STRING: {

      SANITY(d.onion_type == oDET ||
             d.onion_type == oAGG ||
             d.onion_type == oAGG_ORIGINAL);
      // NO decryption of OPE onions

      switch (d.onion_type) {
        case oAGG:
          return do_decrypt_hom_agg(ctx, s);
        case oAGG_ORIGINAL:
          return do_decrypt_hom_agg_original(ctx, s);
        default:
          return db_elem(decrypt_string_det(ctx.crypto, s, pos, level));
      }
    }

    case db_elem::TYPE_DATE: {

      SANITY(d.size == 3);

      SANITY(d.onion_type == oDET ||
             d.onion_type == oOPE);

      uint64_t x = (d.onion_type == oDET) ?
        decrypt_date_det(ctx.crypto, s, pos, level) :
        decrypt_date_ope(ctx.crypto, s, pos, level) ;

      uint32_t m, d, y;
      extract_date_from_encoding(x, m, d, y);

      //cerr << "date ciphertext: " << s << endl;
      //cerr << "date encoding: " << x << endl;
      //cerr << "date extract: " << y << "-" << m << "-" << d << endl;

      return db_elem(y, m, d);
    }

    // TODO: fix hack...
    case db_elem::TYPE_DECIMAL_15_2: {

      SANITY(d.onion_type == oDET ||
             d.onion_type == oOPE);

      uint64_t x =
        (d.onion_type == oDET) ?
          decrypt_decimal_15_2_det(ctx.crypto, s, pos, level) :
          decrypt_decimal_15_2_ope(ctx.crypto, s, pos, level) ;

      return db_elem(double(x)/100.0);
    }

    default:
      cerr << "got unhandled type(" << d.type << ")" << endl;
      throw runtime_error("UNIMPL");
  }
}

static db_elem
do_decrypt_db_elem_op_with_cache(
    exec_context& ctx,
    const db_elem& elem,
    const db_column_desc& d,
    decrypt_cache& cache)
{
  decrypt_cache_key key;
  key.type       = d.type;
  key.size       = d.size;
  key.onion_type = d.onion_type;
  key.level      = d.level;
  key.pos        = d.pos;
  key.ciphertext = elem.stringify(); // TODO: avoid redundant stringify

  // lookup value first
  db_elem ret;
  if (cache.lookup(key, ret)) {
    // fast path
    return ret;
  }

  // slow path
  ret = do_decrypt_db_elem_op(ctx, elem, d);
  cache.insert(key, ret);
  return ret;
}

struct decrypt_db_elem_args {
  exec_context* ctx;
  const vector< db_elem >* elems;
  const db_column_desc* desc;
  vector< db_elem >* results;
};

static void* do_decrypt_db_elem_op_wrapper(void* p) {
  decrypt_db_elem_args* args = (decrypt_db_elem_args *) p;
  size_t n = args->elems->size();
  args->results->reserve(n);
  for (size_t i = 0; i < n; i++) {
    args->results->push_back(
        do_decrypt_db_elem_op(*args->ctx, args->elems->operator[](i), *args->desc));
  }
  return NULL;
}

// on vise4, it takes about 0.377 ms to spawn and join 8 threads
// (which do nothing). it also takes about 0.0173 ms to do one DET
// decryption. doing the math, this means we should switch over to
// parallel (8 threads) at n = 25 elems
static const size_t ParThreshold = 25;
static const size_t NThreads = 8;

static bool column_desc_qualifies_for_parallel_decryption(const db_column_desc& d)
{
  if (d.onion_type == oAGG ||
      d.onion_type == oAGG_ORIGINAL) return false;
  return true;
}

// modifies tuple in place
static void
do_decrypt_db_tuple_op(
    exec_context& ctx,
    db_tuple& tuple,
    const local_decrypt_op::desc_vec& desc,
    const local_decrypt_op::pos_vec& pos,
    const vector<decrypt_cache*>& column_caches)
{
  SANITY(tuple.columns.size() == desc.size());
  SANITY(column_caches.empty() || column_caches.size() == desc.size());

  for (auto p : pos) {
    const db_column_desc& d = desc[p];
    db_elem& e = tuple.columns[p];
    decrypt_cache* cache = column_caches.empty() ? NULL : column_caches[p];
    SANITY(cache || column_caches.empty());

    if (e.is_vector()) {
      vector< db_elem > elems;
      elems.reserve(e.size());

      if (e.size() >= ParThreshold) {

        SANITY(column_desc_qualifies_for_parallel_decryption(d));

        // TODO: bootstrap these vector parallel decrypts with
        // the given decrypt cache. we don't just pass it along
        // because we want the decrypt_cache to avoid concurrenecy
        // control

        //dprintf("using parallel impl to decrypt %s elems\n", TO_C(e.size()));
        timer t;

        vector< vector< db_elem > > groups;
        SplitRowsIntoGroups(groups, e.elements(), NThreads);
        SANITY(groups.size() == NThreads);

        // TODO: all threads write into shared array
        vector< vector< db_elem > > group_results;
        group_results.resize(NThreads);

        // TODO: should really use a thread-pool of workers. However:
        // A) the exec implementation is not quite there yet
        // B) this is easier though- no need for managing a global shared thread-pool
        //    (and measured to be not that slow)

        pthread_t thds[NThreads];
        decrypt_db_elem_args args[NThreads];

        for (size_t i = 0; i < NThreads; i++) {
          args[i].ctx     = &ctx;
          args[i].elems   = &groups[i];
          args[i].desc    = &d;
          args[i].results = &group_results[i];
          int r = pthread_create(&thds[i], NULL, do_decrypt_db_elem_op_wrapper, &args[i]);
          if (r) SANITY(false);
        }

        for (size_t i = 0; i < NThreads; i++) {
          int r = pthread_join(thds[i], NULL);
          if (r) SANITY(false);
          // TODO: this copy is un-necessary
          elems.reserve(elems.size() + group_results[i].size());
          elems.insert(elems.end(), group_results[i].begin(), group_results[i].end());
        }

        SANITY(elems.size() == e.size());
        //dprintf("decryption took %s ms\n", TO_C( double(t.lap()) / 1000.0 ));

      } else {
        for (auto &e0 : e) {
          elems.push_back(
              cache ?
                do_decrypt_db_elem_op_with_cache(ctx, e0, d, *cache) :
                do_decrypt_db_elem_op(ctx, e0, d));
        }
      }

      tuple.columns[p] = db_elem(elems);
    } else {
      tuple.columns[p] =
        cache ?
          do_decrypt_db_elem_op_with_cache(ctx, e, d, *cache) :
          do_decrypt_db_elem_op(ctx, e, d);
    }
  }
}

struct decrypt_db_tuple_args {
  exec_context* ctx;
  const vector< db_tuple* >* tuples;
  const local_decrypt_op::desc_vec* desc;
  const local_decrypt_op::pos_vec* pos;
};

static void* do_decrypt_db_tuple_op_wrapper(void* p) {
  decrypt_db_tuple_args* args = (decrypt_db_tuple_args *) p;

  // use a thread-local decrypt cache
  vector<decrypt_cache*> caches;
  caches.reserve(args->desc->size());
  for (size_t i = 0; i < args->desc->size(); i++) {
    caches.push_back(new decrypt_cache(512));
  }

  size_t n = args->tuples->size();
  for (size_t i = 0; i < n; i++) {
    // modifies tuples[i]
    do_decrypt_db_tuple_op(
      *args->ctx, *args->tuples->operator[](i), *args->desc, *args->pos, caches);
  }

  // cleanup caches
  for (size_t i = 0; i < args->desc->size(); i++) {
    // emit cache stats

    dprintf("cache for col=(%s) stats: n_lookups=(%s), n_hits_pct=(%s), n_evictions=(%s)\n",
            TO_C(i),
            TO_C(caches[i]->n_lookups()),
            TO_C(double(caches[i]->n_hits()) / double(caches[i]->n_lookups()) * 100.0),
            TO_C(caches[i]->n_evictions()));

    delete caches[i];
  }
  return NULL;
}

static bool
desc_vec_qualifies_for_parallel_decryption(
    const local_decrypt_op::desc_vec& dv,
    const local_decrypt_op::pos_vec& pv)
{
  // NTL isn't thread-safe unfortunately. So we can't do
  // hom decryptions in parallel, unfortunately.
  for (auto p : pv) {
    if (!column_desc_qualifies_for_parallel_decryption(dv[p])) return false;
  }
  return true;
}

// modifies tuples in place
static void
do_parallel_tuple_decrypt(
    exec_context& ctx,
    vector<db_tuple>& tuples,
    const local_decrypt_op::desc_vec& desc,
    const vector<size_t>& pos)
{
  // take pointers of all the tuples (modify in place)
  vector< db_tuple* > tuple_pointers;
  tuple_pointers.reserve(tuples.size());
  for (size_t i = 0; i < tuples.size(); i++) {
    tuple_pointers.push_back(&tuples[i]);
  }

  vector< vector< db_tuple* > > groups;
  SplitRowsIntoGroups(groups, tuple_pointers, NThreads);
  SANITY(groups.size() == NThreads);

  pthread_t thds[NThreads];
  decrypt_db_tuple_args args[NThreads];

  for (size_t i = 0; i < NThreads; i++) {
    args[i].ctx     = &ctx;
    args[i].tuples  = &groups[i];
    args[i].desc    = &desc;
    args[i].pos     = &pos;
    int r = pthread_create(&thds[i], NULL, do_decrypt_db_tuple_op_wrapper, &args[i]);
    if (r) SANITY(false);
  }

  for (size_t i = 0; i < NThreads; i++) {
    int r = pthread_join(thds[i], NULL);
    if (r) SANITY(false);
  }
}

void
local_decrypt_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);

  SANITY(first_child()->has_more(ctx));
  desc_vec desc = first_child()->tuple_desc();

  first_child()->next(ctx, tuples);

  // three levels of potential parallelism:
  // 0) each row can be decrypted in parallel
  // 1) each column can be decrypted in parallel
  // 2) within a column, vector contexts can be decrypted in parallel
  //
  // for now, (2) is the only one really worth exploiting (maybe (1) if time
  // permitting)

#ifdef EXTRA_SANITY_CHECKS
  for (auto p : _pos) {
    if (p >= desc.size()) {
      cerr << "ERROR: p=(" << p << "), desc.size()=(" << desc.size() << ")" << endl;
    }
    SANITY(p < desc.size());
  }
#endif

  bool enough_tuples = tuples.size() >= ParThreshold;
  bool do_par = enough_tuples &&
                desc_vec_qualifies_for_parallel_decryption(desc, _pos);

  timer t;
  if (do_par) {
    // easy parallel case
    dprintf("using parallel impl to decrypt %s tuples\n", TO_C(tuples.size()));
    do_parallel_tuple_decrypt(ctx, tuples, desc, _pos);
  } else if (enough_tuples) {
    // not as easy parallel case
    dprintf("SUB-OPTIMALITY: having to use fork parallel decryption for %s tuples\n",
            TO_C(tuples.size()));

    vector< db_tuple* > tuple_pointers;
    tuple_pointers.reserve(tuples.size());
    for (size_t i = 0; i < tuples.size(); i++) {
      tuple_pointers.push_back(&tuples[i]);
    }
    size_t nelems_per_group = tuple_pointers.size() / NThreads; // excluding the last group

    vector< vector< db_tuple* > > groups;
    SplitRowsIntoGroups(groups, tuple_pointers, NThreads);
    SANITY(groups.size() == NThreads);

    set<size_t> already_decrypted;
    for (auto col : _pos) {
      if (!column_desc_qualifies_for_parallel_decryption(desc[col])) {
        already_decrypted.insert(col);

        SANITY(desc[col].type == db_elem::TYPE_STRING);

        bool orig = desc[col].onion_type == oAGG_ORIGINAL;
        size_t mmap_elem_size = ((orig ? MaxAggPtSizeOrig : MaxAggPtSize) + sizeof(uint32_t));

        vector<char*> mapped_regions;
        mapped_regions.reserve(NThreads);
        for (size_t i = 0; i < NThreads; i++) {
          char* p = (char *) mmap(NULL, groups[i].size() * mmap_elem_size,
                                  PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
          SANITY(p != MAP_FAILED);
          mapped_regions.push_back(p);
        }

        vector<pid_t> pids;
        pids.reserve(NThreads);
        for (size_t i = 0; i < NThreads; i++) {
          pid_t pid;
          switch ((pid = fork())) {
            case -1: SANITY(false);
            case 0: // child
              {
                char *group_buffer = mapped_regions[i];
                // take control of group i
                auto &group = groups[i];
                for (size_t row = 0; row < group.size(); row++) {
                  char *row_buffer = group_buffer + row * mmap_elem_size;
                  db_elem x = orig ?
                    do_decrypt_hom_agg_original(ctx, group[row]->columns[col].unsafe_cast_string()) :
                    do_decrypt_hom_agg(ctx, group[row]->columns[col].unsafe_cast_string());
                  string s = x.unsafe_cast_string();
                  SANITY(s.size() <= (orig ? MaxAggPtSizeOrig : MaxAggPtSize));

                  _static_assert(MaxAggPtSize     <= std::numeric_limits<uint32_t>::max());
                  _static_assert(MaxAggPtSizeOrig <= std::numeric_limits<uint32_t>::max());

                  uint32_t* ptr = (uint32_t *) row_buffer;
                  *ptr = s.size();
                  memcpy((row_buffer + sizeof(uint32_t)), s.data(), s.size());
                }
                // msync
                int ret = msync(group_buffer, group.size() * mmap_elem_size, MS_SYNC);
                SANITY(ret != -1);
                _exit(0);
              }
            default: // parent
              pids.push_back(pid);
              break;
          }
        }

        for (auto x : pids) {
          int st;
          int ret = waitpid(x, &st, 0);
          SANITY(ret != -1);
          SANITY(WIFEXITED(st));
          SANITY(WEXITSTATUS(st) == 0);
        }

        // now set the elements to reflect the decrypted versions
        for (size_t i = 0; i < NThreads; i++) {
          char *group_ptr = mapped_regions[i];
          for (size_t j = 0; j < groups[i].size(); j++) {
            char *row_ptr = group_ptr + j * mmap_elem_size;
            size_t row_id = i * nelems_per_group + j;
            tuples[row_id].columns[col] =
              db_elem(string(row_ptr + sizeof(uint32_t), *((uint32_t*)row_ptr)));
          }
          // munmap the region
          int ret = munmap(group_ptr, mmap_elem_size * groups[i].size());
          SANITY(ret != -1);
        }
      }
    }

    SANITY(!already_decrypted.empty());

    // decrypt remaining things in parallel using standard threads
    vector<size_t> new_pos;
    for (auto p : _pos) {
      if (already_decrypted.count(p) == 0) {
        new_pos.push_back(p);
      }
    }
    if (!new_pos.empty()) do_parallel_tuple_decrypt(ctx, tuples, desc, new_pos);

  } else {
    // no tuple-level parallelism
    for (size_t i = 0; i < tuples.size(); i++) {
      db_tuple& tuple = tuples[i];
      do_decrypt_db_tuple_op(ctx, tuple, desc, _pos, vector<decrypt_cache*>());
    }
  }
  dprintf("decryption took %s ms\n", TO_C( double(t.lap()) / 1000.0 ));

}

local_encrypt_op::desc_vec
local_encrypt_op::tuple_desc()
{
  desc_vec v = first_child()->tuple_desc();
  desc_vec ret;
  ret.reserve(v.size());
  map<size_t, db_column_desc> m = util::map_from_pair_vec(_enc_desc_vec);
  for (size_t i = 0; i < v.size(); i++) {
    auto it = m.find(i);
    if (it == m.end()) {
      ret.push_back(v[i]);
    } else {
      ret.push_back(it->second);
    }
  }
  return ret;
}

static db_elem
do_encrypt_op(exec_context& ctx, const db_elem& elem, const db_column_desc& d)
{
  SANITY(elem.get_type() != db_elem::TYPE_VECTOR);

#ifdef ALL_SAME_KEY
  size_t pos = 0;
  SECLEVEL level =
    (d.level == SECLEVEL::DETJOIN) ? SECLEVEL::DET :
      ((d.level == SECLEVEL::OPEJOIN) ? SECLEVEL:: OPE :
        d.level);
#else
  size_t pos     = d.pos;
  SECLEVEL level = d.level;
#endif /* ALL_SAME_KEY */

  switch (d.type) {
    case db_elem::TYPE_INT: {

      SANITY(d.size == 1 ||
             d.size == 2 ||
             d.size == 4 ||
             d.size == 8);

      SANITY(d.onion_type == oDET ||
             d.onion_type == oOPE);

      bool isDet = d.onion_type == oDET;
      bool isJoin = (level == SECLEVEL::DETJOIN) ||
                    (level == SECLEVEL::OPEJOIN);

      // TODO: allow float w/ conversion
      SANITY(elem.get_type() == db_elem::TYPE_INT);
      int64_t val = elem.unsafe_cast_i64();

      switch (d.size) {
        case 1:
          return db_elem((int64_t)
            (isDet ? encrypt_u8_det(ctx.crypto, val, pos, isJoin) :
                     encrypt_u8_ope(ctx.crypto, val, pos, isJoin)));
        case 2:
          return db_elem((int64_t)
            (isDet ? encrypt_u16_det(ctx.crypto, val, pos, isJoin) :
                     encrypt_u16_ope(ctx.crypto, val, pos, isJoin)));
        case 4:
          return db_elem((int64_t)
            (isDet ? encrypt_u32_det(ctx.crypto, val, pos, isJoin) :
                     encrypt_u32_ope(ctx.crypto, val, pos, isJoin)));
        case 8:
          return (isDet ?
              db_elem((int64_t)encrypt_u64_det(ctx.crypto, val, pos, isJoin)) :
              db_elem(encrypt_u64_ope(ctx.crypto, val, pos, isJoin)));
      }

      assert(false);
    }

    case db_elem::TYPE_DECIMAL_15_2: {

      SANITY(d.onion_type == oDET ||
             d.onion_type == oOPE);

      bool isJoin = (level == SECLEVEL::DETJOIN) ||
                    (level == SECLEVEL::OPEJOIN);

      // TODO: allow int w/ conversion
      SANITY(elem.get_type() == db_elem::TYPE_DOUBLE);
      double val = elem.unsafe_cast_double() * 100.0;

      return (d.onion_type == oDET) ?
          db_elem((int64_t)encrypt_decimal_15_2_det(ctx.crypto, val, pos, isJoin)) :
          db_elem(
            str_reverse(
              str_resize(
                encrypt_decimal_15_2_ope(ctx.crypto, val, pos, isJoin), 16))) ;

    }

    default: // TODO: implement more cases
      cerr << "got unhandled type(" << d.type << ")" << endl;
      throw runtime_error("UNIMPL");
  }
}

void
local_encrypt_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);
  map<size_t, db_column_desc> m = util::map_from_pair_vec(_enc_desc_vec);
  SANITY(first_child()->has_more(ctx));
  first_child()->next(ctx, tuples);
  for (auto &tuple : tuples) {
    for (size_t i = 0; i < tuple.columns.size(); i++) {
      auto it = m.find(i);
      if (it != m.end()) {
        tuple.columns[i] = do_encrypt_op(ctx, tuple.columns[i], it->second);
      }
    }
  }
}

void
local_filter_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);
  SANITY(first_child()->has_more(ctx));
  db_tuple_vec v;
  first_child()->next(ctx, v);
  for (auto &tuple : v) {
    exec_context eval_ctx = ctx.bind(&tuple, _subqueries);
    db_elem test;

    // REALLY UGLY HACK
    // If data does not exist then treat the filter as failing
    // This is necessary to get us past Q20.
    //
    // The problem is as follows. Suppose we pull out the following clause
    // (meaning we are running this locally):
    //   col1 < ( select sum(x) from tbl where tbl.id = $outer_scoped_var )
    // In SQL, this basically is doing a join on tbl with the current row. However,
    // if no elements exist, then this row should not pass the predicate test
    // trivially. However, when we remove this predicate, we potentially get back
    // rows with $outer_scoped_var which don't join to tbl. So what's happening
    // is in this case, sum(x) will try to evaluate over an empty vector and
    // fail. So for now we just catch the exception...
    try {
      test = _filter->eval(eval_ctx);
    } catch (no_data_exception& e) {
      dprintf("WARNING: no_data_exception caught in filter, assuming false\n");
      test = db_elem(false);
    }

    SANITY(test.get_type() == db_elem::TYPE_BOOL ||
           test.get_type() == db_elem::TYPE_VECTOR);
    if (test.get_type() == db_elem::TYPE_BOOL) {
      if (test) tuples.push_back(tuple);
    } else {
      // test is a vector mask now (vector of bools)
      if (!test.empty_mask()) {
        // if the vector masks at least leaves one element,
        // construct a filtered tuple by going through each column
        // and applying the mask

        db_tuple t;
        t.columns.reserve(tuple.columns.size());
        for (auto &e : tuple.columns) {
          if (e.is_vector()) {
            t.columns.push_back( e.filter(test) );
          } else {
            t.columns.push_back( e );
          }
        }

        tuples.push_back(t);
      }
    }
  }
}

local_transform_op::desc_vec
local_transform_op::tuple_desc()
{
  desc_vec dv = first_child()->tuple_desc();
  desc_vec ret;
  ret.reserve(_trfm_vec.size());
  for (auto &e : _trfm_vec) {
    if (e.isLeft()) {
      ret.push_back(dv[e.left()]);
    } else {
      ret.push_back(e.right().first);
    }
  }
  return ret;
}

void
local_transform_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);
  SANITY(first_child()->has_more(ctx));
  db_tuple_vec v;
  first_child()->next(ctx, v);
  tuples.reserve(v.size());
  for (auto &t : v) {
    db_tuple tt;
    tt.columns.reserve(_trfm_vec.size());
    for (auto &e : _trfm_vec) {
      if (e.isLeft()) {
        tt.columns.push_back(t.columns[e.left()]);
      } else {
        exec_context eval_ctx = ctx.bind(&t);
        tt.columns.push_back(e.right().second->eval(eval_ctx));
      }
    }
    tuples.push_back(tt);
  }
}

local_group_by::desc_vec
local_group_by::tuple_desc()
{
  desc_vec v = first_child()->tuple_desc();
  set<size_t> s = util::vec_to_set(_pos);
  for (size_t i = 0; i < v.size(); i++) {
    if (s.count(i) == 1) {
      v[i] = v[i].vectorize();
    }
  }
  return v;
}

void
local_group_by::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);
  SANITY(first_child()->has_more(ctx));

  while (first_child()->has_more(ctx)) {
    db_tuple_vec v;
    first_child()->next(ctx, v);
    tuples.reserve(tuples.size() + v.size());
    tuples.insert(tuples.end(), v.begin(), v.end());
  }

  // only strategy is in-memory hash aggregate
  unordered_map< db_tuple, vector< vector<db_elem> > > m;

  map< size_t /* pos */, size_t /* position in group key */ > sm;
  for (size_t i = 0; i < _pos.size(); i++) sm[_pos[i]] = i;

  desc_vec dv = first_child()->tuple_desc();

  // do the hash aggregation
  for (auto &e : tuples) {

    SANITY( dv.size() == e.columns.size() );

    // extract the grouping key
    db_tuple group_key;
    group_key.columns.reserve( _pos.size() );
    for (auto p : _pos) group_key.columns.push_back( e.columns[p] );

    // locate grouping key in map
    auto it = m.find( group_key );
    if (it == m.end()) {
      // new group- initialize it

      vector< vector<db_elem> >& group = m[group_key];
      SANITY( e.columns.size() > _pos.size() );
      group.resize( e.columns.size() - _pos.size() );

      size_t i = 0, j = 0;
      for (; i < e.columns.size(); i++) {
        if (sm.find(i) == sm.end()) {
          group[j].push_back( e.columns[i] );
          j++;
        }
      }

    } else {
      // append each non group key element from this tuple
      vector< vector<db_elem> >& group = it->second;
      size_t i = 0, j = 0;
      for (; i < e.columns.size(); i++) {
        if (sm.find(i) == sm.end()) {
          group[j].push_back( e.columns[i] );
          j++;
        }
      }
    }
  }

  // now emit the results
  tuples.clear();
  for (auto it = m.begin(); it != m.end(); ++it) {
    db_tuple t;
    t.columns.reserve(dv.size());
    size_t i = 0, j = 0;
    for (; i < dv.size(); i++) {
      auto sit = sm.find(i);
      if (sit == sm.end()) {
        t.columns.push_back(db_elem(it->second[j]));
        j++;
      } else {
        t.columns.push_back(it->first.columns[sit->second]);
      }
    }
    tuples.push_back(t);
  }
}

void
local_group_filter::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);

  SANITY(first_child()->has_more(ctx));

  db_tuple_vec v;
  first_child()->next(ctx, v);

  tuples.reserve(v.size());
  for (auto &t : v) {
    exec_context eval_ctx = ctx.bind(&t, _subqueries);
    if (_filter->eval(eval_ctx)) tuples.push_back(t);
  }
}

struct cmp_functor {
  cmp_functor(const local_order_by::sort_keys_vec& sort_keys)
    : _sort_keys(sort_keys) {}
  bool operator()(const db_tuple& lhs, const db_tuple& rhs) const {
    for (auto &p : _sort_keys) {
      if (p.second) { // desc
        if (lhs.columns[p.first] > rhs.columns[p.first]) return true;
        if (lhs.columns[p.first] < rhs.columns[p.first]) return false;
      } else { // asc
        if (lhs.columns[p.first] < rhs.columns[p.first]) return true;
        if (lhs.columns[p.first] > rhs.columns[p.first]) return false;
      }
    }
    return false;
  }
  local_order_by::sort_keys_vec _sort_keys;
};

void
local_order_by::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);

  // TODO: external merge sort?
  SANITY(first_child()->has_more(ctx));

  while (first_child()->has_more(ctx)) {
    db_tuple_vec v;
    first_child()->next(ctx, v);
    tuples.insert(tuples.end(), v.begin(), v.end());
  }

  sort(tuples.begin(), tuples.end(), cmp_functor(_sort_keys));
}

bool
local_limit::has_more(exec_context& ctx)
{
  return first_child()->has_more(ctx) && _i < _n;
}

void
local_limit::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();
  WARN_IF_NOT_EMPTY(tuples);

  SANITY(has_more(ctx));
  db_tuple_vec v;
  first_child()->next(ctx, v);
  size_t i = 0;
  while (i < v.size() && _i < _n) {
    tuples.push_back(v[i]);
    i++; _i++;
  }
}
