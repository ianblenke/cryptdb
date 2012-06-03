#pragma once

#include <edb/ConnectNew.hh>
#include <parser/cdb_helpers.hh>

class exec_context {
public:
  exec_context(
      ConnectNew* connection,
      crypto_manager_stub* crypto) : connection(connection), crypto(crypto) {}

  ConnectNew* const connection;
  crypto_manager_stub* const crypto;
};
