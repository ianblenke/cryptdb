#include <algorithm>
#include <stdexcept>

#include <execution/tuple.hh>
#include <parser/cdb_helpers.hh>

// values come from looking at: src/include/catalog/pg_type.h

#define BOOLOID			16
#define BYTEAOID		17
#define CHAROID			18
#define INT8OID			20
#define INT2OID			21
#define INT4OID			23
#define TEXTOID			25
#define FLOAT4OID 700
#define FLOAT8OID 701
#define BPCHAROID		1042
#define VARCHAROID		1043
#define DATEOID			1082
#define NUMERICOID		1700

using namespace std;

db_elem::type
db_elem::TypeFromPGOid(unsigned int oid) {
  switch (oid) {

    // int
    case INT8OID: case INT2OID: case INT4OID: return TYPE_INT;

    // bool
    case BOOLOID: return TYPE_BOOL;

    // double
    case FLOAT4OID: case FLOAT8OID: case NUMERICOID: return TYPE_DOUBLE;

    // string
    case BYTEAOID: case CHAROID: case TEXTOID: case BPCHAROID: case VARCHAROID: return TYPE_STRING;

    // date
    case DATEOID: return TYPE_DATE;

    default:
      cerr << "bad oid: " << oid << endl;
      throw runtime_error("could not handle oid");
  }
}

// find all occurances of char c in haystack
static void find_all_occur_of(
    const string& haystack,
    char c,
    vector<size_t>& pos)
{
  for (size_t i = 0; i < haystack.size(); i++) {
    if (haystack[i] == c) pos.push_back(i);
  }
}

static inline string lower_case(const string& target)
{
  string r;
  r.resize(target.size());
  transform(target.begin(), target.end(), r.begin(), ::tolower);
  return r;
}

db_elem
db_elem::like(const db_elem& qstr, bool case_sensitive) const
{
  requireType(TYPE_STRING);
  qstr.requireType(TYPE_STRING);

  string target = _s;
  string pattern = qstr._s;

  if (!case_sensitive) {
    target = lower_case(target);
    pattern = lower_case(pattern);
  }

  // check for special cases
  // qstr = "%..." or qstr = "...%" or qstr = "%...%"

  vector<size_t> pos;
  find_all_occur_of(pattern, '%', pos);

  if (pos.empty()) {
    return db_elem(target == pattern);
  }

  assert(!pattern.empty());

  if (pos.size() == 1 && pos.front() == 0) {
    // pattern = "%..."
    size_t p = target.rfind(pattern.substr(1));
    return (p < target.size() && (target.size() - p) == (pattern.size() - 1));
  }

  if (pos.size() == 1 && pos.front() == (pattern.size() - 1)) {
    // pattern = "...%"
    return target.find(pattern.substr(0, pattern.size())) == 0;
  }

  if (pos.size() == 2 && pos.front() == 0 && pos.back() == (pattern.size() - 1)) {
    // pattern = "%...%"
    return target.find(pattern.substr(1, pattern.size())) != string::npos;
  }

  // otherwise, punt for now
  throw runtime_error("pattern unimplemented for now");
}

db_elem
db_elem::count(bool distinct) const
{
  if (distinct) throw runtime_error("UNIMPL");
  requireType(TYPE_VECTOR);
  return int64_t(_elems.size());
}

db_elem
db_elem::sum(bool distinct) const
{
  if (distinct) throw runtime_error("UNIMPL");
  requireType(TYPE_VECTOR);
  if (_elems.empty()) throw runtime_error("sum on empty vector");
  db_elem accum(0L);
  for (auto &e : _elems) accum = accum + e;
  return accum;
}

string
db_elem::sqlify(bool force_unsigned) const
{
  ostringstream o;
  switch (_t) {
    case db_elem::TYPE_NULL: o << "NULL"; break;
    case db_elem::TYPE_INT:
      if (force_unsigned) o << (uint64_t) _d.i64;
      else                o << _d.i64;
      break;
    case db_elem::TYPE_BOOL: o << (_d.b ? "t" : "f"); break;
    case db_elem::TYPE_DOUBLE: o << _d.dbl; break;
    case db_elem::TYPE_STRING:
      {
        // This gets around the escaping issues for now
        string hex = to_hex(_s);
        o << "decode('" << hex << "', 'hex')";
      }
      break;
    case db_elem::TYPE_DATE:
      {
        uint32_t day, month, year;
        extract_date_from_encoding((uint32_t) _d.i64, month, day, year);
        o << "'" << year << "-" << month << "-" << day << "'";
      }
      break;
    default: assert(false);
  }
  return o.str();
}

ostream& operator<<(ostream& o, const db_elem& e)
{
  switch (e._t) {
    case db_elem::TYPE_UNINIT: o << "UNINIT"; break;
    case db_elem::TYPE_NULL: o << "NULL"; break;
    case db_elem::TYPE_INT: o << e._d.i64; break;
    case db_elem::TYPE_BOOL: o << (e._d.b ? "true" : "false"); break;
    case db_elem::TYPE_DOUBLE: o << e._d.dbl; break;
    case db_elem::TYPE_STRING: o << e._s; break;
    case db_elem::TYPE_DATE:
      {
        uint32_t day, month, year;
        extract_date_from_encoding((uint32_t) e._d.i64, month, day, year);
        // TODO: format yyyy-mm-dd
        o << year << "-" << month << "-" << day;
      }
      break;
    case db_elem::TYPE_VECTOR:
      // TODO: impl
      o << "<vector>";
      break;
    default: assert(false);
  }
  return o;
}

ostream& operator<<(ostream& o, const db_tuple& tuple)
{
  for (size_t i = 0; i < tuple.columns.size(); i++) {
    o << tuple.columns[i];
    if ((i + 1) != tuple.columns.size()) o << "|";
  }
  return o;
}
