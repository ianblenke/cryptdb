#pragma once

/*
 * util.h
 *
 * A set of useful constants, data structures and functions.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <list>
#include <map>
#include <stdint.h>
#include <sys/time.h>
#include <vector>
#include <set>
#include <fstream>
#include <iostream>
#include <sstream>
#include <NTL/ZZ.h>

#include <util/errstream.hh>
#include <util/params.hh>
#include <util/util.hh>

#include <sql_select.h>
#include <sql_delete.h>
#include <sql_insert.h>
#include <sql_update.h>


// ==== CONSTANTS ============== //

#define SVAL2(s) #s
#define SVAL(s) SVAL2(s)


#define TN_SYM_KEY "varbinary(32)"
#define TN_PK_KEY  "varbinary(1220)"

#define psswdtable "activeusers"

const unsigned int bitsPerByte = 8;
const unsigned int bytesPerInt = 4;

const uint32_t MAX_UINT32_T = -1;
const uint64_t MAX_UINT64_T = -1;

const unsigned int SALT_LEN_BYTES = 8;

const unsigned int AES_BLOCK_BITS = 128;
const unsigned int AES_BLOCK_BYTES = AES_BLOCK_BITS/bitsPerByte;
const unsigned int AES_KEY_SIZE = 128;
const unsigned int AES_KEY_BYTES = AES_KEY_SIZE/bitsPerByte;

const unsigned int MASTER_KEY_SIZE = AES_KEY_SIZE; //master key

const unsigned int OPE_KEY_SIZE = AES_KEY_SIZE;

const unsigned int EncryptedIntSize = 128;

const unsigned int bytesOfTextForOPE = 20; //texts may be ordered
                                           // alphabetically -- this variable
                                           // indicates how many of the first
                                           // bytes should be used for sorting


const std::string PWD_TABLE_PREFIX = "pwdcryptdb__";

//maps the name of an annotation we want to process to the number of fields
// after this annotation relevant to it
const std::set<std::string> annotations =
{"enc", "search", "encfor", "equals", "givespsswd", "speaksfor"};

// ============= DATA STRUCTURES ===================================//

#if MYSQL_S
#include "mysql.h"
typedef MYSQL_RES DBResult_native;
#else
#include "libpq-fe.h"
typedef PGresult DBResult_native;
#endif



typedef struct AutoInc {
    AutoInc(std::string fieldval=""):incvalue(0), field(fieldval) {}
    my_ulonglong incvalue;
    std::string field;
} AutoInc;

class ResType {
 public:
    explicit ResType(bool okflag = true) : ok(okflag) {}

    bool ok;  // query executed successfully
    std::vector<std::string> names;
    std::vector<enum_field_types> types;
    std::vector<std::vector<Item*> > rows;
    AutoInc ai;
};

typedef enum class cmd {
    CREATE, UPDATE, INSERT, SELECT, DROP, DELETE, BEGIN,
    COMMIT, ALTER, TRAIN, OTHER
} command;

const std::string BASE_SALT_NAME = "cdb_salt";



typedef struct FieldsToDecrypt {
    std::list<std::string> OPEJoinFields;
    std::list<std::string> OPEFields;
    std::list<std::string> DETFields;
    std::list<std::string> DETJoinFields;
} FieldsToDecrypt;

class Operation {
 public:
    static bool isOp(const std::string &op);
    static bool isDET(const std::string &op);
    static bool isOPE(const std::string &op);
    static bool isILIKE(const std::string &op);
    static bool isIN(const std::string &op);

 private:
};

typedef struct OPESpec {
   std::string fieldname;
   unsigned int minv;
   unsigned int maxv;
} OPESpec;


typedef struct QueryMeta {
    std::map<std::string, std::string> tabToAlias, aliasToTab;
    std::list<std::string> tables;
    std::map<std::string, std::string> aliasToField;

    void cleanup();
} QueryMeta;

typedef struct ParseContext {

} ParseContext;

typedef struct Result {
    std::vector<std::vector<std::string> > a;
} Result;

typedef struct Predicate {
    std::string name;
    std::list<std::string> fields;
} Predicate;

/********* Data structures for multi-key CryptDB -- should not be used by
   single-principal ****/



typedef struct AccessRelation {
    AccessRelation(const std::string &hacc, const std::string &acct) {
        hasAccess = hacc;
        accessTo = acct;
    }
    std::string hasAccess;
    std::string accessTo;
} AccessRelation;


typedef struct AccessRelationComp {
  bool operator() (const AccessRelation& lhs, const AccessRelation& rhs) const {
         if (lhs.hasAccess < rhs.hasAccess) {
              return true;
          }
          if (lhs.hasAccess > rhs.hasAccess) {
              return false;
          }

          if (lhs.accessTo < rhs.accessTo) {
              return true;
          } else {
              return false;
          }
      }
} AccessRelationComp;

//permanent metadata for multi-key CryptDB - stores which field is encrypted
// for which field
typedef struct MultiKeyMeta {
    //e.g., msg_text encrypted for principal u.id
    std::map<std::string, std::string> encForMap;
    //contains an element if that element has some field encrypted to it
    std::map<std::string, bool > reverseEncFor;
    std::map<AccessRelation, Predicate *, AccessRelationComp> condAccess;     //maps a field having accessto to
                                             // any conditional predicate it
                                             // may have
    MultiKeyMeta() {}
    ~MultiKeyMeta() {
        for (auto i = condAccess.begin(); i != condAccess.end(); i++) {
           delete i->second;
        }
    }
} MKM;

//temporary metadata for multi-key CryptDB that belongs to the query or result
// being processed
typedef struct TempMKM {
    //maps a field (fullname) that has another field encrypted for it to its
    // value
    // groups.gid    23
    std::map<std::string, std::string> encForVal;

    //maps a field that has another field encrypted for it to the index in the
    // response std::list of values containing its value
    // groups.gid 5
    std::map<std::string, int> encForReturned;

    // contains fullnames of principals that were seen already in a response
    std::map<std::string, bool> principalsSeen;

    //true if current processing is query rather
    bool processingQuery;

    //some fields will be selected in order to be able to decrypt others, but
    // should not
    // be returned in the response to the application
    // maps position in raw DBMS response to whether it should be returned to
    // user or not
    std::map<unsigned int, bool> returnBitMap;
} TMKM;

//=============  Useful functions =========================//

bool
IsMySQLTypeNumeric(enum_field_types t);

// extracts (nobytes) bytes from int by placing the most significant bits at
// the end
std::string BytesFromInt(uint64_t value, unsigned int noBytes);
uint64_t IntFromBytes(const unsigned char * bytes, unsigned int noBytes);

void assert_s (bool value, const std::string &msg)
    throw (CryptDBError);
void myassert(bool value, const std::string &mess = "assertion failed");

double timeInSec(struct timeval tvstart, struct timeval tvend);

//parsing
const std::set<char> delimsStay = {'(', ')', '=', ',', '>', '<'};
const std::set<char> delimsGo   = {';', ' ', '\t', '\n'};
const std::set<char> keepIntact = {'\''};

#define NELEM(array) (sizeof((array)) / sizeof((array)[0]))
const std::set<std::string> commands =
    { "select", "create", "insert", "update", "delete", "drop", "alter" };
const std::set<std::string> aggregates = { "max", "min", "sum", "count" };
const std::set<std::string> createMetaKeywords = { "primary", "key", "unique" };
const std::set<std::string> comparisons = { ">", "<", "=" };

const std::string math[]=
{"+","-","(",")","*","/",".","0","1","2","3","4","5","6","7","8","9"};
const unsigned int noMath = NELEM(math);

std::string randomBytes(unsigned int len);
uint64_t randomValue();

std::string stringToByteInts(const std::string &s);
std::string angleBrackets(const std::string &s);
static inline std::string id_op(const std::string &x) { return x; }

char *
getCStr(const std::string & x);

/*
 * Turn a std::list (of type C) into a std::string, applying op to each element.
 * Handy ops are id_op, angleBrackets, and std::stringToByteInts.
 */
template<class C, class T>
std::string
toString(const C &l, T op)
{
    std::stringstream ss;
    bool first = true;
    for (auto i = l.begin(); i != l.end(); i++) {
        if (first)
            ss << "(";
        else
            ss << ", ";
        ss << op(*i);
        first = false;
    }
    ss << ")";
    return ss.str();
}

// tries to represent value in minimum no of bytes, avoiding the \0 character
std::string StringFromVal(uint64_t value, unsigned int padLen = 0);

NTL::ZZ UInt64_tToZZ (uint64_t value);

std::string StringFromZZ(const NTL::ZZ &x);
NTL::ZZ ZZFromString(const std::string &s);

std::string padForZZ(std::string s);

std::string StringFromZZFast(const NTL::ZZ& x);

void ZZFromStringFast(NTL::ZZ& x, const std::string& s);
void ZZFromBytesFast(NTL::ZZ& x, const unsigned char *p, long n);

inline NTL::ZZ ZZFromStringFast(const std::string& s) {
  NTL::ZZ z; ZZFromStringFast(z, s); return z;
}
inline NTL::ZZ ZZFromBytesFast(const unsigned char *p, long n) {
  NTL::ZZ z; ZZFromBytesFast(z, p, n); return z;
}

//rolls an interator forward
template<typename T> void
roll(typename std::list<T>::iterator & it,  int count)
{
    if (count < 0) {
        for (int i = 0; i < -count; i++) {
            it--;
        }
    }
    else {
        for (int i = 0; i < count; i++) {
            it++;
        }
    }
}

template<typename A, typename B>
B getAssert(const std::map<A, B> & m, const A & x, const std::string & str = "" ) {
    auto it = m.find(x);
    assert_s(it != m.end(), "item not present in map. " + str);
    return it->second;
}


template <typename T>
bool
isLastIterator(typename std::list<T>::iterator it,
               typename std::list<T>::iterator endit)
{
    roll<T>(it, 1);
    return it == endit;
}

//returns a Postgres bigint representation in std::string form for x
std::string strFromVal(uint64_t x);
std::string strFromVal(uint32_t x);



uint64_t valFromStr(const std::string & str);


//marshalls a binary value into characters readable by Postgres
std::string marshallBinary(const std::string &s);
/*
std::string  marshallSalt(const std::string & s);
std::string unmarshallSalt(const std::string & s);
*/


// unmarshalls a char * received from Postgres into a binary and
// sets newlen to the length of the result..
// marshall and unmarshallBinary are not inverses of each other.
// XXX why not?
std::string unmarshallBinary(const std::string &s);

void consolidate(std::list<std::string> & words);

/********* SQL QUERY PARSING ******/

// splits query in a std::list of std::string tokens; the tokens are obtained by
// splitting query at every character contained in delimsStay or delimsGo; the
// tokens will include the characters
// from delimsStay encountered
std::list<std::string> getSQLWords(const std::string &query);

//parses a given std::string str, by splitting it according to the delimiters in
// delimsStay, delimsGo ; if a piece of std::string
// is included in two keepIntact delimiters, it is not broken into pieces even
// if this std::string
//contains delimiters; delimsStay are kept in the result, delimsGo are
// discarded
std::list<std::string> parse(const std::string &str,
                   const std::set<char> &delimsStay,
                   const std::set<char> &delimsGo,
                   const std::set<char> &keepIntact);

command getCommand(const std::string &query)
    throw (CryptDBError);

//returns a std::string representing a value pointed to by it and advances it
std::string getVal(std::list<std::string>::iterator & it);


//acts only if the first field is "(";
//returns position after matching ")" mirroring all contents
std::string processParen(std::list<std::string>::iterator & it, const std::list<std::string> & words);


//returns the contents of str before the first encounter with c
std::string getBeforeChar(const std::string &str, char c);

//performs a case insensitive search
template<class T>
bool contains(const std::string &token, const T &values)
{
    for (auto i = values.begin(); i != values.end(); i++)
        if (equalsIgnoreCase(token, *i))
            return true;
    return false;
}

//performs a case insensitive search
bool isOnly(const std::string &token, const std::string * values, unsigned int noValues);

void addIfNotContained(const std::string &token, std::list<std::string> & lst);
void addIfNotContained(const std::string &token1, const std::string &token2,
                       std::list<std::pair<std::string, std::string> > & lst);

std::string removeApostrophe(const std::string &data);
bool hasApostrophe(const std::string &data);

std::string homomorphicAdd(const std::string &val1, const std::string &val2,
                      const std::string &valN2);

std::string toLowerCase(const std::string &token);

bool equalsIgnoreCase(const std::string &s1, const std::string &s2);


/**** HELPERS FOR EVAL **************/

std::string getQuery(std::ifstream & qFile);


class Timer {
 private:
    Timer(const Timer &t);  /* no reason to copy timer objects */

 public:
    Timer() { lap(); }

    //microseconds
    uint64_t lap() {
        uint64_t t0 = start;
        uint64_t t1 = cur_usec();
        start = t1;
        return t1 - t0;
    }

    //milliseconds
    double lap_ms() {
        return ((double)lap()) / 1000.0;
    }

 private:
    static uint64_t cur_usec() {
        struct timeval tv;
        gettimeofday(&tv, 0);
        return ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
    }

    uint64_t start;
};
