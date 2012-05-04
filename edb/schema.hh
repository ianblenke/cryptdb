#include <util/onions.hh>

#include <parser/embedmysql.hh>
#include <parser/stringify.hh>
#include <parser/CryptoHandlers.hh>


#include <string>
#include <map>
#include <list>

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
    onionlayout * layout; 
};


const EncDesc FULL_EncDesc = {
        {
            {oDET, SECLEVEL::DET},
            {oOPE, SECLEVEL::OPE},
            {oAGG, SECLEVEL::HOM},
            {oSWP, SECLEVEL::SEARCH},
        }
};

//initial onion configurations 
const EncDesc NUM_initial_levels = {
        {
            {oDET, SECLEVEL::DET},
            {oOPE, SECLEVEL::OPE},
            {oAGG, SECLEVEL::HOM},
        }
};

const EncDesc STR_initial_levels = {
        {
            {oDET, SECLEVEL::DET},
            {oOPE, SECLEVEL::OPE},
            {oSWP, SECLEVEL::SEARCH},
        }
};

typedef struct OnionMeta {
    std::string onionname;
    std::list<EncLayer *> layers; //first in list is lowest layer
} OnionMeta;

struct TableMeta;
//TODO: FieldMeta and TableMeta are partly duplicates with the original
// FieldMetadata an TableMetadata
// which contains data we want to add to this structure soon
typedef struct FieldMeta {
    TableMeta * tm; //point to table belonging in
    std::string fname;
    Create_field * sql_field;
    int index;

    //TODO: may want to integrate onions with encdesc for clarity
    std::map<onion, OnionMeta *> onions;
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

    std::string stringify();
    
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
    ~SchemaInfo() { tableMetaMap.clear();}
    TableMeta * getTableMeta(const std::string & table);
    FieldMeta * getFieldMeta(const std::string & table, const std::string & field);
} SchemaInfo;

