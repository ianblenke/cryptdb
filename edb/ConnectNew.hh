#pragma once

#include <vector>
#include <string>
#include <util/util.hh>
#include <libpq-fe.h>

class DBResultNew {
  public:
    virtual ~DBResultNew() {}
    virtual ResType unpack() = 0;

    virtual bool has_more() = 0;
    virtual void next(ResType& results) = 0;

    virtual size_t size() const = 0;

    static const unsigned int BatchSize = (1024 * 256);
};

class ConnectNew {
 public:
    virtual ~ConnectNew() {}

    virtual bool execute(const std::string &query, DBResultNew *&) = 0;
    bool execute(const std::string &query);

    virtual std::string getError() = 0;
};

// this is a temporary hack, which allows both the mysql and the
// postgres versions to exists at the same time (which the macros don't)

class MySQLConnect : public ConnectNew {
  public:
    MySQLConnect(std::string server, std::string user, std::string passwd,
                 std::string dbname, uint port = 0);
    virtual ~MySQLConnect();

    virtual bool execute(const std::string &query, DBResultNew *&);
    virtual std::string getError();

  private:
    MYSQL* conn;
};

class PGConnect : public ConnectNew {
  public:
    PGConnect(std::string server, std::string user, std::string passwd,
             std::string dbname, uint port = 0, bool binary = false);
    virtual ~PGConnect();

    virtual bool execute(const std::string &query, DBResultNew *&);
    virtual std::string getError();

  private:
    bool _binary;
    PGconn* conn;
};
