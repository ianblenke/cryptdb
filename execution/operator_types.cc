#include <cassert>
#include <algorithm>
#include <stdexcept>

#include <parser/cdb_helpers.hh>
#include <execution/operator_types.hh>

using namespace std;

void
remote_sql_op::open(exec_context& ctx)
{
  physical_operator::open(ctx);
  // execute all the children as a one-shot
  for (auto c : _children) c->execute(ctx);

  // open and execute the sql statement
  if (!ctx.connection->execute(_sql, _res)) {
    throw runtime_error("Could not execute sql");
  }
  assert(_res);
}

void
remote_sql_op::close(exec_context& ctx)
{
  physical_operator::close(ctx);

  // close the sql statement
  if (_res) delete _res;
}

bool
remote_sql_op::has_more(exec_context& ctx)
{
  return _res->has_more();
}

void
remote_sql_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  assert(_res->has_more());

  ResType res;
  _res->next(res);
  tuples.resize(res.rows.size());
  for (size_t i = 0; i < res.rows.size(); i++) {
    vector<SqlItem>& row = res.rows[i];
    db_tuple& tuple = tuples[i];
    size_t c = row.size();
    tuple.columns.reserve(c);
    for (size_t j = 0; j < c; j++) {
      SqlItem& col = row[j];
      if (_desc_vec[j].is_vector) {
        // TODO: this is broken in general, but we might
        // be able to get away with it for TPC-H
        assert(db_elem::TypeFromPGOid(col.type) == db_elem::TYPE_STRING);

        vector<string> tokens;
        tokenize(col.data, ",", tokens);

        vector<db_elem> elems;
        elems.reserve(tokens.size());
        for (auto t : tokens) {
          elems.push_back(
              CreateFromString(t, _desc_vec[j].type));
        }
        tuple.columns.push_back(db_elem(elems));
      } else {
        tuple.columns.push_back(
            CreateFromString(col.data, db_elem::TypeFromPGOid(col.type)));
      }
    }
  }
}

db_elem
remote_sql_op::CreateFromString(
    const string& data,
    db_elem::type type) {
  switch (db_elem::TypeFromPGOid(type)) {
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
    default: throw runtime_error("should not be reached");
  }
}

local_decrypt_op::desc_vec
local_decrypt_op::tuple_desc()
{
  desc_vec v = first_child()->tuple_desc();
  desc_vec ret; ret.reserve(v.size());
  for (auto &p : _pos) ret.push_back(v[p].decrypt_desc());
  return ret;
}

void
local_decrypt_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  assert(first_child()->has_more(ctx));
  desc_vec desc = first_child()->tuple_desc();

  first_child()->next(ctx, tuples);

  // two levels of potential parallelism:
  // 1) each column can be decrypted in parallel
  // 2) within a column, vector contexts can be decrypted in parallel

  crypto_manager_stub cm_stub(ctx.crypto, false);

  for (size_t i = 0; i < tuples.size(); i++) {
    db_tuple& tuple = tuples[i];
    for (auto p : _pos) {
      switch (desc[p].type) {
        case db_elem::TYPE_INT: {
          bool isBin = false;
          tuple.columns[p] =
            db_elem(
              resultFromStr<int64_t>(
                cm_stub.crypt<4>(
                  ctx.crypto->getmkey(),
                  tuple.columns[p].stringify(),
                  TYPE_INTEGER,
                  desc[p].fieldname,
                  desc[p].level, getMin(desc[p].onion_type), isBin, 12345)));
          break;
        }

        case db_elem::TYPE_STRING: {
          bool isBin = false;
          tuple.columns[p] =
            db_elem(
              cm_stub.crypt(
                ctx.crypto->getmkey(),
                tuple.columns[p].stringify(),
                TYPE_TEXT,
                desc[p].fieldname,
                desc[p].level, getMin(desc[p].onion_type), isBin, 12345));
          break;
        }

        default:
          throw runtime_error("UNIMPL");
      }
    }
  }
}

void
local_filter_op::next(exec_context& ctx, db_tuple_vec& tuples)
{
  assert(first_child()->has_more(ctx));
  db_tuple_vec v;
  first_child()->next(ctx, v);
  for (auto &tuple : v) {
    eval_context eval_ctx(&tuple);
    db_elem test = _filter->eval(eval_ctx);
    if (test.unsafe_cast_bool()) {
      tuples.push_back(tuple);
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
  assert(first_child()->has_more(ctx));
  db_tuple_vec v;
  first_child()->next(ctx, v);
  for (auto &t : v) {
    db_tuple tt;
    tt.columns.reserve(_trfm_vec.size());
    for (auto &e : _trfm_vec) {
      if (e.isLeft()) {
        tt.columns.push_back(t.columns[e.left()]);
      } else {
        eval_context eval_ctx(&t);
        tt.columns.push_back(e.right().second->eval(eval_ctx));
      }
    }
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
  assert(has_more(ctx));
  db_tuple_vec v;
  first_child()->next(ctx, v);
  size_t i = 0;
  while (i < v.size() && _i < _n) {
    tuples.push_back(v[i]);
    i++; _i++;
  }
}
