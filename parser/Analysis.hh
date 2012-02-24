#pragma once

#include <crypto-old/CryptoManager.hh>
#include <util/onions.hh>

#include <parser/embedmysql.hh>
#include <parser/stringify.hh>

using namespace std;

struct FieldMeta;
/**
 * Field here is either:
 * A) empty string, representing any field or
 * B) the field that the onion is key-ed on. this
 *    only has semantic meaning for DET and OPE
 */
typedef std::pair<SECLEVEL, FieldMeta *> LevelFieldPair;
typedef std::map<onion, LevelFieldPair>  OnionLevelFieldMap;
typedef std::pair<onion, LevelFieldPair> OnionLevelFieldPair;
typedef std::map<onion, SECLEVEL>        OnionLevelMap;


/**
 * Use to keep track of a field's encryption onions.
 */
class EncDesc {
public:
    EncDesc(OnionLevelMap input) : olm(input) {}
    EncDesc(const EncDesc & ed) : olm(ed.olm) {}
    /**
     * Returns true if something was changed, false otherwise.
     */
    bool restrict(onion o, SECLEVEL maxl);

    OnionLevelMap olm;
};

struct TableMeta;
//TODO: FieldMeta and TableMeta are partly duplicates with the original
// FieldMetadata an TableMetadata
// which contains data we want to add to this structure soon
// we can remove old ones when we have needed functionality here
typedef struct FieldMeta {
    TableMeta * tm; //point to table belonging in
    std::string fname;
    Create_field * sql_field;

    map<onion, string> onionnames;
    EncDesc encdesc;

    bool has_salt; //whether this field has its own salt
    std::string salt_name;

    FieldMeta();

    inline bool hasOnion(onion o) const {
        return encdesc.olm.find(o) !=
            encdesc.olm.end();
    }

    inline SECLEVEL getOnionLevel(onion o) const {
        auto it = encdesc.olm.find(o);
        if (it == encdesc.olm.end()) return SECLEVEL::INVALID;
        return it->second;
    }

    inline bool setOnionLevel(onion o, SECLEVEL maxl) {
        return encdesc.restrict(o, maxl);
    }

} FieldMeta;


typedef struct TableMeta {

    std::list<std::string> fieldNames;     //in order field names                     
    unsigned int tableNo;
    std::string anonTableName;

    std::map<std::string, FieldMeta *> fieldMetaMap;

    bool hasSensitive;

    bool has_salt;
    std::string salt_name;

    TableMeta();
    ~TableMeta();
} TableMeta;

typedef struct SchemaInfo {
    std::map<std::string, TableMeta *> tableMetaMap;
    unsigned int totalTables;
    embedmysql * embed_db;

    SchemaInfo():totalTables(0) {};
    ~SchemaInfo() {cerr << "called schema destructor"; tableMetaMap.clear();}
    TableMeta * getTableMeta(const string & table);
    FieldMeta * getFieldMeta(const string & table, const string & field);
} SchemaInfo;

/***************************************************/

// metadata for field analysis                                                        
class FieldAMeta {
public:
    EncDesc exposedLevels; //field identifier to max sec level allowed to process a query
    FieldAMeta(const EncDesc & ed) : exposedLevels(ed) {}
};

//Metadata about how to encrypt an item
class ItemMeta {
public:
    onion o;
    SECLEVEL uptolevel;
    FieldMeta * basefield;
};
extern "C" void *create_embedded_thd(int client_flag);

typedef struct ReturnField {
    bool is_salt;
    std::string field_called;
    ItemMeta *im;
    int pos_salt; //position of salt of this field in the query results,
                  // or -1 if such salt was not requested
} ReturnField;

typedef struct ReturnMeta {
    map<int, ReturnField> rfmeta;
} ReturnMeta;


class Analysis {
public:
    Analysis(MYSQL *m, SchemaInfo * schema, CryptoManager *cm)
        : pos(0), schema(schema), cm(cm), m(m) {
        assert(m != NULL);
    }
    Analysis(): pos(0), schema(NULL), cm(NULL), m(NULL) {
    }
    inline MYSQL* conn() {
        mysql_thread_init();
        return m;
    }

    unsigned int pos; //a counter indicating how many projection fields have been analyzed so far                                                                    
    std::map<std::string, FieldAMeta *> fieldToAMeta;
    std::map<Item*, ItemMeta *>         itemToMeta;
    std::map<Item_field*, FieldMeta*>   itemToFieldMeta;
    std::set<Item*>                     itemHasRewrite;
    SchemaInfo *                        schema;
    CryptoManager *                     cm;

    ReturnMeta rmeta;

private:
    MYSQL * m;

};

typedef struct ConnectionData {
    std::string server;
    std::string user;
    std::string psswd;
    std::string dbname;
    uint port;

    ConnectionData() {}

    ConnectionData(std::string serverarg, std::string userarg, std::string psswdarg, std::string dbnamearg, uint portarg = 0) {
        server = serverarg;
        user = userarg;
        psswd = psswdarg;
        dbname = dbnamearg;
        port = portarg;
    }

} ConnectionData;
