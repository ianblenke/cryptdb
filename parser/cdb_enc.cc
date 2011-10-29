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
    return string("X'") + to_hex(t) + string("'");
}

static void do_encrypt(size_t i,
                       datatypes dt,
                       const string &plaintext,
                       vector<string> &enccols,
                       CryptoManager &cm) {

    switch (dt) {
    case DT_INTEGER:
        {
            // DET, OPE, AGG, SALT
            bool isBin;
            string encDET = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                     fieldname(i, "DET"),
                                     getMin(oDET), SECLEVEL::DET, isBin, 12345);
            enccols.push_back(to_s(valFromStr(encDET)));

            string encOPE = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                     fieldname(i, "OPE"),
                                     getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
            enccols.push_back(to_s(valFromStr(encOPE)));

            string encAGG = cm.crypt(cm.getmkey(), plaintext, TYPE_INTEGER,
                                     fieldname(i, "AGG"),
                                     getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
            enccols.push_back(to_mysql_hex(encAGG));

            // salt
            enccols.push_back(to_s(12345));
        }
        break;
    case DT_FLOAT:
        {
            // TODO: fix precision
            double f = atof(plaintext.c_str());
            uint64_t t = (uint64_t) (f * 100.0);
            do_encrypt(i, DT_INTEGER, to_s(t), enccols, cm);
        }
        break;
    case DT_STRING:
        {
            // DET, OPE, SALT
            bool isBin;
            string encDET = cm.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                     fieldname(i, "DET"),
                                     getMin(oDET), SECLEVEL::DET, isBin, 12345);
            enccols.push_back(to_mysql_hex(encDET));

            string encOPE = cm.crypt(cm.getmkey(), plaintext, TYPE_TEXT,
                                     fieldname(i, "OPE"),
                                     getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
            enccols.push_back(to_s(valFromStr(encOPE)));

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

            // year_DET, year_OPE, year_AGG
            {
                bool isBin;
                string encDET = cm.crypt(cm.getmkey(), to_s(year), TYPE_INTEGER,
                                         fieldname(i, "year_DET"),
                                         getMin(oDET), SECLEVEL::DET, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encDET)));

                string encOPE = cm.crypt(cm.getmkey(), to_s(year), TYPE_INTEGER,
                                         fieldname(i, "year_OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encOPE)));
                cerr << fieldname(i, "year_OPE") << ": " << valFromStr(encOPE) << endl;

                string encAGG = cm.crypt(cm.getmkey(), to_s(year), TYPE_INTEGER,
                                         fieldname(i, "year_AGG"),
                                         getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
                enccols.push_back(to_mysql_hex(encAGG));
            }

            // month_DET, month_OPE, month_AGG
            {
                bool isBin;
                string encDET = cm.crypt(cm.getmkey(), to_s(month), TYPE_INTEGER,
                                         fieldname(i, "month_DET"),
                                         getMin(oDET), SECLEVEL::DET, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encDET)));

                string encOPE = cm.crypt(cm.getmkey(), to_s(month), TYPE_INTEGER,
                                         fieldname(i, "month_OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encOPE)));

                string encAGG = cm.crypt(cm.getmkey(), to_s(month), TYPE_INTEGER,
                                         fieldname(i, "month_AGG"),
                                         getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
                enccols.push_back(to_mysql_hex(encAGG));
            }

            // day_DET, day_OPE, day_AGG
            {
                bool isBin;
                string encDET = cm.crypt(cm.getmkey(), to_s(day), TYPE_INTEGER,
                                         fieldname(i, "day_DET"),
                                         getMin(oDET), SECLEVEL::DET, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encDET)));

                string encOPE = cm.crypt(cm.getmkey(), to_s(day), TYPE_INTEGER,
                                         fieldname(i, "day_OPE"),
                                         getMin(oOPE), SECLEVEL::OPE, isBin, 12345);
                enccols.push_back(to_s(valFromStr(encOPE)));

                string encAGG = cm.crypt(cm.getmkey(), to_s(day), TYPE_INTEGER,
                                         fieldname(i, "day_AGG"),
                                         getMin(oAGG), SECLEVEL::SEMANTIC_AGG, isBin, 12345);
                enccols.push_back(to_mysql_hex(encAGG));
            }

            // salt
            enccols.push_back(to_s(12345));
        }
        break;
    }
}

int main(int argc, char **argv) {
    CryptoManager cm("12345");
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

    static const size_t n_schema = sizeof(schema) / sizeof(schema[0]);

    for (;;) {
        string s;
        getline(cin, s);
        if (!cin.good())
            break;

        vector<string> tokens;
        tokenize(s, "|", tokens);

        assert(tokens.size() >= n_schema);

        vector<string> columns;
        for (size_t i = 0; i < n_schema; i++) {
            do_encrypt(i, schema[i], tokens[i], columns, cm);
        }

        string out;
        join(columns, "|", out);
        cout << out << endl;
    }
    return 0;
}
