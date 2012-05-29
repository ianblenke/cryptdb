#pragma once

#include <execution/tuple.hh>
#include <vector>

class eval_context {
public:
  eval_context(db_tuple* tuple) : tuple(tuple) {}

  // no ownership
  db_tuple* const tuple;
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

  std::vector< expr_node* > _children;
};
