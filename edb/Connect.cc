/*
 * Connect.cpp
 *
 *  Created on: Dec 1, 2010
 *      Author: raluca
 */

#include <stdexcept>
#include <assert.h>
#include <edb/Connect.hh>
#include <util/cryptdb_log.hh>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

Connect::Connect(const string &server, const string &user, const string &passwd,
                 uint port)
    : conn(nullptr), close_on_destroy(true)
{
    do_connect(server, user, passwd, port);
}

Connect::Connect(const string &server, const string &user, const string &passwd,
                 const string &dbname, uint port)
    : conn(nullptr), close_on_destroy(true)
{
    do_connect(server, user, passwd, port);
    if (!select_db(dbname))
        thrower() << "cannot select dbname " << dbname;
}

void
Connect::do_connect(const string &server, const string &user,
                    const string &passwd, uint port)
{
    const char *dummy_argv[] = {
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
                            passwd.c_str(), 0, port, 0, 0)) {
        LOG(warn) << "connecting to server " << server
                  << " user " << user
                  << " pwd " << passwd
                  << " port " << port;
        LOG(warn) << "mysql_real_connect: " << mysql_error(conn);
        throw runtime_error("cannot connect");
    }
}

bool
Connect::select_db(const std::string &dbname)
{
    return mysql_select_db(conn, dbname.c_str()) ? false : true;
}

Connect * Connect::getEmbedded() {
    MYSQL * m = mysql_init(0);
    assert(m);
    
    mysql_options(m, MYSQL_OPT_USE_EMBEDDED_CONNECTION, 0);
    
    if (!mysql_real_connect(m, 0, 0, 0, 0, 0, 0, CLIENT_MULTI_STATEMENTS)) {
        mysql_close(m);
        cryptdb_err() << "mysql_real_connect: " << mysql_error(m);
    }

    Connect * conn = new Connect(m);
    conn->close_on_destroy = true;

    return conn;
}

bool
Connect::execute(const string &query, DBResult * & res)
{
    //silently ignore empty queries
    if (query.length() == 0) {
        LOG(warn) << "empty query";
        res = 0;
        return true;
    }
    bool success = true;
    if (mysql_query(conn, query.c_str())) {
        LOG(warn) << "mysql_query: " << mysql_error(conn);
        LOG(warn) << "on query: " << query;
        res = 0;
        success = false;
    } else {
        res = DBResult::wrap(mysql_store_result(conn));
    }

    // HACK taken from stephentu:
    // Calling mysql_query seems to have destructive effects
    // on the current_thd. Thus, we must call create_embedded_thd
    // again.
    // TODO: mysql examples for embed server do not need to recreate thd,
    // what are we doing different?
    void* ret = create_embedded_thd(0);
    if (!ret) assert(false);

    
    return success;
}


bool
Connect::execute(const string &query)
{
    DBResult *aux;
    bool r = execute(query, aux);
    if (r)
        delete aux;
    return r;
}

string
Connect::getError()
{
    return mysql_error(conn);
}


my_ulonglong
Connect::last_insert_id()
{
    return mysql_insert_id(conn);
}

Connect::~Connect()
{
    if (close_on_destroy) {
        mysql_close(conn);
    }
}

DBResult::DBResult()
{
}

DBResult *
DBResult::wrap(DBResult_native *n)
{
    DBResult *r = new DBResult();
    r->n = n;
    return r;
}

DBResult::~DBResult()
{
    mysql_free_result(n);
}

static Item *
getItem(char * content, enum_field_types type, uint len) {
    if (content == NULL) {
        return new Item_null();
    }
    Item * i;
    string content_str = string(content, len);
    if (IsMySQLTypeNumeric(type)) {
        ulonglong val = valFromStr(content_str);
        i = new Item_int(val);
    } else {
        i = new Item_string(make_thd_string(content_str), len, &my_charset_bin);
    }
    
    return i;
}

// returns the data in the last server response
// TODO: to optimize return pointer to avoid overcopying large result sets?
ResType
DBResult::unpack()
{
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

        vector<Item *> resrow;

        for (int j = 0; j < cols; j++) {
            Item * it = getItem(row[j], res.types[j], lengths[j]);
            resrow.push_back(it);
        }

        res.rows.push_back(resrow);
    }

    return res;
}

// returns the data in the last server response
// TODO: to optimize return pointer to avoid overcopying large result sets?
string
DBResult::oneval()
{
    assert(n != NULL);

    size_t rows = (size_t)mysql_num_rows(n);
    assert(rows == 1);
    assert(mysql_num_fields(n) ==1);

    MYSQL_FIELD * field = mysql_fetch_field(n);
    assert(field != 0);
    MYSQL_ROW row = mysql_fetch_row(n);
    unsigned long *lengths = mysql_fetch_lengths(n);
    return string(row[0], lengths[0]);

}
