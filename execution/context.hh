#pragma once

#include <edb/ConnectNew.hh>
#include <crypto-old/CryptoManager.hh>

class exec_context {
public:
  exec_context(
      ConnectNew* connection,
      CryptoManager* crypto) : connection(connection), crypto(crypto) {}

  ConnectNew* const connection;
  CryptoManager* const crypto;
};
