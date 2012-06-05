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
