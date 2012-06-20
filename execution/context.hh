#pragma once

#include <cassert>
#include <vector>
#include <map>
#include <utility>

#include <execution/common.hh>
#include <edb/ConnectNew.hh>
#include <crypto/paillier.hh>
#include <parser/cdb_helpers.hh>

// forward decls (avoid circular references)
class db_tuple;
class physical_operator;
class query_cache;

class paillier_cache {
public:
  ~paillier_cache() {
    for (auto p : _pp_cache) delete p.second;
  }

  Paillier_priv& get_paillier_priv(size_t nbits, size_t abits) {
    auto mkey = std::make_pair(nbits, abits);
    auto it = _pp_cache.find(mkey);
    if (it != _pp_cache.end()) return *it->second;
    auto sk = Paillier_priv::keygen(nbits, abits);
    return *(_pp_cache[mkey] = new Paillier_priv(sk));
  }

private:
  std::map< std::pair< size_t, size_t >, Paillier_priv* > _pp_cache;
};

typedef std::vector< std::vector< uint64_t > >
        dict_compress_tables;

class exec_context {
public:
  exec_context(

      ConnectNew* connection,
      crypto_manager_stub* crypto,
      paillier_cache* pp_cache,

      dict_compress_tables* dict_tables = NULL,

      query_cache* cache = NULL,

      db_tuple* tuple = NULL /* current tuple context */,
      ssize_t idx = -1, /* only meaningful if tuple != NULL */

      const std::vector< physical_operator* >& subqueries =
        std::vector< physical_operator* >(),

      db_tuple* args = NULL

  ) : connection(connection), crypto(crypto), pp_cache(pp_cache), dict_tables(dict_tables),
      cache(cache), tuple(tuple), idx(idx), subqueries(subqueries), args(args) {}

  // if previous tuple bound, the information is discarded
  inline exec_context bind(
      db_tuple* tuple,
      const std::vector< physical_operator* >& subqueries =
        std::vector< physical_operator* >()) {
    SANITY(tuple != NULL);
    return exec_context(
        connection,
        crypto,
        pp_cache,
        dict_tables,
        cache,
        tuple,
        -1,
        subqueries,
        args);
  }

  // assumes a tuple has already been bound, but the row is not bound
  inline exec_context bind_row(size_t idx) {
    SANITY(this->tuple != NULL);
    SANITY(this->idx == -1);
    return exec_context(
        connection,
        crypto,
        pp_cache,
        dict_tables,
        cache,
        tuple,
        ssize_t(idx),
        subqueries,
        args);
  }

  inline exec_context bind_args(db_tuple* args) {
    SANITY(args);
    return exec_context(
        connection,
        crypto,
        pp_cache,
        dict_tables,
        cache,
        tuple,
        ssize_t(idx),
        subqueries,
        args);
  }

  // no ownership for any of the pointers

  ConnectNew* const connection;
  crypto_manager_stub* const crypto;
  paillier_cache* const pp_cache;
  dict_compress_tables* const dict_tables;
  query_cache* const cache;

  db_tuple* const tuple;
  const ssize_t idx;

  // if idx != -1, then we are evaluating specifially the
  // idx-th row of this tuple (this only applies to the vector
  // columns, non-vector columns are unaffected)
  const std::vector< physical_operator* > subqueries;

  db_tuple* const args;

};
