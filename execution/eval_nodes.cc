#include <execution/eval_nodes.hh>

using namespace std;
using namespace util;

db_elem
case_expr_case_node::eval(exec_context& ctx)
{
  return second_child()->eval(ctx);
}

db_elem
case_when_node::eval(exec_context& ctx)
{
  vector<case_expr_case_node*> cases =
    change_ptr_vec_type<case_expr_case_node>(_children);

  // implementation is trickier b/c of vector elements
  vector<size_t> vcs = ctx.tuple->get_vector_columns();
  if (!vcs.empty()) {
    size_t s = ctx.tuple->columns[vcs.front()].size();
    vector< db_elem > elems;
    elems.reserve(s);
    for (size_t i = 0; i < s; i++) {
      exec_context ctx0 = ctx.bind_row(i);
      bool done = false;
      for (auto c : cases) {
        if (c->condition()->eval(ctx0)) {
          elems.push_back( c->eval(ctx0) );
          done = true;
          break;
        }
      }
      if (!done && _dft) {
        elems.push_back( _dft->eval(ctx0) );
        done = true;
      }
      if (!done) elems.push_back( db_elem(db_elem::null_tag) );
    }
    return db_elem(elems);
  } else {
    // straightforward implementation
    for (auto c : cases) {
      if (c->condition()->eval(ctx)) {
        return c->eval(ctx);
      }
    }
    if (_dft) return _dft->eval(ctx);
  }
  return db_elem(db_elem::null_tag);
}

db_elem
subselect_node::eval(exec_context& ctx)
{
  // build args for subselect
  db_tuple args;
  args.columns.reserve(_children.size());
  for (auto c : _children) args.columns.push_back(c->eval(ctx));

  // check for vector args - must either be all scalars or all vectors
  vector<size_t> vcs = args.get_vector_columns();
  assert(vcs.empty() || vcs.size() == args.columns.size());

  if (vcs.empty()) {
    return eval_scalar(ctx, args);
  } else {
    size_t n = args.columns.front().size();
    vector< db_elem > elems;
    elems.reserve(n);
    for (size_t i = 0; i < n; i++) {
      db_tuple scalar_args;
      size_t m = args.columns.size();
      scalar_args.columns.reserve(m);
      for (size_t j = 0; j < m; j++) {
        scalar_args.columns.push_back(args.columns[j][i]);
      }
      elems.push_back(eval_scalar(ctx, scalar_args));
    }
    return db_elem(elems);
  }
}

db_elem
subselect_node::eval_scalar(exec_context& ctx, db_tuple& scalar_args)
{
  // assume that subselects are deterministic (so we can cache based
  // on just the argument values)
  auto it = _cached_values.find(scalar_args);
  if (it != _cached_values.end()) {
    // cache hit
    return it->second;
  }

  exec_context eval_ctx = ctx.bind_args(&scalar_args);

  assert(_n < ctx.subqueries.size());
  physical_operator* op = ctx.subqueries[_n];
  op->open(eval_ctx);
  physical_operator::db_tuple_vec v;
  while (op->has_more(eval_ctx)) op->next(eval_ctx, v);
  op->close(eval_ctx);

  // we expect this query to return a single scalar result (or empty)
  assert(v.size() == 0 || v.size() == 1);

  if (v.size() == 0) {
    // exists query, return false
    dprintf("subquery(%s): returning false\n", TO_C(_n));
    return _cached_values[scalar_args] = db_elem(false);
  } else {
    // scalar result
    assert(v[0].columns.size() == 1);
    dprintf("subquery(%s): %s\n", TO_C(_n), TO_C(v[0].columns.front()));
    return _cached_values[scalar_args] = v[0].columns.front();
  }
}
