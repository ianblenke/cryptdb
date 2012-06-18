#include <algorithm>
#include <unordered_map>
#include <sys/time.h>

#include <udf/edb_common.hh>
#include <util/serializer.hh>

extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"

// utils/lsyscache.h
extern void get_typlenbyvalalign(Oid typid, int16 *typlen, bool *typbyval, char *typalign);

}

#define DBGOUT(x)

static unsigned char *
getba(PG_FUNCTION_ARGS, int i, unsigned int & len)
{
    bytea * eValue = PG_GETARG_BYTEA_P(i);

    len = VARSIZE(eValue) - VARHDRSZ;
    unsigned char * eValueBytes = new unsigned char[len];
    memcpy(eValueBytes, VARDATA(eValue), len);
    return eValueBytes;
}

static unsigned char *
getba_rdonly(PG_FUNCTION_ARGS, int i, unsigned int & len)
{
    bytea * eValue = PG_GETARG_BYTEA_P(i);
    len = VARSIZE(eValue) - VARHDRSZ;
    return (unsigned char *) VARDATA(eValue);
}

static std::string
getvarchar(PG_FUNCTION_ARGS, int i)
{
  VarChar* v = PG_GETARG_VARCHAR_P(i);
  unsigned int len = VARSIZE(v) - VARHDRSZ;
  return std::string(VARDATA(v), len);
}

#define PG_PASS_FARGS fcinfo

static inline std::string
getstring_fromba(PG_FUNCTION_ARGS, int i)
{
  unsigned int len;
  unsigned char *p  = getba_rdonly(PG_PASS_FARGS, i, len);
  return std::string((char*) p, len);
}

static inline unsigned char*
copy_to_pg_return_buffer(const std::string& s)
{
  unsigned char* ret = (unsigned char *) palloc( s.size() + VARHDRSZ );
  SET_VARSIZE(ret, s.size() + VARHDRSZ);
  memcpy(VARDATA(ret), s.data(), s.size());
  return ret;
}

static inline uint64_t cur_usec() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
}

extern "C" {

// create function test_sum_add(bigint, bigint) returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function test_sum_final(bigint) returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate test_sum ( sfunc=test_sum_add, basetype=bigint, stype=bigint, finalfunc=test_sum_final, initcond='0' );

PG_MODULE_MAGIC;

// create function bytea_cmp_less(bytea, bytea) returns bytea language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so' strict;
// create aggregate min ( sfunc=bytea_cmp_less, basetype=bytea, stype=bytea );
Datum bytea_cmp_less(PG_FUNCTION_ARGS);

Datum searchswp(PG_FUNCTION_ARGS);

Datum test_sum_add(PG_FUNCTION_ARGS);
Datum test_sum_final(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(bytea_cmp_less);
PG_FUNCTION_INFO_V1(searchswp);

PG_FUNCTION_INFO_V1(test_sum_add);
PG_FUNCTION_INFO_V1(test_sum_final);

Datum bytea_cmp_less(PG_FUNCTION_ARGS) {
  unsigned int l1, l2;
  unsigned char* b1 = getba_rdonly(PG_PASS_FARGS, 0, l1);
  unsigned char* b2 = getba_rdonly(PG_PASS_FARGS, 1, l2);

  int res = memcmp(b1, b2, l1 < l2 ? l1 : l2);
  unsigned char *p = (res < 0) ? b1 : b2;
  unsigned int l = (res < 0) ? l1 : l2;

  unsigned char* ret = (unsigned char *) palloc( l + VARHDRSZ );
  SET_VARSIZE(ret, l + VARHDRSZ);
  memcpy(VARDATA(ret), p, l);
  PG_RETURN_BYTEA_P(ret);
}

// create function searchSWP(bytea, bytea, bytea) returns bool language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
Datum searchswp(PG_FUNCTION_ARGS) {

  Token t;

  unsigned int ciphLen;
  unsigned char *ciph = getba(PG_PASS_FARGS, 0, ciphLen);

  unsigned int wordKeyLen;
  unsigned char *wordKey = getba(PG_PASS_FARGS, 1, wordKeyLen);

  t.ciph = Binary(ciphLen, ciph);
  t.wordKey = Binary(wordKeyLen, wordKey);

  unsigned int allciphLen;
  unsigned char * allciph = getba(PG_PASS_FARGS, 2, allciphLen);
  Binary overallciph(allciphLen, allciph);

  bool ret = search(t, overallciph);

  delete [] ciph;
  delete [] wordKey;
  delete [] allciph;

  PG_RETURN_BOOL(ret);
}

Datum test_sum_add(PG_FUNCTION_ARGS) {
  int64_t* a0 = (int64_t *) PG_GETARG_INT64(0);
  int64_t a1 = PG_GETARG_INT64(1);
  if (!a0) {
    a0 = new int64_t(0);
  }
  *a0 += a1;
  PG_RETURN_INT64((int64_t)a0);
}

Datum test_sum_final(PG_FUNCTION_ARGS) {
  int64_t* a0 = (int64_t *) PG_GETARG_INT64(0);
  if (!a0) PG_RETURN_INT64(0);
  int64_t r = *a0;
  delete a0;
  PG_RETURN_INT64(r);
}

}

// homomorphic aggregator with builtin hash aggregate support
// TODO: this code is somewhat of a duplicate from the mysql version
// for the time being, it's easier to maintain duplicates instead of
// figuring out how to modularize the common components

struct nokey {};

static inline std::ostream&
operator<<(std::ostream& o, const nokey&) {
  o << "<nokey>";
  return o;
}

namespace std {
  template <>
  struct hash<nokey> {
    inline size_t operator()(const nokey& e) const { return 0; }
  };
  template <>
  struct equal_to<nokey> {
    inline bool operator()(const nokey& a, const nokey& b) const { return true; }
  };
}

template <typename K0, typename K1>
struct tuple2 {
  tuple2() : k0(), k1() {}
  tuple2(const K0& k0, const K1& k1) : k0(k0), k1(k1) {}
  K0 k0;
  K1 k1;
};

template <typename K0, typename K1>
static std::ostream& operator<<(std::ostream& o, const tuple2<K0, K1>& t) {
  o << "{" << t.k0 << ", " << t.k1 << "}";
  return o;
}

template <typename K0, typename K1>
tuple2<K0, K1> make_tuple2(const K0& k0, const K1& k1) {
  return tuple2<K0, K1>(k0, k1);
}

namespace std {
  template <class K0, class K1>
  struct hash< tuple2<K0, K1> > {
    inline size_t operator()(const tuple2<K0, K1>& e) const {
      return hash<K0>()(e.k0) ^ hash<K1>()(e.k1);
    }
  };
  template <class K0, class K1>
  struct equal_to< tuple2<K0, K1> > {
    inline bool operator()(
        const tuple2<K0, K1>& a,
        const tuple2<K0, K1>& b) const {
      return equal_to<K0>()(a.k0, b.k0) &&
             equal_to<K1>()(a.k1, b.k1);
    }
  };
}

template <>
struct serializer<nokey> {
  static void write(std::ostream&, const nokey&) {}
};

template <typename K0, typename K1>
struct serializer< tuple2<K0, K1> > {
  static void write(std::ostream& o, const tuple2<K0, K1>& t) {
    serializer<K0>::write(o, t.k0);
    serializer<K1>::write(o, t.k1);
  }
};

// this class keeps the state of the aggregate which is independent of the
// argument types. this includes things like internal file buffers, etc
class agg_static_state {
public:

  // buffer is large to optimizer for sequential scans
  static const size_t NumBlocksInternalBuffer = 524288; // roughly 128MB buffer

  // stats
  struct stats {
    stats() : seeks(0), mults(0), all_pos(0), masks_sum(0), masks_cnt(0) {}
    uint32_t seeks;
    uint32_t mults;
    uint32_t all_pos;

    uint32_t masks_sum;
    uint32_t masks_cnt;
  };

  agg_static_state(
      const std::string& key,
      const std::string& filename,
      size_t agg_size,
      size_t rows_per_agg)
    : filename(filename), agg_size(agg_size), rows_per_agg(rows_per_agg),
      _start_time_us(cur_usec()),
      _fp(NULL), _internal_buffer(NULL), _internal_block_id(-1) {

    // public key
    mpz_init(public_key);
    MPZFromBytes(public_key, (const uint8_t *) key.data(), key.size());

    // debug crap
    _debug_stream.open("/tmp/debug.txt");
    assert(_debug_stream.good());

    // file
    _fp = fopen(filename.c_str(), "rb");
    assert(_fp);

    // internal buffer
    _internal_buffer = (char *) malloc(agg_size * NumBlocksInternalBuffer);
    assert(_internal_buffer);

    // TODO: eagerly load the first block?
  }

  ~agg_static_state() {
    mpz_clear(public_key);

    _debug_stream.flush();
    _debug_stream.close();

    fclose(_fp);

    free(_internal_buffer);
  }

  mpz_t public_key;
  const std::string filename;

  const size_t agg_size;
  const size_t rows_per_agg;

  inline std::ostream& debug_stream() { return _debug_stream; }

  stats st;

  // the interface

  // returns a READ ONLY pointer to the block at the given index the returned
  // pointer is valid only until the next call to get_block.
  //
  // WARNING: assumes that the block idx is a valid idx in the file (behavior undefined
  // if idx invalid)
  const char* get_block(size_t idx) {

    DBGOUT( debug_stream() << "get_block(" << idx << ")" << std::endl );

    // TODO: test this impl VS mmap() implementation

    if (LIKELY( is_internal_buffer_valid() )) {
      DBGOUT( debug_stream() << "  buffer is valid" << std::endl );
      if (LIKELY( internal_block_id() <= idx && idx < last_internal_block_id_excl() )) {
        DBGOUT( debug_stream() << "  answered by fast path" << std::endl );
        // fast path- if the block is already located in the _internal_buffer
        return _internal_buffer + block_id_to_file_offset(idx - internal_block_id());
      }
    }

    DBGOUT( debug_stream() << "  answering by slow path" << std::endl );

    // slow path- seek the _fp to block idx and read in an entire block
    long int pos = ftell(_fp); // TODO: how expensive is ftell?
    assert(pos != -1);

    if (size_t(pos) != block_id_to_file_offset(idx)) {
      DBGOUT( debug_stream() << "  answering with seek" << std::endl );
      st.seeks++;
      int ret = fseek(_fp, block_id_to_file_offset(idx), SEEK_SET);
      if (ret) assert(false);
    }

    size_t read = fread(_internal_buffer, agg_size, NumBlocksInternalBuffer, _fp);
    if (read != NumBlocksInternalBuffer) {
      // check EOF
      assert(feof(_fp));
    }
    _internal_block_id = idx;
    return _internal_buffer;
  }

  inline double millis_alive() const {
    return double(cur_usec() - _start_time_us) / 1000.0;
  }

private:

  inline size_t block_id_to_file_offset(size_t idx) const { return idx * agg_size; }

  inline size_t internal_block_id() const {
    SANITY(_internal_block_id >= 0);
    return size_t(_internal_block_id);
  }

  inline bool is_internal_buffer_valid() const { return _internal_block_id != -1; }

  // assumes _internal_block_id is valid
  inline size_t last_internal_block_id_excl() const {
    SANITY(_internal_block_id >= 0);
    return internal_block_id() + NumBlocksInternalBuffer;
  }

  std::ofstream _debug_stream;

  const uint64_t _start_time_us;

  // TODO: buffered vs unbuffered IO, which is better here?
  FILE* _fp;

  // the file is represented as a sequence of aggs. we call each agg a block,
  // thus each block has block_size = agg_size.

  char* _internal_buffer; // allocated buffer which is the unit of transfer
                          // from the file to memory. must free

  ssize_t _internal_block_id; // the beginning block that _internal_buffer points to,
                              // or -1 if no blocks read into _internal_buffer

};

template <typename Key>
struct worker_msg {
  worker_msg(const uint8_t* p, size_t s) {
    mpz_init2(value, s * 8);
    MPZFromBytes(value, p, s);
  }

  ~worker_msg() {
    mpz_clear(value);
  }

  struct ent {
    Key group_key;
    size_t group_id;
    size_t idx;
  };
  std::vector< ent > ents;

  mpz_t value; // has ownership
};

template <typename Key> class agg_typed_state;

template <typename Key>
struct worker_state {
  std::unordered_map<
    Key,
    std::vector< std::vector< mpz_wrapper > > > group_map;

  tbb::concurrent_bounded_queue< worker_msg<Key>* > q;

  agg_typed_state<Key>* typed_state;
};

struct per_group_state {
  std::vector< std::vector< mpz_wrapper > > running_sums_mp;
  uint64_t count;
};

// this contains the threadpool
// key must be something hashable by std::unordered_map
template <typename Key>
class agg_typed_state {
public:
  static const size_t NThreads = 8;

  /** Gives us about a 16MB element buffer */
  static const size_t BufferSize = 1024 * 1024;

  static void* agg_worker_main(void *p) {
    worker_state<Key>* ctx = (worker_state<Key> *) p;
    worker_msg<Key>* m;
    while (true) {
      ctx->q.pop(m);
      if (UNLIKELY(m == NULL)) break;
      else {
        for (auto ent_it = m->ents.begin(); ent_it != m->ents.end(); ++ent_it) {
          auto it = ctx->group_map.find(ent_it->group_key);
          std::vector< std::vector< mpz_wrapper > >* v = NULL;
          if (UNLIKELY(it == ctx->group_map.end())) {
            // init it
            v = &ctx->group_map[ent_it->group_key]; // create
            v->resize(ctx->typed_state->_num_groups);
            for (size_t i = 0; i < ctx->typed_state->_num_groups; i++) {
              InitMPZRunningSums(
                  ctx->typed_state->_static_state->rows_per_agg,
                  v->operator[](i));
            }
          } else {
            v = &it->second;
          }
          std::vector<mpz_wrapper>& vr = v->operator[](ent_it->group_id);
          mpz_mul(vr[ent_it->idx].mp, vr[ent_it->idx].mp, m->value);
          mpz_mod(vr[ent_it->idx].mp, vr[ent_it->idx].mp, ctx->typed_state->_static_state->public_key);
        }
        delete m;
      }
    }
    return NULL;
  }

  agg_typed_state(
      agg_static_state* static_state,
      size_t num_groups)
    : _static_state(static_state), _workerCnt(0), _num_groups(num_groups) {

    assert(static_state);
    assert(num_groups <= 64); // current limitation

    _workers.resize(NThreads);
    _worker_states.resize(NThreads);
  }

  ~agg_typed_state() {
    delete _static_state;
  }

  // init so we can pass this pointer safely
  void init() {
    // initialize threads
    for (size_t i = 0; i < NThreads; i++) {
      _worker_states[i].typed_state = this;
      int ret = pthread_create(&_workers[i], NULL, agg_worker_main, &_worker_states[i]);
      if (ret) assert(false);
    }
  }

  void offer(const Key& key, uint64_t row_id, uint64_t group_bitmask) {

    DBGOUT( _static_state->debug_stream() << "offered row_id " << row_id << std::endl );

    // insert key
    auto it = _element_buffer.find(key);
    std::vector< std::pair<uint64_t, uint64_t> >* elem_buffer = NULL;
    if (UNLIKELY(it == _element_buffer.end())) {
      elem_buffer = &_element_buffer[key];
      elem_buffer->reserve(BufferSize);
    } else {
      elem_buffer = &it->second;
    }

    // buffer the element
    if (group_bitmask) {
      elem_buffer->push_back(std::make_pair(row_id, group_bitmask));
    }

    // if per group buffer is full, then flush all group buffers
    if (UNLIKELY(elem_buffer->size() >= BufferSize)) {
      flush_group_buffers();
      SANITY(elem_buffer->empty());
    }
  }

  // puts the response to return to the client in buffer
  void compute_final_answer_and_serialize(std::string& buffer) {

    flush_group_buffers();

    for (size_t i = 0; i < NThreads; i++) {
      // signal all threads to stop
      _worker_states[i].q.push(NULL);
    }

    for (size_t i = 0; i < NThreads; i++) {
      // wait until computation is done
      pthread_join(_workers[i], NULL);
    }

    for (size_t i = 0; i < NThreads; i++) {
      // merge (aggregate) results from workers into main results
      worker_state<Key>& s = _worker_states[i];
      for (auto it = s.group_map.begin(); it != s.group_map.end(); ++it) {
        assert( it->second.size() == _num_groups );
        for (size_t group_id = 0; group_id < _num_groups; group_id++) {
          std::vector<mpz_wrapper>& v =
            _aggs[it->first].running_sums_mp[group_id];
          assert( v.size() == it->second[group_id].size() );
          for (size_t i = 0; i < v.size(); i++) {
            mpz_mul(v[i].mp, v[i].mp, it->second[group_id][i].mp);
            mpz_mod(v[i].mp, v[i].mp, _static_state->public_key);
          }
        }
      }
    }

    std::unordered_map< Key, std::vector< size_t > > non_zero_m;
    for (auto it = _aggs.begin(); it != _aggs.end(); ++it) {
      std::vector<size_t>& nn = non_zero_m[it->first];
      nn.resize(_num_groups);
      for (size_t i = 0; i < _num_groups; i++) {
        size_t non_zero = 0;
        size_t s = it->second.running_sums_mp[i].size();
        assert(s == size_t((0x1 << _static_state->rows_per_agg) - 1));
        for (size_t idx = 0; idx < s; idx++) {
          if (mpz_cmp_ui(it->second.running_sums_mp[i][idx].mp, 1)) non_zero++;
        }
        nn[i] = non_zero;
      }
    }

    std::ostringstream buf;

    // write headers, to make client side decryption easier
    serializer<uint32_t>::write(buf, _static_state->agg_size);
    serializer<uint32_t>::write(buf, _static_state->rows_per_agg);

    for (auto it = _aggs.begin(); it != _aggs.end(); ++it) {
      serializer<Key>::write(buf, it->first);
      serializer<uint64_t>::write(buf, it->second.count);

      std::vector<size_t>& nn = non_zero_m[it->first];
      for (size_t i = 0; i < _num_groups; i++) {
        serializer<uint32_t>::write(buf, nn[i]);

        size_t s = it->second.running_sums_mp[i].size();
        for (size_t idx = 0; idx < s; idx++) {
          if (!mpz_cmp_ui(it->second.running_sums_mp[i][idx].mp, 1)) {
            continue; // test usefullness
          }

          serializer<uint32_t>::write(buf, idx+1);

          std::string b;
          b.resize(_static_state->agg_size);
          BytesFromMPZ(
              (uint8_t*)&b[0],
              it->second.running_sums_mp[i][idx].mp,
              _static_state->agg_size);
          buf << b; // write w/o string length
        }
      }
    }

    // need to clear worker states
    for (size_t i = 0; i < NThreads; i++) {
      worker_state<Key>& s = _worker_states[i];
      for (auto it = s.group_map.begin(); it != s.group_map.end(); ++it) {
        for (auto it0 = it->second.begin(); it0 != it->second.end(); ++it0) {
          for (auto it1 = it0->begin(); it1 != it0->end(); ++it1) {
            mpz_clear(it1->mp);
          }
        }
      }
    }

    buffer = buf.str();
  }

  inline agg_static_state* static_state() { return _static_state; };

private:

  // flush _element_buffer
  void flush_group_buffers() {
    using namespace std;

    typedef
      map
      < uint64_t /*block_id*/,
        vector< vector< uint32_t > > /* masks (per user group) */
      > control_group;

    // value is ordered list based on the pair's first element
    typedef
      vector < pair < Key /*agg group_id*/, control_group > > allocation_map;
    allocation_map alloc_map;

    alloc_map.resize(_element_buffer.size());

    size_t eit_pos = 0;
    for (auto eit = _element_buffer.begin();
        eit != _element_buffer.end(); ++eit, ++eit_pos) {
      // lookup agg group
      auto it = _aggs.find(eit->first);
      per_group_state* s = NULL;
      if (UNLIKELY(it == _aggs.end())) {
        s = &_aggs[eit->first]; // creates on demand
        s->running_sums_mp.resize(_num_groups);
        for (size_t i = 0; i < _num_groups; i++) {
          InitMPZRunningSums(_static_state->rows_per_agg, s->running_sums_mp[i]);
        }
        s->count = 0;
      } else {
        s = &it->second;
      }
      s->count += eit->second.size();

      alloc_map[eit_pos].first = eit->first;
      control_group &m = alloc_map[eit_pos].second;

      DBGOUT(
        _static_state->debug_stream() <<
          "going over " << eit->second.size() <<
          " buffered entries for group: " << eit->first << std::endl );

      //timer t0;
      for (vector< pair< uint64_t, uint64_t > >::iterator it = eit->second.begin();
          it != eit->second.end(); ++it) {
        uint64_t row_id = it->first;
        uint64_t mask   = it->second;
        //assert(mask != 0); // otherwise would not be here...

        // compute block ID from row ID
        uint64_t block_id = row_id / _static_state->rows_per_agg;
        // compute block offset
        uint64_t block_offset = row_id % _static_state->rows_per_agg;

        vector< vector< uint32_t > >&v = m[block_id]; // create on demand
        if (v.empty()) v.resize(_num_groups);

        for (size_t i = 0; i < _num_groups; i++) {
          if (!(mask & (0x1 << i))) continue;
          vector<uint32_t>& masks = v[i];
          if (masks.empty()) {
            masks.push_back(0x1 << block_offset);
          } else {
            // search for empty spot w/ linear scan
            bool found = false;
            for (vector<uint32_t>::iterator it = masks.begin();
                it != masks.end(); ++it) {
              if (!((*it) & (0x1 << block_offset))) {
                *it |= 0x1 << block_offset;
                found = true;
                break;
              }
            }
            if (!found) masks.push_back(0x1 << block_offset);
          }
        }
      }
      eit->second.clear(); // clear buffer
    }

    typedef vector
      <
      pair< bool /*done?*/, control_group::iterator /* iterator to current group (if not done) */ >
      > group_done_vec; // keeps track of which groups are done
    group_done_vec group_done;
    group_done.resize(alloc_map.size());

    size_t pos_first_non_empty = 0;
    size_t alloc_map_pos = 0;
    for (auto it = alloc_map.begin(); it != alloc_map.end(); ++it, ++alloc_map_pos) {
      assert(!it->second.empty());
      group_done[alloc_map_pos].first  = false;
      group_done[alloc_map_pos].second = it->second.begin();
    }

    while (pos_first_non_empty < alloc_map.size()) {
      // find min block id for all groups
      uint64_t minSoFar = group_done[pos_first_non_empty].second->first; // block_id
      for (size_t i = pos_first_non_empty + 1; i < group_done.size(); i++) {
        pair< bool, control_group::iterator > &group = group_done[i];
        if (!group.first /* not done */ &&
            group.second->first /* block_id */ < minSoFar) {
          minSoFar = group.second->first;
        }
      }

      const uint8_t* block_ptr =
        (const uint8_t *) _static_state->get_block(minSoFar);

      // service the requests
      worker_msg<Key>* m = new worker_msg<Key>(block_ptr, _static_state->agg_size);

      for (size_t i = pos_first_non_empty; i < group_done.size(); i++) {
        pair< bool, control_group::iterator > &group = group_done[i];
        if (group.first /* done */ ||
            group.second->first /* block_id */ != minSoFar) continue;

        Key& group_key = alloc_map[i].first;

        // push all these multiplications into the queue
        size_t group_id_idx = 0;
        for (auto it = group.second->second.begin();
            it != group.second->second.end();
            ++it, ++group_id_idx) {

          for (auto it0 = it->begin(); it0 != it->end(); ++it0) {

            typename worker_msg<Key>::ent e;
            e.group_key = group_key;
            e.group_id  = group_id_idx;
            e.idx       = (*it0) - 1;
            m->ents.push_back(e);

            // stats
            _static_state->st.mults++;
            //static const uint32_t AllPosHigh = (0x1 << _static_state->rows_per_agg) - 1;
            //if (group.second.front().mask == AllPosHigh) as->st.all_pos++;
          }
        }

        // advance this group

        group.second++;
        group.first = group.second == alloc_map[i].second.end();
      }

      _worker_states[ _workerCnt++ % NThreads ].q.push(m);

      // find first non-empty
      for (group_done_vec::iterator it = group_done.begin() + pos_first_non_empty;
          it != group_done.end(); ++it) {
        if (it->first /*done*/) pos_first_non_empty++;
        else break;
      }
    }
  }

  agg_static_state* _static_state; // ownership

  std::vector< pthread_t > _workers;
  std::vector< worker_state<Key> > _worker_states;
  size_t _workerCnt; // for RR-ing

  typedef std::unordered_map<Key, per_group_state> group_map;
  group_map _aggs; // running aggregate state
  size_t _num_groups;

  // internal element buffer
  typedef std::unordered_map<
    Key,
    std::vector<
      std::pair< uint64_t, uint64_t > /* (row_id, group_id) */
    > > element_buffer_map;
  element_buffer_map _element_buffer;

};

template <typename Key>
static Datum agg_hash_finalizer(PG_FUNCTION_ARGS) {
  agg_typed_state<Key>* state = (agg_typed_state<Key>*) PG_GETARG_INT64(0);
  assert(state);
  std::string buf;
  state->compute_final_answer_and_serialize(buf);

  DBGOUT(
    state->static_state()->debug_stream() <<
      "finished agg in " << state->static_state()->millis_alive() <<
      " ms" << std::endl );

  DBGOUT(
    state->static_state()->debug_stream() <<
      "...return buf is " << buf.size() << " bytes" << std::endl );

  delete state;
  // TODO: eliminate redundant buffer copy
  PG_RETURN_BYTEA_P(copy_to_pg_return_buffer(buf));
}

template <typename Key>
static agg_typed_state<Key>* extract_state(PG_FUNCTION_ARGS, size_t n_groups) {
  agg_typed_state<Key>* state = (agg_typed_state<Key>*) PG_GETARG_INT64(0);
  if (UNLIKELY(state == NULL)) {
    state = new agg_typed_state<Key>(
      new agg_static_state(
        getstring_fromba(PG_PASS_FARGS, 1),
        getvarchar(PG_PASS_FARGS, 2),
        PG_GETARG_INT64(3),
        PG_GETARG_INT64(4)),
      n_groups
    );
    state->init();

    DBGOUT( state->static_state()->debug_stream() << "started agg" << std::endl );
  }
  return state;
}

#define AGG_ROW_ID_IDX 5
#define AGG_ROW_ID()   PG_GETARG_INT64(AGG_ROW_ID_IDX)

template <typename Key>
Datum agg_hash_state_transition_impl(PG_FUNCTION_ARGS, const Key& key) {
  agg_typed_state<Key>* state = extract_state<Key>(PG_PASS_FARGS, 1);
  if (!PG_ARGISNULL(AGG_ROW_ID_IDX)) {
    // if row id is null, we take that to mean exclude the row.
    // this is a fast way to implement addition by 0
    state->offer(key, AGG_ROW_ID(), 0x1);
  } else {
    DBGOUT(
      state->static_state()->debug_stream() <<
        "was offered NULL row_id" << std::endl );
  }
  PG_RETURN_INT64(state);
}

// avoid commans in passing macro args
typedef tuple2<uint64_t, uint64_t> tuple2_u64_u64;

extern "C" {

// create function agg_hash_nokey_state_transition
//  (bigint, bytea, varchar, bigint, bigint, bigint)
//    returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function agg_hash_nokey_finalizer(bigint)
//    returns bytea language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate agg_hash
//  (bytea, varchar, bigint, bigint, bigint)
//  (
//    sfunc=agg_hash_nokey_state_transition,
//    stype=bigint,
//    finalfunc=agg_hash_nokey_finalizer,
//    initcond='0'
//  );
//
// create function agg_hash_bytea_state_transition
//  (bigint, bytea, varchar, bigint, bigint, bigint, bytea)
//    returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function agg_hash_bytea_finalizer(bigint)
//    returns bytea language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate agg_hash
//  (bytea, varchar, bigint, bigint, bigint, bytea)
//  (
//    sfunc=agg_hash_bytea_state_transition,
//    stype=bigint,
//    finalfunc=agg_hash_bytea_finalizer,
//    initcond='0'
//  );
//
// create function agg_hash_u64_u64_state_transition
//  (bigint, bytea, varchar, bigint, bigint, bigint, bigint, bigint)
//    returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function agg_hash_u64_u64_finalizer(bigint)
//    returns bytea language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate agg_hash
//  (bytea, varchar, bigint, bigint, bigint, bigint, bigint)
//  (
//    sfunc=agg_hash_u64_u64_state_transition,
//    stype=bigint,
//    finalfunc=agg_hash_u64_u64_finalizer,
//    initcond='0'
//  );

#define DECL_AGG_HASH_ARG0_FCNS(cpp_type) \
  Datum agg_hash_## cpp_type ##_state_transition(PG_FUNCTION_ARGS); \
  Datum agg_hash_## cpp_type ##_finalizer(PG_FUNCTION_ARGS); \
  PG_FUNCTION_INFO_V1(agg_hash_## cpp_type ##_state_transition); \
  PG_FUNCTION_INFO_V1(agg_hash_## cpp_type ##_finalizer); \
  Datum agg_hash_## cpp_type ##_finalizer(PG_FUNCTION_ARGS) { \
    return agg_hash_finalizer< cpp_type >(PG_PASS_FARGS); \
  }

#define DECL_AGG_HASH_ARG1_FCNS(cpp_type, k0) \
  Datum agg_hash_## k0 ##_state_transition(PG_FUNCTION_ARGS); \
  Datum agg_hash_## k0 ##_finalizer(PG_FUNCTION_ARGS); \
  PG_FUNCTION_INFO_V1(agg_hash_## k0 ##_state_transition); \
  PG_FUNCTION_INFO_V1(agg_hash_## k0 ##_finalizer); \
  Datum agg_hash_## k0 ##_finalizer(PG_FUNCTION_ARGS) { \
    return agg_hash_finalizer< cpp_type >(PG_PASS_FARGS); \
  }

#define DECL_AGG_HASH_ARG2_FCNS(cpp_type, k0, k1) \
  Datum agg_hash_## k0 ## _ ## k1 ##_state_transition(PG_FUNCTION_ARGS); \
  Datum agg_hash_## k0 ## _ ## k1 ##_finalizer(PG_FUNCTION_ARGS); \
  PG_FUNCTION_INFO_V1(agg_hash_## k0 ## _ ## k1 ##_state_transition); \
  PG_FUNCTION_INFO_V1(agg_hash_## k0 ## _ ## k1 ##_finalizer); \
  Datum agg_hash_## k0 ## _ ## k1 ##_finalizer(PG_FUNCTION_ARGS) { \
    return agg_hash_finalizer< cpp_type >(PG_PASS_FARGS); \
  }


DECL_AGG_HASH_ARG0_FCNS(nokey);
DECL_AGG_HASH_ARG1_FCNS(std::string, bytea);
DECL_AGG_HASH_ARG2_FCNS(tuple2_u64_u64, u64, u64);

// args ( for N-col group key ):
//
// <state_ptr>,
// public_key (bytea),
// filename (varchar),
// agg_size (int64),
// rows_per_agg (int64),
// row_id (int64),
// key0,
// ...,
// keyN-1

#define ARG0_IDX 6
#define ARG1_IDX 7

Datum agg_hash_nokey_state_transition(PG_FUNCTION_ARGS) {
  return agg_hash_state_transition_impl(PG_PASS_FARGS, nokey());
}

Datum agg_hash_bytea_state_transition(PG_FUNCTION_ARGS) {
  return agg_hash_state_transition_impl(
      PG_PASS_FARGS,
      getstring_fromba(PG_PASS_FARGS, ARG0_IDX));
}

Datum agg_hash_u64_u64_state_transition(PG_FUNCTION_ARGS) {
  uint64_t k0 = (uint64_t) PG_GETARG_INT64(ARG0_IDX);
  uint64_t k1 = (uint64_t) PG_GETARG_INT64(ARG1_IDX);
  return agg_hash_state_transition_impl(PG_PASS_FARGS, make_tuple2(k0, k1));
}

Datum agg_ident_i64_state_transition(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(agg_ident_i64_state_transition);

// create function agg_ident_i64_state_transition(bigint, bigint)
//    returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate agg_ident(bigint)
//  (
//    sfunc=agg_ident_i64_state_transition,
//    stype=bigint,
//    initcond='0'
//  );
Datum agg_ident_i64_state_transition(PG_FUNCTION_ARGS) {
  // always return the incoming value
  PG_RETURN_INT64(PG_GETARG_INT64(1));
}

Datum fast_sum_i64_state_transition(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(fast_sum_i64_state_transition);

// create function fast_sum_i64_state_transition(bigint, bigint)
//    returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate fast_sum(bigint)
//  (
//    sfunc=fast_sum_i64_state_transition,
//    stype=bigint,
//    initcond='0'
//  );
Datum fast_sum_i64_state_transition(PG_FUNCTION_ARGS) {
  PG_RETURN_INT64(PG_GETARG_INT64(0) + PG_GETARG_INT64(1));
}

}

// port of parallel hom agg functions from mysql

struct basic_hom_agg {
private:
  struct worker_msg {
    worker_msg(const uint8_t* p, size_t s) {
      mpz_init2(value, s * 8);
      MPZFromBytes(value, p, s);
    }

    ~worker_msg() {
      mpz_clear(value);
    }

    mpz_t value; // has ownership
  };

  struct worker_state {
    worker_state() : global_state(NULL) {
      init_and_set_mpz_for_paillier(agg);
    }
    ~worker_state() { mpz_clear(agg); }
    mpz_t agg;
    tbb::concurrent_bounded_queue<worker_msg*> q;
    basic_hom_agg* global_state; // no ownership
  };

  // inits to 1 w/ adequate space reserved
  static void init_and_set_mpz_for_paillier(mpz_t rop) {
    mpz_init2(rop, 256 * 8);
    mpz_set_ui(rop, 1);
  }

public:
  static const size_t NThreads = 8;

  basic_hom_agg(const std::string& key) : _workerCnt(0) {
    mpz_init2(public_key, key.size() * 8);
    MPZFromBytes(public_key, (const uint8_t *) key.data(), key.size());

    _workers.resize(NThreads);
    _worker_states.resize(NThreads);
  }

  ~basic_hom_agg() {
    mpz_clear(public_key);
  }

  void init() {
    for (size_t i = 0; i < NThreads; i++) {
      _worker_states[i].global_state = this;
      int ret = pthread_create(&_workers[i], NULL, worker_main, &_worker_states[i]);
      if (ret) assert(false);
    }
  }

  void offer(const std::string& v) {
    worker_msg* m = new worker_msg((const uint8_t *) v.data(), v.size());
    _worker_states[_workerCnt++ % NThreads].q.push(m);
  }

  void compute_final_answer_and_serialize(std::string& buffer) {
    for (size_t i = 0; i < NThreads; i++) {
      _worker_states[i].q.push(NULL);
    }
    for (size_t i = 0; i < NThreads; i++) {
      pthread_join(_workers[i], NULL);
    }

    mpz_t agg;
    init_and_set_mpz_for_paillier(agg);
    for (size_t i = 0; i < NThreads; i++) {
      mpz_mul(agg, agg, _worker_states[i].agg);
      mpz_mod(agg, agg, public_key);
    }

    buffer.clear();
    buffer.resize(256);
    BytesFromMPZ((uint8_t*)&buffer[0], agg, 256);
  }

  static basic_hom_agg* extract_state(PG_FUNCTION_ARGS) {
    basic_hom_agg* state = (basic_hom_agg*) PG_GETARG_INT64(0);
    if (UNLIKELY(state == NULL)) {
      state = new basic_hom_agg(getstring_fromba(PG_PASS_FARGS, 1));
      state->init();
    }
    return state;
  }

private:

  mpz_t public_key;

  std::vector< pthread_t > _workers;
  std::vector< worker_state > _worker_states;
  size_t _workerCnt;

  static void* worker_main(void* p) {
    worker_state* ctx = (worker_state *) p;
    worker_msg* m;
    while (true) {
      ctx->q.pop(m);
      if (UNLIKELY(m == NULL)) break;
      else {
        mpz_mul(ctx->agg, ctx->agg, m->value);
        mpz_mod(ctx->agg, ctx->agg, ctx->global_state->public_key);
        delete m;
      }
    }
    return NULL;
  }
};

extern "C" {

// create function hom_agg_state_transition
//  (bigint, bytea, bytea)
//    returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function hom_agg_finalizer(bigint)
//    returns bytea language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate hom_agg
//  (bytea, bytea)
//  (
//    sfunc=hom_agg_state_transition,
//    stype=bigint,
//    finalfunc=hom_agg_finalizer,
//    initcond='0'
//  );

Datum hom_agg_state_transition(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(hom_agg_state_transition);

Datum hom_agg_state_transition(PG_FUNCTION_ARGS) {
  basic_hom_agg* state = basic_hom_agg::extract_state(PG_PASS_FARGS);
  if (!PG_ARGISNULL(2)) {
    state->offer(getstring_fromba(PG_PASS_FARGS, 2));
  }
  PG_RETURN_INT64(state);
}

Datum hom_agg_finalizer(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(hom_agg_finalizer);

Datum hom_agg_finalizer(PG_FUNCTION_ARGS) {
  basic_hom_agg* state = basic_hom_agg::extract_state(PG_PASS_FARGS);
  std::string buf;
  state->compute_final_answer_and_serialize(buf);
  delete state;
  PG_RETURN_BYTEA_P(copy_to_pg_return_buffer(buf));
}

// simplest serializer: encodes each uint64 in a fixed-width 8-byte slot
class u64_agg_serializer {
public:
  inline void offer(uint64_t v) { serializer<uint64_t>::write(_buf, v); }
  std::string finalize() { return _buf.str(); }
  static u64_agg_serializer* extract_state(PG_FUNCTION_ARGS) {
    u64_agg_serializer* state = (u64_agg_serializer*) PG_GETARG_INT64(0);
    if (UNLIKELY(state == NULL)) state = new u64_agg_serializer;
    return state;
  }
private:
  std::ostringstream _buf;
};

// more complex serializer: sorts the integers and delta compresses
// them
class u64_varint_agg_serializer {
public:
  inline void offer(uint64_t v) { _vs.push_back(v); }
  inline std::string finalize() {
    std::sort(_vs.begin(), _vs.end());
    std::ostringstream buf;
    varint_serializer::delta_encode_uvint64_vector(buf, _vs);
    return buf.str();
  }
  static u64_varint_agg_serializer* extract_state(PG_FUNCTION_ARGS) {
    u64_varint_agg_serializer* state = (u64_varint_agg_serializer*) PG_GETARG_INT64(0);
    if (UNLIKELY(state == NULL)) state = new u64_varint_agg_serializer;
    return state;
  }
private:
  std::vector<uint64_t> _vs;
};

// dictionary encoder: caller supplies a dictionary of known numbers.
// encoding is as follows:
// [ uvint32 num_dict_hits | [fixed width uvint32 array of hits] | [delta compressed uvint64 array with remaining elems]]

typedef std::unordered_map<uint64_t, uint32_t> dict_lookup_table;

// TODO: per-connection lookup manager, which is why we don't bother with
// making it thread-safe (so we don't inherit the overheads of locking)
class lookup_table_manager {
public:
  const dict_lookup_table* get_table(uint64_t table_id) const {
    auto it = _tables.find(table_id);
    if (it == _tables.end()) return NULL;
    return it->second;
  }

  // takes ownership
  void insert_table(uint64_t table_id, dict_lookup_table* table) {
    // TODO: make sure we aren't overriding anything
    _tables[table_id] = table;
  }

  ~lookup_table_manager() {
    for (auto kv : _tables) delete kv.second;
  }

private:
  std::unordered_map<uint64_t, dict_lookup_table*> _tables;
} LookupTableMgr;

class u64_dict_agg_serializer {
public:
  u64_dict_agg_serializer(const std::unordered_map<uint64_t, uint32_t>* lookup)
    : _lookup(lookup) {
    assert(lookup != NULL);
  }

  inline void offer(uint64_t v) {
    auto it = _lookup->find(v);
    if (it != _lookup->end()) {
      _hits.push_back(it->second);
    } else {
      _misses.push_back(v);
    }
  }

  std::string finalize() {
    std::ostringstream buf;
    varint_serializer::write_uvint32(buf, _hits.size());
    for (auto h : _hits) varint_serializer::write_uvint32(buf, h);
    varint_serializer::delta_encode_uvint64_vector(buf, _misses);
    return buf.str();
  }

  static u64_dict_agg_serializer* extract_state(PG_FUNCTION_ARGS) {
    u64_dict_agg_serializer* state = (u64_dict_agg_serializer*) PG_GETARG_INT64(0);
    if (UNLIKELY(state == NULL)) {
      state = new u64_dict_agg_serializer(LookupTableMgr.get_table(PG_GETARG_INT64(2)));
    }
    return state;
  }

private:
  // lookup element -> idx in dictionary
  const std::unordered_map<uint64_t, uint32_t>* const _lookup;
  std::vector<uint32_t> _hits;
  std::vector<uint64_t> _misses;
};

extern "C" {

Datum insert_dict_table(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(insert_dict_table);

// create function insert_dict_table
//  (bigint, bigint[])
//    returns bool language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
Datum insert_dict_table(PG_FUNCTION_ARGS) {
  uint64_t id = PG_GETARG_INT64(0);

  // extract bigint array
  // most of these details come from:
  // http://archives.postgresql.org/pgsql-novice/2008-02/msg00129.php
  //
  // there's really no proper documentation on how to extract an array
  // in a C UDF, other than reading source code.

  ArrayType* input;
  Datum* i_data;
  bool* nulls;
  Oid i_eltype;
  int16_t i_typlen;
  bool i_typbyval;
  char i_typalign;
  int i, n;

  input = PG_GETARG_ARRAYTYPE_P(1);
  i_eltype = ARR_ELEMTYPE(input);
  get_typlenbyvalalign(i_eltype, &i_typlen, &i_typbyval, &i_typalign);

  deconstruct_array(
      input, i_eltype, i_typlen, i_typbyval, i_typalign,
      &i_data, &nulls, &n);

  std::vector<uint64_t> vs;
  vs.reserve(n);

  for (i = 0; i < n; i++) vs.push_back(DatumGetInt64(i_data[i]));

  pfree(i_data);
  pfree(nulls);

  dict_lookup_table* tbl = new dict_lookup_table;
  for (uint32_t idx = 0; idx < vs.size(); idx++) {
    tbl->operator[](vs[idx]) = idx;
  }

  LookupTableMgr.insert_table(id, tbl);
  PG_RETURN_BOOL(true);
}

// create function group_serializer_state_transition
//  (bigint, bigint)
//    returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function group_serializer_finalizer(bigint)
//    returns bytea language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate group_serializer
//  (bigint)
//  (
//    sfunc=group_serializer_state_transition,
//    stype=bigint,
//    finalfunc=group_serializer_finalizer,
//    initcond='0'
//  );

Datum group_serializer_state_transition(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(group_serializer_state_transition);

Datum group_serializer_state_transition(PG_FUNCTION_ARGS) {
  u64_agg_serializer* state =
    u64_agg_serializer::extract_state(PG_PASS_FARGS);
  state->offer((uint64_t)PG_GETARG_INT64(1));
  PG_RETURN_INT64(state);
}

Datum group_serializer_finalizer(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(group_serializer_finalizer);

Datum group_serializer_finalizer(PG_FUNCTION_ARGS) {
  u64_agg_serializer* state =
    u64_agg_serializer::extract_state(PG_PASS_FARGS);
  std::string buf = state->finalize();
  delete state;
  PG_RETURN_BYTEA_P(copy_to_pg_return_buffer(buf));
}

// create function group_serializer_dict_comp_state_transition
//  (bigint, bigint, bigint)
//    returns bigint language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create function group_serializer_dict_comp_finalizer(bigint)
//    returns bytea language C as '/home/stephentu/cryptdb/obj/udf/edb_pg.so';
// create aggregate group_serializer
//  (bigint, bigint)
//  (
//    sfunc=group_serializer_dict_comp_state_transition,
//    stype=bigint,
//    finalfunc=group_serializer_dict_comp_finalizer,
//    initcond='0'
//  );

Datum group_serializer_dict_comp_state_transition(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(group_serializer_dict_comp_state_transition);

Datum group_serializer_dict_comp_state_transition(PG_FUNCTION_ARGS) {
  u64_dict_agg_serializer* state =
    u64_dict_agg_serializer::extract_state(PG_PASS_FARGS);
  state->offer((uint64_t)PG_GETARG_INT64(1));
  PG_RETURN_INT64(state);
}

Datum group_serializer_dict_comp_finalizer(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(group_serializer_dict_comp_finalizer);

Datum group_serializer_dict_comp_finalizer(PG_FUNCTION_ARGS) {
  u64_dict_agg_serializer* state =
    u64_dict_agg_serializer::extract_state(PG_PASS_FARGS);
  std::string buf = state->finalize();
  delete state;
  PG_RETURN_BYTEA_P(copy_to_pg_return_buffer(buf));
}

}

}
