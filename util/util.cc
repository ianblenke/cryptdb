#include <algorithm>
#include <string>
#include <iomanip>
#include <stdexcept>
#include <assert.h>

#include <gmp.h>

#include <openssl/rand.h>

#include <util/util.hh>
#include <util/cryptdb_log.hh>


using namespace std;
using namespace NTL;

void
myassert(bool value, const string &mess)
{
    if (ASSERTS_ON) {
        if (!value) {
            LOG(warn) << "ERROR: " << mess;
            throw std::runtime_error(mess);
        }
    }
}

void
assert_s (bool value, const string &msg)
throw (CryptDBError)
{
    if (ASSERTS_ON) {
        if (!value) {
            LOG(warn) << "ERROR: " << msg;
            throw CryptDBError(msg);
        }
    }
}

bool
IsMySQLTypeNumeric(enum_field_types t) {
    switch (t) {
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_NEWDECIMAL:

        // numeric also includes dates for now,
        // since it makes sense to do +/- on date types
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_NEWDATE:
            return true;
        default: return false;
    }
}


double
timeInSec(struct timeval tvstart, struct timeval tvend)
{
    return
        ((double) (tvend.tv_sec - tvstart.tv_sec) +
         ((double) (tvend.tv_usec - tvstart.tv_usec)) / 1000000.0);
}

string
randomBytes(unsigned int len)
{
    string s;
    s.resize(len);
    RAND_bytes((uint8_t*) &s[0], len);
    return s;
}

uint64_t
randomValue()
{
    return IntFromBytes((const uint8_t*) randomBytes(8).c_str(), 8);
}

string
stringToByteInts(const string &s)
{
    stringstream ss;
    bool first = true;
    for (size_t i = 0; i < s.length(); i++) {
        if (!first)
            ss << " ";
        ss << (uint) (uint8_t) s[i];
        first = false;
    }
    return ss.str();
}

string
angleBrackets(const string &s)
{
    return "<" + s + ">";
}

string
BytesFromInt(uint64_t value, unsigned int noBytes)
{
    string result;
    result.resize(noBytes);

    for (uint i = 0; i < noBytes; i++) {
        result[noBytes-i-1] = ((unsigned char) value) % 256;
        value = value / 256;
    }

    return result;
}

uint64_t
IntFromBytes(const unsigned char * bytes, unsigned int noBytes)
{
    uint64_t value = 0;

    for (unsigned int i = 0; i < noBytes; i++) {
        unsigned int bval = (unsigned int)bytes[i];
        value = value * 256 + bval;
    }

    return value;
}

uint64_t
uint64FromZZ(ZZ val)
{
    uint64_t res = 0;
    uint64_t mul = 1;
    while (val > 0) {
        res = res + mul*(to_int(val % 10));
        mul = mul * 10;
        val = val / 10;
    }
    return res;
}




string
StringFromZZ(const ZZ &x)
{
    string s;
    s.resize(NumBytes(x), 0);
    BytesFromZZ((uint8_t*) &s[0], x, s.length());
    return s;
}

ZZ
ZZFromString(const string &s)
{
    return ZZFromBytes((const uint8_t *) s.data(), s.length());
}

// based on _ntl_gcopy from g_lip_impl.h
// we duplicate this so we can avoid having to do a copy
#define ALLOC(p) (((long *) (p))[0])
#define SIZE(p) (((long *) (p))[1])
#define DATA(p) ((mp_limb_t *) (((long *) (p)) + 2))
#define STORAGE(len) ((long)(2*sizeof(long) + (len)*sizeof(mp_limb_t)))
#define MustAlloc(c, len)  (!(c) || (ALLOC(c) >> 2) < (len))

static void _ntl_gcopy_mp(mp_limb_t* a, long sa, _ntl_gbigint *bb)
{
   _ntl_gbigint b;
   long abs_sa, i;
   mp_limb_t *adata, *bdata;

   b = *bb;

   if (!a || sa == 0) {
      if (b) SIZE(b) = 0;
   }
   else {
      if (a != b) {
         if (sa >= 0)
            abs_sa = sa;
         else
            abs_sa = -sa;

         if (MustAlloc(b, abs_sa)) {
            _ntl_gsetlength(&b, abs_sa);
            *bb = b;
         }

         adata = a;
         bdata = DATA(b);

         for (i = 0; i < abs_sa; i++)
            bdata[i] = adata[i];

         SIZE(b) = sa;
      }
   }
}

#ifndef NTL_GMP_LIP
#error "NTL is not compiled with GMP"
#endif

string padForZZ(string s) {
    if (s.size() % sizeof(mp_limb_t) == 0) {return s;}
    int n = ((s.size()/sizeof(mp_limb_t)) + 1)*sizeof(mp_limb_t);
    s.resize(n);
    return s;
}

string StringFromZZFast(const ZZ& x) {
    long sa = SIZE(x.rep);
    long abs_sa;
    if (sa >= 0)
      abs_sa = sa;
    else
      abs_sa = -sa;
    mp_limb_t* data = DATA(x.rep);
    return string((char *)data, abs_sa * sizeof(mp_limb_t));
}

void ZZFromStringFast(ZZ& x, const string& s) {
    assert(s.size() % sizeof(mp_limb_t) == 0);
    _ntl_gcopy_mp(
        (mp_limb_t*) s.data(),
        s.size() / sizeof(mp_limb_t),
        &x.rep);
}

//Faster function, if p is 8-byte aligned; if not go for the slower one
// TODO: figure out how to pad non-8-byte aligned p
void ZZFromBytesFast(ZZ& x, const unsigned char *p, long n) {
    if (n % sizeof(mp_limb_t) != 0) {
        x = ZZFromBytes((const uint8_t *) p, n); 
        return;
    }
    _ntl_gcopy_mp(
        (mp_limb_t*) p,
        n / sizeof(mp_limb_t),
        &x.rep);
}

ZZ
ZZFromUint64 (uint64_t value)
{
    unsigned int unit = 256;
    ZZ power;
    power = 1;
    ZZ res;
    res = 0;

    while (value > 0) {
        res = res + ((long int)value % unit) * power;
        power = power * unit;
        value = value / unit;
    }
    return res;

};

char *
getCStr(const std::string & x) {
    char *  res = (char *)malloc(x.size());
    memcpy(res, x.c_str(), x.size());
    return res;
}

string
strFromVal(uint64_t x)
{
    stringstream ss;
    ss <<  x;
    return ss.str();
}

string
strFromVal(uint32_t x)
{
    stringstream ss;
    ss << (int32_t) x;
    return ss.str();
}

string
StringFromVal(uint64_t value, uint padLen)
{
    stringstream ss;
    ss << setfill('0') << setw(padLen) << value;
    return ss.str();
}

//Transforms a string that contains an integer (in decimal form) into the
//integer
// Does not work as wanted if str codifies a number in binary.
uint64_t
valFromStr(const string &str)
{
   stringstream ss(str);
   uint64_t val;
   ss >> val;
        
    return  val;
}

string
marshallBinary(const string &s)
{
    string r;
    r += "X\'";

    const char *sp = &s[0];
    static const char *digits = "0123456789ABCDEF";
    size_t l = s.length();
    for (size_t i = 0; i < l; i++) {
        r += digits[sp[i] >> 4 & 0xf];
        r += digits[sp[i] & 0xf];
    }

    r += "\'";
    return r;
}


static unsigned char
getFromHex(const string &hexValues)
{
    unsigned int v;
    sscanf(hexValues.c_str(), "%2x", &v);
    return (unsigned char) v;
}

string
unmarshallBinary(const string &s)
{
    uint offset;
    size_t len = s.length();

#if MYSQL_S
    offset = 2;
    myassert(s[0] == 'X',
             "unmarshallBinary: first char is not x; it is " + s[0]);
    len = len - 1;     // removing last apostrophe
#else
    myassert(s[0] == '\\',
             "unmarshallBinary: first char is not slash; it is " + s[0]);
    myassert(s[1] == 'x',
             "unmarshallBinary: second char is not x; it is " + s[1]);
    offset = 2;
#endif

    myassert((len - offset) % 2 == 0,
             "unmarshallBinary: newlen is odd! newlen is " +
             strFromVal(len-offset));

    string r;
    for (uint i = 0; i < (len-offset)/2; i++)
        r += getFromHex(&s[offset+i*2]);
    return r;
}



list<std::string>
split(string s, const char * separators) {
    char* tok = strtok((char *)s.c_str(), separators);
    list<std::string> parts = list<std::string>();
    while (tok != NULL) {
        parts.push_back(string(tok));
        tok = strtok(NULL, ",");
    }
    return parts;
}

string
getQuery(ifstream & qFile)
{

    string query = "";
    string line = "";

    while ((!qFile.eof()) && (line.find(';') == string::npos)) {
        getline(qFile, line);
        query = query + line;
    }


    return query;

}

string
getBeforeChar(const string &str, char c)
{
    size_t pos = str.find(c);
    if (pos != string::npos) {
        return str.substr(0, pos);
    } else {
        return "";
    }
}


bool
isOnly(const string &token, const string * values, unsigned int noValues)
{
    for (unsigned int j = 0; j < token.size(); j++) {
        bool is_value = false;
        for (unsigned int i = 0; i < noValues; i++) {
            string test = "";
            test += token[j];
            if (equalsIgnoreCase(test, values[i])) {
                is_value = true;
            }
        }
        if (!is_value) {
            cerr << token[j] << endl;
            return false;
        }
    }
    return true;
}

static bool
contains(const string &token1, const string &token2, list<pair<string,
                                                               string> > &
         lst)
{
    for (auto it = lst.begin(); it != lst.end(); it++)
        if ((it->first.compare(token1) == 0) &&
            (it->second.compare(token2) == 0))
            return true;

    return false;
}

void
addIfNotContained(const string &token, list<string> & lst)
{
    if (!contains(token, lst)) {
        lst.push_back(token);
    }
}

void
addIfNotContained(const string &token1, const string &token2,
                  list<pair<string,
                            string> >
                  & lst)
{

    if (!contains(token1, token2, lst)) {
        lst.push_back(pair<string, string>(token1, token2));
    }
}

string
removeApostrophe(const string &data)
{
    if (data[0] == '\'') {
        assert_s(data[data.length()-1] == '\'', "not matching ' ' \n");
        return data.substr(1, data.length() - 2);
    } else {
        return data;
    }
}

bool
hasApostrophe(const string &data)
{
    return ((data[0] == '\'') && (data[data.length()-1] == '\''));
}

string
homomorphicAdd(const string &val1, const string &val2, const string &valn2)
{
    ZZ z1 = ZZFromStringFast(padForZZ(val1));
    ZZ z2 = ZZFromStringFast(padForZZ(val2));
    ZZ n2 = ZZFromStringFast(padForZZ(valn2));
    ZZ res = MulMod(z1, z2, n2);
    return StringFromZZ(res);
}

string
toLowerCase(const string &token)
{
    string s = token;
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

bool
equalsIgnoreCase(const string &s1, const string &s2)
{
    return toLowerCase(s1) == toLowerCase(s2);
}

// Various test OPE stuff


vector<uint64_t>
get_random(uint no_test_vals) {
    vector<uint64_t> vals;
    //to make debugging deterministic
    srand(time(NULL));
    
    uint elems_inserted = 0;
    // test on random values
    for(uint i=0; i< no_test_vals; i++){
	uint64_t val = rand() % 100000;
	// only insert unique elements
	if (!mcontains(vals, val)) {
	    vals.push_back(val);
		elems_inserted++;
	} else {
	    i--;
	    cerr << "value still repeats\n";
	}
    }
    
    return vals;    
}

 void
check_good_height(uint maxheight, uint no_elems, uint breadth) {
    assert_s(maxheight <= (log(no_elems*1.0)/log(breadth*1.0)) + 1, "tree is too high");
}
