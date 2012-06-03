#include <stdexcept>
#include <assert.h>
#include <edb/ConnectNew.hh>
#include <util/cryptdb_log.hh>

using namespace std;

class MySQLDBRes : public DBResultNew {
  public:
    MySQLDBRes(MYSQL_RES* native) : native(native) {}
    virtual ~MySQLDBRes() {
      mysql_free_result(native);
    }
    virtual ResType unpack() {
      MYSQL_RES* n = native;
      if (n == NULL)
          return ResType();

      size_t rows = (size_t)mysql_num_rows(n);
      int cols  = -1;
      if (rows > 0) {
          cols = mysql_num_fields(n);
      } else {
          return ResType();
      }

      ResType res;

      for (int j = 0;; j++) {
          MYSQL_FIELD *field = mysql_fetch_field(n);
          if (!field)
              break;

          res.names.push_back(field->name);
          res.types.push_back(field->type);
      }

      for (int index = 0;; index++) {
          MYSQL_ROW row = mysql_fetch_row(n);
          if (!row)
              break;
          unsigned long *lengths = mysql_fetch_lengths(n);

          vector<SqlItem> resrow;

          for (int j = 0; j < cols; j++) {
              SqlItem item;
              if (row[j] == NULL) {
                  item.null = true;
              } else {
                  item.null = false;
                  item.type = res.types[j];
                  item.data = string(row[j], lengths[j]);
              }
              resrow.push_back(item);
          }

          res.rows.push_back(resrow);
      }
      return res;
    }

    virtual bool has_more() {
      throw runtime_error("UNIMPL");
    }

    virtual void next(ResType& results) {
      throw runtime_error("UNIMPL");
    }

  private:
    MYSQL_RES* native;
};

class PGDBRes : public DBResultNew {
  public:
    PGDBRes(PGresult* native)
      : native(native), _i(0), _n(0), _c(0) {
      assert(native);
      _n = PQntuples(native);
      _c = PQnfields(native);
    }

    virtual ~PGDBRes() {
      PQclear(native);
    }

    virtual ResType unpack() {
      ResType res;
      fill(res, _n);
      return res;
    }

    virtual bool has_more() { return _i < _n; }

    virtual void next(ResType& results) { fill(results, _i + BatchSize); }

  private:

    void fill(ResType& res, unsigned int to) {
      unsigned int end = to > _n ? _n : to;
      for (; _i < end; _i++) {
        vector<SqlItem> resrow;
        for (unsigned int col = 0; col < _c; col++) {
          SqlItem item;
          item.data = string(PQgetvalue(native, _i, col), PQgetlength(native, _i, col));
          item.type = PQftype(native, col);
          assert(item.type >= 17); // look at: select oid, typname from pg_type
          if (item.type == 17 /*BYTEAOID*/) {
            // need to escape (binary string)
            ostringstream o;
            for (size_t i = 0; i < item.data.size(); ) {
              switch (item.data[i]) {
                case '\\':
                  if (item.data[i + 1] == '\\') {
                    o << '\\';
                    i += 2;
                  } else {
                    char buf[3];
                    buf[0] = item.data[i + 1];
                    buf[1] = item.data[i + 2];
                    buf[2] = item.data[i + 3];
                    o << (unsigned char) strtoul(buf, NULL, 8);
                    i += 4;
                  }
                  break;
                default:
                  o << item.data[i];
                  i++;
              }
            }
            item.data = o.str();
          }
          resrow.push_back(item);
        }
        res.rows.push_back(resrow);
      }

      for (unsigned int col = 0; col < _c; col++) {
        res.types.push_back(PQftype(native, col));
      }
    }

    PGresult* native;
    size_t _i;
    size_t _n;
    size_t _c;
};

bool
ConnectNew::execute(const string &query)
{
    DBResultNew *aux;
    bool r = execute(query, aux);
    if (r)
        delete aux;
    return r;
}

MySQLConnect::MySQLConnect(
    string server, string user, string passwd,
    string dbname, uint port)
  : conn(NULL)
{
    const char *dummy_argv[] =
        {
            "progname",
            "--skip-grant-tables",
            "--skip-innodb",
            "--default-storage-engine=MEMORY",
            "--character-set-server=utf8",
            "--language=" MYSQL_BUILD_DIR "/sql/share/"
        };
    assert(0 == mysql_server_init(sizeof(dummy_argv) / sizeof(*dummy_argv),
                                  (char**) dummy_argv, 0));

    conn = mysql_init(NULL);

    /* Make sure we always connect via TCP, and not via Unix domain sockets */
    uint proto = MYSQL_PROTOCOL_TCP;
    mysql_options(conn, MYSQL_OPT_PROTOCOL, &proto);

    /* Connect to the real server even if linked against embedded libmysqld */
    mysql_options(conn, MYSQL_OPT_USE_REMOTE_CONNECTION, 0);

    /* Connect to database */
    if (!mysql_real_connect(conn, server.c_str(), user.c_str(),
                            passwd.c_str(), dbname.c_str(), port, 0, 0)) {
        LOG(warn) << "connecting to server " << server
                  << " user " << user
                  << " pwd " << passwd
                  << " dbname " << dbname
                  << " port " << port;
        LOG(warn) << "mysql_real_connect: " << mysql_error(conn);
        throw runtime_error("cannot connect");
    }
}

MySQLConnect::~MySQLConnect() {
  mysql_close(conn);
}

bool
MySQLConnect::execute(const string& query, DBResultNew*& res) {
  if (mysql_query(conn, query.c_str())) {
      LOG(warn) << "mysql_query: " << mysql_error(conn);
      LOG(warn) << "on query: " << query;
      res = 0;
      return false;
  } else {
      res = new MySQLDBRes(mysql_store_result(conn));
      return true;
  }
}

string
MySQLConnect::getError() {
  return mysql_error(conn);
}

PGConnect::PGConnect(std::string server, std::string user, std::string passwd,
             std::string dbname, uint port) : conn(NULL) {
  ostringstream o;
  o << "host = " << server << " ";
  if (port) o << "port = " << port << " ";
  o << "dbname = " << dbname << " "
    << "user = " << user << " "
    << "password = " << passwd << " "
    ;
  conn = PQconnectdb(o.str().c_str());
  if (PQstatus(conn) != CONNECTION_OK) {
      LOG(warn) << "Connection to database failed: "
                 << PQerrorMessage(conn);
      throw runtime_error("cannot connect");
  }
}

PGConnect::~PGConnect() {
  PQfinish(conn);
}

bool
PGConnect::execute(const string& query, DBResultNew*& res) {
  PGresult* pg = PQexec(conn, query.c_str());
  if (!pg) {
    res = 0;
    return false;
  }
  ExecStatusType status = PQresultStatus(pg);
  if ((status == PGRES_COMMAND_OK) || (status == PGRES_TUPLES_OK)) {
      res = new PGDBRes(pg);
      return true;
  } else {
    LOG(warn) << "PQerrorMessage: " << PQerrorMessage(conn);
    LOG(warn) << "on query: " << query;
    res = 0;
    return false;
  }
}

string
PGConnect::getError() {
  return PQerrorMessage(conn);
}
