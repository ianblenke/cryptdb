#include <cstdio>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <unordered_map>
#include <set>

#include <parser/cdb_helpers.hh>

#include <util/stl.hh>

#include <execution/common.hh>
#include <execution/encryption.hh>
#include <execution/operator_types.hh>
#include <execution/query_cache.hh>

using namespace std;

#define TO_C(s) (to_s(s).c_str())

#define TRACE_OPERATOR() \
  do { \
    static size_t _n = 0; \
    _n++; \
    dprintf("called %s times\n", TO_C(_n)); \
  } while (0)

void
remote_sql_op::open(exec_context& ctx)
{
  physical_operator::open(ctx);
  // execute all the children as a one-shot
  for (auto c : _children) c->execute(ctx);

  // generate params
  _do_cache_write_sql = _sql;
  if (_param_generator) {
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
                isdigit(_do_cache_write_sql[i])) {
            p << _do_cache_write_sql[i];
            i++;
          }
          string p0 = p.str();
          assert(!p0.empty());
          size_t idx = resultFromStr<size_t>(p0);
          assert(param_map.find(idx) != param_map.end());
          buf << param_map[idx].sqlify(true);
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

  cerr << "executing sql: " << _do_cache_write_sql << endl;

  // look in query cache first
  if (ctx.cache) {
    auto it = ctx.cache->cache.find(_do_cache_write_sql);
    if (it != ctx.cache->cache.end()) {
      _cached_results = it->second;
      assert(_res == NULL); // sets read
      dprintf("cache hit! %s rows\n", TO_C(_cached_results.size()));
      return;
    }
  }

  // open and execute the sql statement
  if (!ctx.connection->execute(_do_cache_write_sql, _res)) {
    throw runtime_error("Could not execute sql");
  }
  assert(_res);

  dprintf("expect %s rows\n", TO_C(_res->size()));
}

void
remote_sql_op::close(exec_context& ctx)
{
  physical_operator::close(ctx);

  // place result in cache if qualifies
  if (_do_cache_write) {
    assert(!_do_cache_write_sql.empty());
    if (ctx.cache) {
      ctx.cache->cache[_do_cache_write_sql] = _cached_results;
    }
    _do_cache_write_sql.clear();
    _do_cache_write = false;
  }

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

  assert(has_more(ctx));

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
        if (_desc_vec[j].is_vector) {

          // TODO: this is broken in general, but we might
          // be able to get away with it for TPC-H. there are a
          // pretty big set of assumptions here that allows this to work
          // properly. we'll fix if we need more flexibility

          // if the underlying type is string (binary), then we encode
          // it in hex. otherwise, if the underlying type is numeric, then
          // we use the ascii repr of the number.
          //
          // gross? very much so! we *really* should be using the binary
          // interface to postgres, but its too much work to change it now
          // (unless this becomes a potential source of inefficiency)

          // array_to_string(...) always produces a string (from postgres's point
          // of view)
          SANITY(db_elem::TypeFromPGOid(col.type) == db_elem::TYPE_STRING);

          // whitelist the cases we are ready to handle (add more if needed)
          SANITY(_desc_vec[j].type == db_elem::TYPE_INT ||
                 _desc_vec[j].type == db_elem::TYPE_DATE ||
                 _desc_vec[j].type == db_elem::TYPE_STRING ||
                 _desc_vec[j].type == db_elem::TYPE_DECIMAL_15_2);

          // simplifying assumption for now (relax if needed)
          SANITY(_desc_vec[j].onion_type == oDET);

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
        } else {
          tuple.columns.push_back(
              CreateFromString(col.data, db_elem::TypeFromPGOid(col.type)));
        }
      }
    }

    // for now, only write to query cache if 0/1 result
    if (_res->size() <= 1) {
      assert(tuples.size() == _res->size());
      _cached_results = tuples;
      _do_cache_write = true;
    }
  } else {
    // read from cache
    assert(_read_cursor == 0);
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
      assert(data == "f" || data == "t");
      return (db_elem(data == "t"));

    case db_elem::TYPE_DOUBLE:
      return (db_elem(resultFromStr<double>(data)));

    case db_elem::TYPE_STRING:
      return (db_elem(data));

    case db_elem::TYPE_DATE:
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

static db_elem
do_decrypt_op(
    exec_context& ctx,
    const db_elem& elem,
    const db_column_desc& d) {
  assert(elem.get_type() != db_elem::TYPE_VECTOR);
  string s = elem.stringify();
  switch (d.type) {
    case db_elem::TYPE_INT: {

      assert(d.size == 1 ||
             d.size == 2 ||
             d.size == 4 ||
             d.size == 8);

      assert(d.onion_type == oDET ||
             d.onion_type == oOPE);

      bool isDet = d.onion_type == oDET;

      switch (d.size) {
        case 1:
          return db_elem((int64_t)
            (isDet ? decrypt_u8_det(ctx.crypto, s, d.pos, d.level) :
                     decrypt_u8_ope(ctx.crypto, s, d.pos, d.level)));
        case 2:
          return db_elem((int64_t)
            (isDet ? decrypt_u16_det(ctx.crypto, s, d.pos, d.level) :
                     decrypt_u16_ope(ctx.crypto, s, d.pos, d.level)));
        case 4:
          return db_elem((int64_t)
            (isDet ? decrypt_u32_det(ctx.crypto, s, d.pos, d.level) :
                     decrypt_u32_ope(ctx.crypto, s, d.pos, d.level)));
        case 8:
          return db_elem((int64_t)
            (isDet ? decrypt_u64_det(ctx.crypto, s, d.pos, d.level) :
                     decrypt_u64_ope(ctx.crypto, s, d.pos, d.level)));
      }

      assert(false);
    }

    case db_elem::TYPE_CHAR: {

      assert(d.size == 1);
      assert(d.onion_type == oDET ||
             d.onion_type == oOPE);

      uint64_t x =
        (d.onion_type == oDET) ?
          decrypt_u8_det(ctx.crypto, s, d.pos, d.level) :
          decrypt_u8_ope(ctx.crypto, s, d.pos, d.level) ;

      assert( x <= numeric_limits<unsigned char>::max() );

      return db_elem(char(x));
    }

    case db_elem::TYPE_STRING: {

      assert(d.onion_type == oDET);
      // NO decryption of OPE onions

      return db_elem(decrypt_string_det(ctx.crypto, s, d.pos, d.level));

    }

    case db_elem::TYPE_DATE: {

      assert(d.size == 3);

      assert(d.onion_type == oDET ||
             d.onion_type == oOPE);

      uint64_t x = (d.onion_type == oDET) ?
        decrypt_date_det(ctx.crypto, s, d.pos, d.level) :
        decrypt_date_ope(ctx.crypto, s, d.pos, d.level) ;

      uint32_t m, d, y;
      extract_date_from_encoding(x, m, d, y);

      //cerr << "date ciphertext: " << s << endl;
      //cerr << "date encoding: " << x << endl;
      //cerr << "date extract: " << y << "-" << m << "-" << d << endl;

      return db_elem(y, m, d);
    }

    // TODO: fix hack...
    case db_elem::TYPE_DECIMAL_15_2: {

      assert(d.onion_type == oDET ||
             d.onion_type == oOPE);

      uint64_t x =
        (d.onion_type == oDET) ?
          decrypt_decimal_det(ctx.crypto, s, d.pos, d.level) :
          decrypt_u64_ope(ctx.crypto, s, d.pos, d.level) ;

      return db_elem(double(x)/100.0);
    }

    default:
      cerr << "got unhandled type(" << d.type << ")" << endl;
      throw runtime_error("UNIMPL");
  }
}

void
local_decrypt_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();

  assert(first_child()->has_more(ctx));
  desc_vec desc = first_child()->tuple_desc();

  first_child()->next(ctx, tuples);

  // three levels of potential parallelism:
  // 0) each row can be decrypted in parallel
  // 1) each column can be decrypted in parallel
  // 2) within a column, vector contexts can be decrypted in parallel
  //
  // for now, (2) is the only one really worth exploiting

#ifdef EXTRA_SANITY_CHECKS
  for (auto p : _pos) {
    if (p >= desc.size()) {
      cerr << "ERROR: p=(" << p << "), desc.size()=(" << desc.size() << ")" << endl;
    }
    assert(p < desc.size());
  }
#endif

  for (size_t i = 0; i < tuples.size(); i++) { // parallelism (0)
    db_tuple& tuple = tuples[i];
    SANITY(tuple.columns.size() == desc.size());

    for (auto p : _pos) { // parallelism (1)
      db_column_desc& d = desc[p];
      db_elem& e = tuple.columns[p];
      if (e.is_vector()) {
        vector< db_elem > elems;
        elems.reserve(e.size());
        for (auto &e0 : e) { // parallelism (2)
          elems.push_back(do_decrypt_op(ctx, e0, d));
        }
        tuple.columns[p] = db_elem(elems);
      } else {
        tuple.columns[p] = do_decrypt_op(ctx, e, d);
      }
    }
  }
}

void
local_filter_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  TRACE_OPERATOR();

  assert(first_child()->has_more(ctx));
  db_tuple_vec v;
  first_child()->next(ctx, v);
  for (auto &tuple : v) {
    exec_context eval_ctx = ctx.bind(&tuple, _subqueries);
    db_elem test = _filter->eval(eval_ctx);
    assert(test.get_type() == db_elem::TYPE_BOOL ||
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

  assert(first_child()->has_more(ctx));
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

  assert(first_child()->has_more(ctx));

  while (first_child()->has_more(ctx)) {
    db_tuple_vec v;
    first_child()->next(ctx, v);
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
      assert( e.columns.size() > _pos.size() );
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

  assert(first_child()->has_more(ctx));

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

  // TODO: external merge sort?
  assert(first_child()->has_more(ctx));

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

  assert(has_more(ctx));
  db_tuple_vec v;
  first_child()->next(ctx, v);
  size_t i = 0;
  while (i < v.size() && _i < _n) {
    tuples.push_back(v[i]);
    i++; _i++;
  }
}
