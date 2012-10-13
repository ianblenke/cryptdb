#include <algorithm>
#include <stdexcept>
#include <limits>

#include <execution/tuple.hh>
#include <parser/cdb_helpers.hh>
#include <util/pgoids.hh>

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

#define IMPL_VEC_ARG0_OPS(fcn) \
  do { \
    if (is_vector()) { \
      vector< db_elem > v; \
      v.reserve( _elems.size() ); \
      for (auto &e : _elems) v.push_back(e.fcn()); \
      return db_elem(v); \
    } \
  } while (0)

db_elem
db_elem::get_day() const
{
  IMPL_VEC_ARG0_OPS(get_day);
  requireType(TYPE_DATE);
  uint32_t m, d, y;
  extract_date_from_encoding(_d.i64, m, d, y);
  return db_elem(int64_t(d));
}

db_elem
db_elem::get_month() const
{
  IMPL_VEC_ARG0_OPS(get_month);
  requireType(TYPE_DATE);
  uint32_t m, d, y;
  extract_date_from_encoding(_d.i64, m, d, y);
  return db_elem(int64_t(m));
}

db_elem
db_elem::get_year() const
{
  IMPL_VEC_ARG0_OPS(get_year);
  requireType(TYPE_DATE);
  uint32_t m, d, y;
  extract_date_from_encoding(_d.i64, m, d, y);
  return db_elem(int64_t(y));
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

  SANITY(!pattern.empty());

  if (pos.size() == 1 && pos.front() == 0) {
    // pattern = "%..."
    size_t p = target.rfind(pattern.substr(1));
    return (p < target.size() && (target.size() - p) == (pattern.size() - 1));
  }

  if (pos.size() == 1 && pos.front() == (pattern.size() - 1)) {
    // pattern = "...%"
    return target.find(pattern.substr(0, pattern.size() - 1)) == 0;
  }

  if (pos.size() == 2 && pos.front() == 0 && pos.back() == (pattern.size() - 1)) {
    // pattern = "%...%"
    return target.find(pattern.substr(1, pattern.size() - 2)) != string::npos;
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
  if (_elems.empty()) throw no_data_exception("sum on empty vector");
  db_elem accum(0L);
  for (auto &e : _elems) accum = accum + e;
  return accum;
}

db_elem
db_elem::avg(bool distinct) const
{
  return sum(distinct) / db_elem(int64_t(size()));
}

db_elem
db_elem::filter(const db_elem& mask) const
{
  requireType(TYPE_VECTOR);
  mask.requireType(TYPE_VECTOR);
  if (_elems.size() != mask._elems.size()) {
    throw runtime_error("num mask elements does not match this elem");
  }
  vector< db_elem > elems;
  for (size_t i = 0; i < _elems.size(); i++) {
    if (mask[i]) elems.push_back(_elems[i]);
  }
  return db_elem(elems);
}

db_elem
db_elem::min() const
{
  requireType(TYPE_VECTOR);
  if (_elems.empty()) throw no_data_exception("min on empty vector");
  db_elem ret = _elems[0];
  for (auto it = _elems.begin() + 1; it != _elems.end(); ++it)
    if ((*it) < ret)
      ret = *it;
  return ret;
}

db_elem
db_elem::max() const
{
  requireType(TYPE_VECTOR);
  if (_elems.empty()) throw no_data_exception("max on empty vector");
  db_elem ret = _elems[0];
  for (auto it = _elems.begin() + 1; it != _elems.end(); ++it)
    if ((*it) > ret)
      ret = *it;
  return ret;
}

bool
db_elem::empty_mask() const
{
  requireType(TYPE_VECTOR);
  for (size_t i = 0; i < _elems.size(); i++) {
    if (_elems[i]) return false;
  }
  return true;
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
    case db_elem::TYPE_CHAR  :
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

string
db_elem::print(bool force_unsigned) const
{
  ostringstream o;
  o.precision(numeric_limits<double>::digits10);
  switch (_t) {
    case db_elem::TYPE_UNINIT: o << "UNINIT"; break;
    case db_elem::TYPE_NULL: o << "NULL"; break;
    case db_elem::TYPE_INT:
      if (force_unsigned) o << (uint64_t) _d.i64;
      else                o << _d.i64;
      break;
    case db_elem::TYPE_BOOL: o << (_d.b ? "true" : "false"); break;
    case db_elem::TYPE_DOUBLE: o << fixed << _d.dbl; break;
    case db_elem::TYPE_CHAR  :
    case db_elem::TYPE_STRING: o << _s; break;
    case db_elem::TYPE_DATE:
      {
        uint32_t day, month, year;
        extract_date_from_encoding((uint32_t) _d.i64, month, day, year);
        // TODO: format yyyy-mm-dd
        o << year << "-" << month << "-" << day;
      }
      break;
    case db_elem::TYPE_VECTOR:
      {
        size_t n = _elems.size() > 10 ? 10 : _elems.size();
        o << "{";
        for (size_t i = 0; i < n; i++) {
          o << _elems[i];
          if ((i+1) != n) o << ",";
          else if (n < _elems.size()) o << ",...";
        }
        o << "}";
      }
      break;
    default: assert(false);
  }
  return o.str();
}

ostream& operator<<(ostream& o, const db_elem& e)
{
  o << e.print(false);
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
