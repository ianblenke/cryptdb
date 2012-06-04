#pragma once

#include <execution/tuple.hh>
#include <vector>

class eval_context {
public:
  eval_context(db_tuple* tuple, ssize_t idx = -1) : tuple(tuple), idx(idx) {}

  inline eval_context bind_row(size_t idx) {
    return eval_context(tuple, ssize_t(idx));
  }

  // no ownership
  db_tuple* const tuple; // current tuple being evaluated

  const ssize_t idx; // if idx != -1, then we are evaluating specifially the
                     // idx-th row of this tuple (this only applies to the vector
                     // columns, non-vector columns are unaffected)
};

class expr_node {
public:

  typedef std::vector< expr_node* > child_vec;

  expr_node() {}
  expr_node(const std::vector< expr_node* > children)
    : _children(children) {}

  virtual ~expr_node() {
    for (auto e : _children) delete e;
  }

  virtual db_elem eval(eval_context& ctx) = 0;

protected:

  inline expr_node* first_child() {
    return _children.at(0);
  }

  inline expr_node* second_child() {
    return _children.at(1);
  }

  inline expr_node* last_child() {
    return _children.back();
  }

  std::vector< expr_node* > _children;
};
