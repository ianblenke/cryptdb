#pragma once

#include <cassert>
#include <vector>

#include <edb/ConnectNew.hh>
#include <parser/cdb_helpers.hh>

// forward decls (avoid circular references)
class db_tuple;
class physical_operator;
class query_cache;

class exec_context {
public:
  exec_context(

      ConnectNew* connection,
      crypto_manager_stub* crypto,
      query_cache* cache = NULL,

      db_tuple* tuple = NULL /* current tuple context */,
      ssize_t idx = -1, /* only meaningful if tuple != NULL */

      const std::vector< physical_operator* >& subqueries =
        std::vector< physical_operator* >(),

      db_tuple* args = NULL

  ) : connection(connection), crypto(crypto), cache(cache),
      tuple(tuple), idx(idx), subqueries(subqueries), args(args) {}

  // if previous tuple bound, the information is discarded
  inline exec_context bind(
      db_tuple* tuple,
      const std::vector< physical_operator* >& subqueries =
        std::vector< physical_operator* >()) {
    assert(tuple != NULL);
    return exec_context(
        connection,
        crypto,
        cache,
        tuple,
        -1,
        subqueries,
        args);
  }

  // assumes a tuple has already been bound, but the row is not bound
  inline exec_context bind_row(size_t idx) {
    assert(this->tuple != NULL);
    assert(this->idx == -1);
    return exec_context(
        connection,
        crypto,
        cache,
        tuple,
        ssize_t(idx),
        subqueries,
        args);
  }

  inline exec_context bind_args(db_tuple* args) {
    assert(args);
    return exec_context(
        connection,
        crypto,
        cache,
        tuple,
        ssize_t(idx),
        subqueries,
        args);
  }

  // no ownership for any of the pointers

  ConnectNew* const connection;
  crypto_manager_stub* const crypto;
  query_cache* const cache;

  db_tuple* const tuple;
  const ssize_t idx;

  // if idx != -1, then we are evaluating specifially the
  // idx-th row of this tuple (this only applies to the vector
  // columns, non-vector columns are unaffected)
  const std::vector< physical_operator* > subqueries;

  db_tuple* const args;
};
