#pragma once

/*
 * cdb_rewrite.hh
 *
 *
 *  TODO: need to integrate it with util.h: some declarations are repeated
 */

#include <edb/Translator.hh>
#include <edb/Connect.hh>
#include <edb/MultiPrinc.hh>

#include <sql_select.h>
#include <sql_delete.h>
#include <sql_insert.h>
#include <sql_update.h>

#include <parser/Analysis.hh>
#include <edb/Annotation.hh>
#include <parser/stringify.hh>

#include <util/errstream.hh>
#include <util/cleanup.hh>
#include <util/rob.hh>


using namespace std;


class FieldReturned {
public:
    bool encrypted;
    bool includeInResult;
    std::string key;
    unsigned int SaltIndex;
    std::string nameForReturn;
};

void
printRes(const ResType & r);

typedef class ConnectionInfo {
public:
    std::string server;
    uint port;
    std::string user;
    std::string passwd;
   
    ConnectionInfo(std::string s, std::string u, std::string p, uint port = 0) :
	server(s), port(port), user(u), passwd(p) {}
    
} ConnectionInfo;


class Rewriter {
public:
    Rewriter(ConnectionInfo ci, 
             const std::string &embed_dir,
             bool MultiPrinc = false,
             bool encByDefault = true);
    ~Rewriter();

    void setMasterKey(const std::string &mkey);
    list<std::string> rewrite(const std::string &q, Analysis & a);
    ResType decryptResults(ResType & dbres, Analysis & a);

private:
    void initSchema();
    void createMetaTablesIfNotExists();
    list<string> processAnnotation(Annotation annot, Analysis &a);
    //initialize multi-principal data structures
    void mp_init(Analysis &a);
    list<string> rewrite_helper(const string & q, Analysis & a, query_parse & p);

    ConnectionInfo ci;
    // current database name -- wrong place for it, must be per-connection!
    std::string    cur_db;
    // connection to remote and embedded server
    Connect*       conn;
    Connect*       e_conn;

    bool           encByDefault;
    AES_KEY*       masterKey;
    
    SchemaInfo*    schema;
    unsigned int   totalTables;

    MultiPrinc*    mp;
    TMKM           tmkm;

};

class ScopedMySQLRes {
public:
    ScopedMySQLRes(MYSQL_RES *r) : r(r) {
        if (!r) fatal() << "input result set was NULL";
    }
    ~ScopedMySQLRes() {
        assert(r);
        mysql_free_result(r);
    }
    inline MYSQL_RES *res() { return r; }
private:
    MYSQL_RES *r;
};
