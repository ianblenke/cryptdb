#pragma once

/*
 * Connect.h
 *
 */

#include <vector>
#include <string>
#include <util/util.hh>
#include <edb/sql_utils.hh>


extern "C" void *create_embedded_thd(int client_flag);

class DBResult {
 private:
    DBResult();

 public:
    ~DBResult();
    DBResult_native *n;

    //returns data from this db result
    ResType unpack();

    std::string oneval(); 

    static DBResult *wrap(DBResult_native *);
};

class Connect {
 public:
    Connect(const std::string &server, const std::string &user,
            const std::string &passwd, uint port = 0);
    Connect(const std::string &server, const std::string &user,
            const std::string &passwd, const std::string &dbname, uint port = 0);

    Connect(MYSQL* _conn) : conn(_conn), close_on_destroy(false) { }

    //returns Connect for the embedded server
    static Connect * getEmbedded();
    
    // returns true if execution was ok; caller must delete DBResult
    bool execute(const std::string &query, DBResult *&);
    bool execute(const std::string &query);
    bool select_db(const std::string &dbname);

    // returns error message if a query caused error
    std::string getError();

    my_ulonglong last_insert_id();

    ~Connect();

 private:
    void do_connect(const std::string &server, const std::string &user,
                    const std::string &passwd, uint port);

    MYSQL *  conn;
    bool     close_on_destroy;
};
