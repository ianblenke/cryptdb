#pragma once

#include <cstdint>

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

#include <util/onions.hh>

/** represents an element from the database. currently not very efficient */
class db_elem {
public:

  friend std::ostream& operator<<(std::ostream&, const db_elem&);

  enum null_type {
    null_tag,
  };

  db_elem() : _t(TYPE_UNINIT) {}
  db_elem(null_type t) : _t(TYPE_NULL) {}

  db_elem(int64_t i) : _t(TYPE_INT) { _d.i64 = i; }
  db_elem(bool b) : _t(TYPE_BOOL) { _d.b = b; }
  db_elem(double d) : _t(TYPE_DOUBLE) { _d.dbl = d; }
  db_elem(const std::string& s) : _t(TYPE_STRING), _s(s) {}
  db_elem(const char *s, size_t n) : _t(TYPE_STRING), _s(s, n) {}

  db_elem(unsigned int y, unsigned int m, unsigned int d)
    : _t(TYPE_DATE) { _d.i64 = (d | (m << 5) | (y << 9)); }

  db_elem(const std::vector< db_elem >& elems)
    : _t(TYPE_VECTOR), _elems(elems) {}

  enum type {
    TYPE_UNINIT,

    TYPE_NULL, // null has to be explicitly initialized as null

    TYPE_BOOL,
    TYPE_INT,
    TYPE_DOUBLE,

    TYPE_STRING,
    TYPE_DATE,
    TYPE_VECTOR,

    TYPE_DECIMAL_15_2, // XXX: hack! this is used to represent DECIMAL(15, 2) from TPC-H.
                       // we don't handle it internally- it is simply a tag so that our
                       // encrypt/decrypt functions do the right thing
  };

  inline type get_type() const { return _t; }
  inline bool is_inited() const { return _t != TYPE_UNINIT; }
  inline bool is_uninit() const { return _t == TYPE_UNINIT; }

  inline std::string stringify() const {
    std::ostringstream oss;
    oss << *this;
    return oss.str();
  }

  // just a hack for now
  std::string sqlify(bool force_unsigned) const;

private:
  inline static bool IsNumericType(type t) {
    return t >= TYPE_BOOL && t <= TYPE_DOUBLE;
  }

  void checkNumeric() const {
    if (!IsNumericType(_t)) {
      throw std::runtime_error("non-numeric type");
    }
  }

  void requireType(type t) const {
    if (_t != t) {
      std::cerr << "expected (" << t << "), got (" << _t << ")" << std::endl;
      throw std::runtime_error("not required type");
    }
  }

  // this must be a numeric type and promotable
  // for now, we don't handle nulls correctly
  db_elem promote(type t) const {
    assert(IsNumericType(t));
    assert(IsNumericType(_t));
    assert(_t < t);
    switch (_t) {
      case TYPE_BOOL:
        switch (t) {
          case TYPE_INT:    return db_elem(int64_t(_d.b ? 1 : 0));
          case TYPE_DOUBLE: return db_elem(double(_d.b ? 1.0 : 0.0));
          default: assert(false);
        }
      case TYPE_INT:
        assert(t == TYPE_DOUBLE);
        return db_elem( (double) _d.i64 );
      default: assert(false);
    }
    throw std::runtime_error("unreachable");
  }

public:

  // C++ operators

#define VECTOR_OPS_IMPL(op) \
  if (_t == TYPE_VECTOR && rhs._t == TYPE_VECTOR) { \
    if (_elems.size() != rhs._elems.size()) { \
      throw std::runtime_error("vectors not same size"); \
    } \
    std::vector< db_elem > elems; elems.reserve(_elems.size()); \
    for (size_t i = 0; i < _elems.size(); i++) { \
      elems.push_back(_elems[i] op rhs._elems[i]); \
    } \
    return db_elem(elems); \
  } else if (_t == TYPE_VECTOR) { \
    std::vector< db_elem > elems; elems.reserve(_elems.size()); \
    for (size_t i = 0; i < _elems.size(); i++) { \
      elems.push_back(_elems[i] op rhs); \
    } \
    return db_elem(elems); \
  } else if (rhs._t == TYPE_VECTOR) { \
    std::vector< db_elem > elems; elems.reserve(rhs._elems.size()); \
    for (size_t i = 0; i < rhs._elems.size(); i++) { \
      elems.push_back((*this) op rhs._elems[i]); \
    } \
    return db_elem(elems); \
  }

#define NUMERIC_BINOP_BODY(op) \
  if (_t == rhs._t) { \
    switch (_t) { \
      case TYPE_BOOL:   return db_elem(bool(_d.b op rhs._d.b)); \
      case TYPE_INT:    return db_elem(int64_t(_d.i64 op rhs._d.i64)); \
      case TYPE_DOUBLE: return db_elem(double(_d.dbl op rhs._d.dbl)); \
      default:          assert(false); \
    } \
  } else { \
    if (_t > rhs._t) { \
      return (*this) op rhs.promote(_t); \
    } else { \
      return promote(rhs._t) op rhs; \
    } \
  } \
  throw std::runtime_error("unreachable"); \

#define NUMERIC_BINOP_BODY_BOOL(op) \
  if (_t == rhs._t) { \
    switch (_t) { \
      case TYPE_BOOL:   return db_elem((_d.b op rhs._d.b)); \
      case TYPE_INT:    return db_elem((_d.i64 op rhs._d.i64)); \
      case TYPE_DOUBLE: return db_elem((_d.dbl op rhs._d.dbl)); \
      default:          assert(false); \
    } \
  } else { \
    if (_t > rhs._t) { \
      return (*this) op rhs.promote(_t); \
    } else { \
      return promote(rhs._t) op rhs; \
    } \
  } \
  throw std::runtime_error("unreachable"); \

#define DECL_IMPL_NUMERIC_BINOP(op) \
  inline db_elem operator op(const db_elem& rhs) const { \
    VECTOR_OPS_IMPL(op); \
    checkNumeric(); \
    rhs.checkNumeric(); \
    NUMERIC_BINOP_BODY(op); \
  }

  DECL_IMPL_NUMERIC_BINOP(+);
  DECL_IMPL_NUMERIC_BINOP(-);
  DECL_IMPL_NUMERIC_BINOP(*);
  DECL_IMPL_NUMERIC_BINOP(/);

#define DECL_IMPL_BINOP(op) \
  inline db_elem operator op(const db_elem& rhs) const { \
    VECTOR_OPS_IMPL(op); \
    if (IsNumericType(_t) || IsNumericType(rhs._t)) { \
      NUMERIC_BINOP_BODY_BOOL(op); \
    } \
    if (_t != rhs._t) { \
      throw std::runtime_error("in-compatible types"); \
    } \
    switch (_t) { \
      case TYPE_STRING: return db_elem(_s op rhs._s); \
      case TYPE_DATE:   return db_elem(_d.i64 op rhs._d.i64); \
      default: break; \
    } \
    throw std::runtime_error("unimpl"); \
  }

#define DECL_IMPL_LOGICAL_BINOP(op) \
  inline db_elem operator op(const db_elem& rhs) const { \
    VECTOR_OPS_IMPL(op); \
    if (_t != TYPE_BOOL || rhs._t != TYPE_BOOL) { \
      throw std::runtime_error("need bool types"); \
    } \
    return db_elem(_d.b op rhs._d.b); \
  }

  DECL_IMPL_BINOP(<);
  DECL_IMPL_BINOP(<=);
  DECL_IMPL_BINOP(>);
  DECL_IMPL_BINOP(>=);
  DECL_IMPL_BINOP(==);
  DECL_IMPL_BINOP(!=);

  DECL_IMPL_LOGICAL_BINOP(||);
  DECL_IMPL_LOGICAL_BINOP(&&);

  // unsafe casts assume not vectors

  inline bool unsafe_cast_bool() const {
    requireType(TYPE_BOOL);
    return _d.b;
  }

  inline explicit operator bool() const { return unsafe_cast_bool(); }

  inline bool is_vector() const { return _t == TYPE_VECTOR; }

  inline size_t size() const {
    requireType(TYPE_VECTOR);
    return _elems.size();
  }

  inline db_elem& operator[](size_t i) {
    requireType(TYPE_VECTOR);
    return _elems[i];
  }

  inline const db_elem& operator[](size_t i) const {
    requireType(TYPE_VECTOR);
    return _elems[i];
  }

  typedef std::vector< db_elem >::iterator       iterator;
  typedef std::vector< db_elem >::const_iterator const_iterator;

  // unchecked
  inline iterator begin() { return _elems.begin(); }
  inline const_iterator begin() const { return _elems.begin(); }
  inline iterator end() { return _elems.end(); }
  inline const_iterator end() const { return _elems.end(); }

  // postgres helpers
  static type TypeFromPGOid(unsigned int oid);

#undef VECTOR_OPS_IMPL
#undef DECL_IMPL_NUMERIC_BINOP
#undef DECL_IMPL_BINOP

  db_elem like(const db_elem& qstr, bool case_sensitive = true) const;

  // assumes vector
  db_elem count(bool distinct) const;
  db_elem sum(bool distinct) const;

  db_elem filter(const db_elem& mask) const;

  bool empty_mask() const;

private:
  type _t;
  union {
    int64_t i64;
    bool b;
    double dbl;
  } _d;
  std::string _s;
  std::vector< db_elem > _elems;
};

std::ostream& operator<<(std::ostream& o, const db_elem& e);

/** represents a tuple from the database.
 * is really a thin wrapper around a vector of db_elems
 */
class db_tuple {
public:
  std::vector< db_elem > columns;
};

std::ostream& operator<<(std::ostream& o, const db_tuple& tuple);

class db_column_desc {
public:

  db_column_desc() {}

  db_column_desc(
    db_elem::type type,
    size_t size,
    onion onion_type,
    SECLEVEL level,
    size_t pos,
    bool is_vector)

    : type(type), size(size), onion_type(onion_type), level(level),
      pos(pos), is_vector(is_vector) {}

  // creates a descriptor after decryption
  db_column_desc decrypt_desc() const {
    return db_column_desc(type, size, oNONE, SECLEVEL::PLAIN, pos, is_vector);
  }

  // TODO: these really should all be const

  db_elem::type type; // the original type
  size_t size; // the original type size (in bytes)

  onion onion_type;
  SECLEVEL level;

  size_t pos; // the column position (used for key generation)

  bool is_vector;
};
