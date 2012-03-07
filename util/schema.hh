#include <util/onions.hh>

#include <parser/embedmysql.hh>
#include <parser/stringify.hh>


#include <string>
#include <map>

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


const EncDesc FULL_EncDesc = {
        {
            {oDET, SECLEVEL::SEMANTIC_DET},
            {oOPE, SECLEVEL::SEMANTIC_OPE},
            {oAGG, SECLEVEL::SEMANTIC_AGG},
            {oSWP, SECLEVEL::SWP         },
        }
};

const EncDesc NUMERIC_EncDec = {
        {
            {oDET, SECLEVEL::SEMANTIC_DET},
            {oOPE, SECLEVEL::SEMANTIC_OPE},
            {oAGG, SECLEVEL::SEMANTIC_AGG},
        }
};

const EncDesc EQ_SEARCH_EncDesc = {
        {
            {oDET, SECLEVEL::SEMANTIC_DET},
            {oOPE, SECLEVEL::SEMANTIC_OPE},
            {oSWP, SECLEVEL::SWP         },
        }
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
    int index;

    std::map<onion, std::string> onionnames;
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

