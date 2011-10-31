#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <list>
#include <algorithm>

#include <unistd.h>
#include <parser/cdb_rewrite.hh>

using namespace std;

static void tokenize(const string &s,
                     const string &delim,
                     vector<string> &tokens) {

    size_t i = 0;
    while (true) {
        size_t p = s.find(delim, i);
        tokens.push_back(
                s.substr(
                    i, (p == string::npos ? s.size() : p) - i));
        if (p == string::npos) break;
        i = p + delim.size();
    }
}

static void join(const vector<string> &tokens,
                 const string &sep,
                 string &p) {
    ostringstream s;
    for (size_t i = 0; i < tokens.size(); i++) {
        s << tokens[i];
        if (i != tokens.size() - 1) s << sep;
    }
    p = s.str();
}

typedef enum datatypes {
    DT_INTEGER,
    DT_FLOAT,
    DT_STRING,
    DT_DATE,
} datatypes;

static string fieldname(size_t fieldnum, const string &suffix) {
    ostringstream s;
    s << "field" << fieldnum << suffix;
    return s.str();
}

template <typename T>
inline string to_s(const T& t) {
    ostringstream s;
    s << t;
    return s.str();
}

template <typename T>
inline string to_hex(const T& t) {
    ostringstream s;
    s << hex << t;
    return s.str();
}

static const char* const lut = "0123456789ABCDEF";

template <>
inline string to_hex(const string& input) {
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i) {
        const unsigned char c = (unsigned char) input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

template <typename T>
static inline string to_mysql_hex(T t) {
    ostringstream buf;
    buf << "X'" << to_hex(t) << "'";
    return buf.str();
}

static inline long roundToLong(double x) {
    return ((x)>=0?(long)((x)+0.5):(long)((x)-0.5));
}

enum onion_bitmask {
    ONION_DET = 0x1,
    ONION_OPE = 0x1 << 1,
    ONION_AGG = 0x1 << 2,
};

static inline bool DoDET(int m) { return m & ONION_DET; }
static inline bool DoOPE(int m) { return m & ONION_OPE; }
static inline bool DoAGG(int m) { return m & ONION_AGG; }

static void do_encrypt(size_t i,
                       datatypes dt,
                       int onions,
                       const string &plaintext,
                       vector<string> &enccols,
                       CryptoManager &cm) {

    switch (dt) {
    case DT_INTEGER:
        {
            // DET, OPE, AGG, SALT
            bool isBin;
            if (DoDET(onions)) {
                string encDET = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         fieldname(i, "DET"),
                                         getMin(oDET), SECLEVEL::DET, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encDET)));
            } else {
                enccols.push_back("NULL");
            }

            if (DoOPE(onions)) {
                string encOPE = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         fieldname(i, "OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encOPE)));
            } else {
                enccols.push_back("NULL");
            }

            if (DoAGG(onions)) {
                string encAGG = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                         fieldname(i, "AGG"),
                                         getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
                enccols.push_back(to_mysql_hex(encAGG));
            } else {
                enccols.push_back("NULL");;
            }

            // salt
            enccols.push_back(to_s(12345));
        }
        break;
    case DT_FLOAT:
        {
            // TODO: fix precision
            double f = strtod(plaintext.c_str(), NULL);
            long t = roundToLong(f * 100.0);
            do_encrypt(i, DT_INTEGER, onions, to_s(t), enccols, cm);
        }
        break;
    case DT_STRING:
        {
            // DET, OPE, SALT
            bool isBin;
            if (DoDET(onions)) {
                string encDET = cm.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                         fieldname(i, "DET"),
                                         getMin(oDET), SECLEVEL::DET, isBin, 12345);
                assert((encDET.size() % 16) == 0);
                enccols.push_back(to_mysql_hex(encDET));
            } else {
                enccols.push_back("NULL");
            }

            if (DoOPE(onions)) {
                string encOPE = cm.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                         fieldname(i, "OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encOPE)));
            } else {
                enccols.push_back("NULL");
            }

            // salt
            enccols.push_back(to_s(12345));
        }
        break;
    case DT_DATE:
        {
            // TODO: don't assume yyyy-mm-dd format
            int year, month, day;
            int ret = sscanf(plaintext.c_str(), "%d-%d-%d", &year, &month, &day);
            assert(ret == 3);

#define __IMPL_FIELD_ENC(field) \
            do { \
                bool isBin; \
                if (DoDET(onions)) { \
                    string encDET = cm.crypt(cm.getmkey(), to_s(field), TYPE_INTEGER, \
                                             fieldname(i, #field "_DET"), \
                                             getMin(oDET), SECLEVEL::DET, isBin, 12345); \
                    enccols.push_back(to_s(valFromStr(encDET))); \
                } else { \
                    enccols.push_back("NULL"); \
                } \
                if (DoOPE(onions)) { \
                    string encOPE = cm.crypt(cm.getmkey(), to_s(field), TYPE_INTEGER, \
                                             fieldname(i, #field "_OPE"), \
                                             getMin(oOPE), SECLEVEL::OPE, isBin, 12345); \
                    enccols.push_back(to_s(valFromStr(encOPE))); \
                } else { \
                    enccols.push_back("NULL"); \
                } \
                if (DoAGG(onions)) { \
                    string encAGG = cm.crypt(cm.getmkey(), to_s(field), TYPE_INTEGER, \
                                             fieldname(i, #field "_AGG"), \
                                             getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345); \
                    enccols.push_back(to_mysql_hex(encAGG)); \
                } else { \
                    enccols.push_back("NULL"); \
                } \
            } while (0)

            // year_DET, year_OPE, year_AGG
            __IMPL_FIELD_ENC(year);
            // month_DET, month_OPE, month_AGG
            __IMPL_FIELD_ENC(month);
            // day_DET, day_OPE, day_AGG
            __IMPL_FIELD_ENC(day);

#undef __IMPL_FIELD_ENC

            // salt
            enccols.push_back(to_s(12345));
        }
        break;
    }
}


struct input_output_t {
    vector<string> input;
    vector<string> output;
};

static inline string process_input(const string &s, CryptoManager &cm) {
    static const datatypes schema[] = {
        DT_INTEGER,
        DT_INTEGER,
        DT_INTEGER,
        DT_INTEGER,

        DT_FLOAT,
        DT_FLOAT,
        DT_FLOAT,
        DT_FLOAT,

        DT_STRING,
        DT_STRING,

        DT_DATE,
        DT_DATE,
        DT_DATE,

        DT_STRING,
        DT_STRING,
        DT_STRING,
    };

    static const unsigned int onion[] = {
        0,
        0,
        0,
        0,

        ONION_DET | ONION_OPE,
        ONION_DET | ONION_OPE,
        ONION_DET | ONION_OPE,
        ONION_DET | ONION_OPE,

        ONION_DET | ONION_OPE,
        ONION_DET | ONION_OPE,

        ONION_OPE,
        0,
        0,

        0,
        0,
        0,
    };

    static const size_t n_schema = sizeof(schema) / sizeof(schema[0]);
    static const size_t n_onion  = sizeof(onion)  / sizeof(onion [0]);
    assert(n_schema == n_onion);

    vector<string> tokens;
    tokenize(s, "|", tokens);

    assert(tokens.size() >= n_schema);

    vector<string> columns;
    for (size_t i = 0; i < n_schema; i++) {
        do_encrypt(i, schema[i], onion[i], tokens[i], columns, cm);
    }

    string out;
    join(columns, "|", out);
    return out;
}

static inline size_t GetNumProcessors() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}

int main(int argc, char **argv) {
    CryptoManager cm("12345");
    for (;;) {
        string s;
        getline(cin, s);
        if (!cin.good())
            break;
        try {
            cout << process_input(s, cm) << endl;
        } catch (...) {
            cerr << "Input line failed:" << endl
                 << "  " << s << endl;
        }
    }
    return 0;
}
