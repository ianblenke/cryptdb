#include <stdexcept>
#include <assert.h>
#include <netinet/in.h>

#include <edb/ConnectNew.hh>
#include <util/cryptdb_log.hh>
#include <util/pgoids.hh>

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

    virtual size_t size() const {
      throw runtime_error("UNIMPL");
    }

  private:
    MYSQL_RES* native;
};

class PGDBBaseRes : public DBResultNew {
  public:
    PGDBBaseRes(PGresult* native)
      : _native(native), _i(0), _n(0), _c(0) {
      assert(_native);
      _n = PQntuples(_native);
      _c = PQnfields(_native);
    }

    virtual ~PGDBBaseRes() {
      PQclear(_native);
    }

    virtual ResType unpack() {
      ResType res;
      fill(res, _n);
      return res;
    }

    virtual bool has_more() { return _i < _n; }

    virtual void next(ResType& results) { fill(results, _i + BatchSize); }

    virtual size_t size() const { return _n; }

  protected:
    virtual void
      read_into_buffer(
          size_t row, size_t col, unsigned int type, std::string& buffer) = 0;

    inline PGresult* native() { return _native; }

  private:

    void fill(ResType& res, unsigned int to) {
      unsigned int end = to > _n ? _n : to;
      for (; _i < end; _i++) {
        res.rows.push_back(vector<SqlItem>());
        vector<SqlItem>& resrow = res.rows.back();
        for (unsigned int col = 0; col < _c; col++) {
          resrow.push_back(SqlItem());
          SqlItem& item = resrow.back();
          item.type = PQftype(_native, col);
          read_into_buffer(_i, col, item.type, item.data);
        }
      }

      for (unsigned int col = 0; col < _c; col++) {
        res.types.push_back(PQftype(_native, col));
      }
    }

    PGresult* _native;
    size_t _i;
    size_t _n;
    size_t _c;
};

class PGDBTextRes : public PGDBBaseRes {
public:
  PGDBTextRes(PGresult* native) : PGDBBaseRes(native) {}
protected:
  virtual void
    read_into_buffer(size_t row, size_t col, unsigned int type, std::string& buffer) {
    buffer = string(PQgetvalue(native(), row, col), PQgetlength(native(), col, col));
    if (type == 17 /*BYTEAOID*/) {
      // need to escape (binary string)
      ostringstream o;
      for (size_t i = 0; i < buffer.size(); ) {
        switch (buffer[i]) {
          case '\\':
            if (buffer[i + 1] == '\\') {
              o << '\\';
              i += 2;
            } else {
              char buf[3];
              buf[0] = buffer[i + 1];
              buf[1] = buffer[i + 2];
              buf[2] = buffer[i + 3];
              o << (unsigned char) strtoul(buf, NULL, 8);
              i += 4;
            }
            break;
          default:
            o << buffer[i];
            i++;
        }
      }
      buffer = o.str();
    }
  }
};

class PGDBBinaryRes : public PGDBBaseRes {
public:
  PGDBBinaryRes(PGresult* native) : PGDBBaseRes(native) {}
protected:
  virtual void read_into_buffer(
      size_t row, size_t col, unsigned int type, std::string& buffer)
  {

    // null check
    if (PQgetisnull(native(), row, col)) {
      // TODO: do something more intelligent
      buffer = "";
      return;
    }

    char* ptr = PQgetvalue(native(), row, col);
    switch (type) {
      case BYTEAOID:
      case CHAROID:
      case TEXTOID:
      case VARCHAROID:
        // can treat as string
        buffer = string(ptr, PQgetlength(native(), row, col));
        break;

      case DATEOID:
        // don't bother to understand the postgres date binary format
        // since we don't actually do anything with it. so for now,
        // just convert into hex
        buffer = to_hex(string(ptr, PQgetlength(native(), row, col)));
        break;

      case INT4OID:
        // yes, this is weird, but we are trying to respect the
        // SqlItem contract here
        buffer = to_s(ntohl(*((uint32_t *) ptr)));
        break;

      case INT8OID:
        buffer = to_s(bswap64(*((uint64_t *) ptr)));
        break;

      default:
        cerr << "unhandled OID type: " << type << endl;
        assert(false);
        break;
    }
  }
private:
  template <typename T>
  static inline std::string to_s(const T& t) {
    std::ostringstream s;
    s << t;
    return s.str();
  }

  static inline std::string to_hex(const std::string& input) {
    // copied from parser/cdb_helpers.hh
    size_t len = input.length();
    const char* const lut = "0123456789ABCDEF";

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i) {
      const unsigned char c = (unsigned char) input[i];
      output.push_back(lut[c >> 4]);
      output.push_back(lut[c & 15]);
    }
    return output;
  }

  // no ntohll
  static uint64_t bswap64(uint64_t orig) {
    const uint8_t* src = (const uint8_t*) &orig;
    uint64_t res = 0;
    uint8_t* dest = (uint8_t*) &res;
    dest[0] = src[7];
    dest[1] = src[6];
    dest[2] = src[5];
    dest[3] = src[4];
    dest[4] = src[3];
    dest[5] = src[2];
    dest[6] = src[1];
    dest[7] = src[0];
    return res;
  }
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
             std::string dbname, uint port, bool binary)
  : _binary(binary), conn(NULL) {
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
  PGresult* pg =
    PQexecParams(conn, query.c_str(), 0, NULL, NULL, NULL, NULL, _binary ? 1 : 0);
  if (!pg) {
    res = 0;
    return false;
  }
  ExecStatusType status = PQresultStatus(pg);
  if ((status == PGRES_COMMAND_OK) || (status == PGRES_TUPLES_OK)) {
      res =
        _binary ? ((DBResultNew*)new PGDBBinaryRes(pg)) :
                  ((DBResultNew*)new PGDBTextRes(pg));
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
