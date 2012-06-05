#pragma once

#include <vector>

#include <execution/tuple.hh>
#include <execution/context.hh>

class expr_node {
public:

  typedef std::vector< expr_node* > child_vec;

  expr_node() {}
  expr_node(const std::vector< expr_node* > children)
    : _children(children) {}

  virtual ~expr_node() {
    for (auto e : _children) delete e;
  }

  virtual db_elem eval(exec_context& ctx) = 0;

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
