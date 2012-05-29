#include <stdexcept>

#include <execution/tuple.hh>

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
      throw runtime_error("could not handle oid");
  }
}

std::ostream& operator<<(std::ostream& o, const db_elem& e) {
  switch (e._t) {
    case db_elem::TYPE_UNINIT: o << "NULL"; break;
    case db_elem::TYPE_INT: o << e._d.i64; break;
    case db_elem::TYPE_BOOL: o << e._d.b; break;
    case db_elem::TYPE_DOUBLE: o << e._d.dbl; break;
    case db_elem::TYPE_STRING: o << e._s; break;
    case db_elem::TYPE_DATE:
      {
        // TODO: this is copied from parser/cdb_helpers.hh, b/c we don't want
        // to create a dependency on parser if not really necessary

        static const uint32_t DayMask = 0x1F;
        static const uint32_t MonthMask = 0x1E0;
        static const uint32_t YearMask = ((uint32_t)-1) << 9;

        uint32_t encoding = (uint32_t) e._d.i64;

        uint32_t day = encoding & DayMask;
        uint32_t month = (encoding & MonthMask) >> 5;
        uint32_t year = (encoding & YearMask) >> 9;

        // TODO: format yyyy-mm-dd
        o << year << "-" << month << "-" << day;
      }
      break;
    case db_elem::TYPE_VECTOR:
      // TODO: impl
      o << "<vector>";
      break;
  }
  return o;
}
