#pragma once

#include <iostream>
#include <vector>

#include <execution/tuple.hh>
#include <execution/context.hh>

/**
 * Base class for execution operators
 *
 * Each physical operator implements a standard database iterator-style
 * interface
 */
class physical_operator {
public:

  typedef std::vector< db_tuple > db_tuple_vec;
  typedef std::vector< size_t > pos_vec;
  typedef std::vector< physical_operator* > op_vec;
  typedef std::vector< db_column_desc > desc_vec;

  inline static void print_tuples(const db_tuple_vec& tuples) {
    for (auto &t : tuples) std::cout << t << std::endl;
  }

  static const unsigned int BatchSize = 8192 * 4; // hinted batch size for operators

  /** Takes ownership of children */
  physical_operator(
      const std::vector< physical_operator* >& children)
    : _children(children) {}

  virtual ~physical_operator() {
    for (auto c : _children) delete c;
  }

  virtual void open(exec_context& ctx) {
    for (auto c : _children) c->open(ctx);
  }

  virtual void close(exec_context& ctx) {
    for (auto c : _children) c->close(ctx);
  }

  virtual desc_vec tuple_desc() {
    // default impl takes it from the first child
    return first_child()->tuple_desc();
  }

  // first interface type - iterator style

  virtual bool has_more(exec_context& ctx) {
    // default impl takes it from the first child
    return first_child()->has_more(ctx);
  }

  virtual void next(exec_context& ctx, db_tuple_vec& tuple) = 0;

  // second interface type - one shot execution
  virtual void execute(exec_context& ctx) {
    db_tuple_vec v;
    while (has_more(ctx)) next(ctx, v);
  }

protected:

  inline physical_operator* first_child() {
    assert(!_children.empty());
    return _children.front();
  }

  std::vector< physical_operator* > _children;
};
