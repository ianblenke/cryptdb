#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <string>

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
  typedef std::map< std::string, physical_operator* > str_map;

  inline static void print_tuples(const db_tuple_vec& tuples) {
    for (auto &t : tuples) std::cout << t << std::endl;
    std::cout << "(" << tuples.size() << " rows)" << std::endl;
  }

  /** Assuming each tuple is about 128 bytes, this gives us a ~32MB buffer */
  // NOTE: should match the one in edb/ConnectNew.hh
  static const unsigned int BatchSize = (1024 * 256);

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

  // the next() invocation contract is as follows:
  //   A) has_more() must be true
  //   B) it assumes the tuples vector is empty to start with
  //      if you put contents in tuples before passing to next(),
  //      results are undefined
  virtual void next(exec_context& ctx, db_tuple_vec& tuples) = 0;

  // second interface type - one shot execution
  void slurp(exec_context& ctx, db_tuple_vec& tuples) {
    while (has_more(ctx)) {
      db_tuple_vec v;
      next(ctx, v);
      tuples.reserve( tuples.size() + v.size() );
      tuples.insert( tuples.end(), v.begin(), v.end() );
    }
  }

  inline void execute_and_discard(exec_context& ctx) {
    db_tuple_vec tuples;
    slurp(ctx, tuples);
  }

protected:

  inline physical_operator* first_child() {
    assert(!_children.empty());
    return _children.front();
  }

  std::vector< physical_operator* > _children;
};
