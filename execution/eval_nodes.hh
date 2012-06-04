#pragma once

#include <cstdint>
#include <string>

#include <execution/eval.hh>
#include <util/stl.hh>

template <typename T>
class literal_node : public expr_node {
public:
  literal_node(const T& t) : _t(t) {}
  virtual db_elem eval(eval_context& ctx) { return db_elem(_t); }
protected:
  T _t;
};

#define DECL_LITERAL_NODE(name, type) \
  class name : public literal_node<type> { \
  public: \
    name(const type& t) : literal_node<type>(t) {} \
    virtual db_elem eval(eval_context& ctx) { return db_elem(_t); } \
  };

DECL_LITERAL_NODE(int_literal_node, int64_t);
DECL_LITERAL_NODE(bool_literal_node, bool);
DECL_LITERAL_NODE(string_literal_node, std::string);
DECL_LITERAL_NODE(double_literal_node, double);

class null_literal_node : public expr_node {
public:
  virtual db_elem eval(eval_context& ctx) { return db_elem(db_elem::null_tag); }
};

class date_literal_node : public expr_node {
public:
  date_literal_node(
      unsigned int year,
      unsigned int month,
      unsigned int day)
    : _year(year), _month(month), _day(day) {}

  date_literal_node(
      const std::string& fmt) {
    int ret = sscanf(fmt.c_str(), "%d-%d-%d", &_year, &_month, &_day);
    assert(ret == 3);
    assert(1 <= _day && _day <= 31);
    assert(1 <= _month && _month <= 12);
    // TODO: validate for actual date
  }

  virtual db_elem eval(eval_context& ctx) {
    return db_elem(_year, _month, _day);
  }

private:
  unsigned int _year;
  unsigned int _month;
  unsigned int _day;
};

class unop_node : public expr_node {
public:
  unop_node(expr_node* child) : expr_node({child}) {}
};

class not_node : public unop_node {
public:
  not_node(expr_node* child) : unop_node(child) {}
  virtual db_elem eval(eval_context& ctx) { return !first_child()->eval(ctx); }
};

class binop_node : public expr_node {
public:
  binop_node(expr_node* left, expr_node* right)
    : expr_node({left, right}) {}
};

#define DECL_BINOP_NODE(name, op) \
  class name : public binop_node { \
  public: \
    name(expr_node* left, expr_node* right) \
      : binop_node(left, right) {} \
    virtual db_elem eval(eval_context& ctx) { \
      return first_child()->eval(ctx) op second_child()->eval(ctx); \
    } \
  };

DECL_BINOP_NODE(add_node, +);
DECL_BINOP_NODE(sub_node, -);
DECL_BINOP_NODE(mult_node, *);
DECL_BINOP_NODE(div_node, /);
DECL_BINOP_NODE(lt_node, <);
DECL_BINOP_NODE(le_node, <=);
DECL_BINOP_NODE(gt_node, >);
DECL_BINOP_NODE(ge_node, >=);
DECL_BINOP_NODE(eq_node, ==);
DECL_BINOP_NODE(neq_node, !=);
DECL_BINOP_NODE(or_node, ||);
DECL_BINOP_NODE(and_node, &&);

class in_node : public expr_node {
public:
  in_node(expr_node* needle,
          const child_vec& haystack)
    : expr_node(util::prepend(needle, haystack)) {}
  virtual db_elem eval(eval_context& ctx);
};

class like_node : public binop_node {
public:
  like_node(expr_node* left, expr_node* right, bool negate)
    : binop_node(left, right), _negate(negate) {}
  virtual db_elem eval(eval_context& ctx) {
    db_elem res = first_child()->eval(ctx).like(second_child()->eval(ctx));
    return _negate ? !res : res;
  }
private:
  bool _negate;
};

class exists_node : public expr_node {
public:
  exists_node(expr_node* child)
    : expr_node({child}) {}
  virtual db_elem eval(eval_context& ctx) {
    return first_child()->eval(ctx).is_inited();
  }
};

class sum_node : public expr_node {
public:
  sum_node(expr_node* child, bool distinct)
    : expr_node({child}), _distinct(distinct) {}
  virtual db_elem eval(eval_context& ctx) {
    return first_child()->eval(ctx).sum(_distinct);
  }
private:
  bool _distinct;
};

class case_expr_case_node : public expr_node {
public:
  typedef std::vector< case_expr_case_node* > node_vec;

  case_expr_case_node(
      expr_node* cond,
      expr_node* expr)
    : expr_node({cond, expr}) {}

  inline expr_node* condition() { return first_child(); }

  virtual db_elem eval(eval_context& ctx);
};

class case_when_node : public expr_node {
public:
  case_when_node(
      const case_expr_case_node::node_vec& cases,
      expr_node* dft)
    : expr_node(util::change_ptr_vec_type<expr_node>(cases)), _dft(dft) {}

  ~case_when_node() { if (_dft) delete _dft; }

  virtual db_elem eval(eval_context& ctx);

private:
  expr_node* _dft;
};

class tuple_pos_node : public expr_node {
public:
  tuple_pos_node(size_t pos) : _pos(pos) {}
  virtual db_elem eval(eval_context& ctx) {
    db_elem& c = ctx.tuple->columns[_pos];
    if (ctx.idx != -1 && c.is_vector()) {
      assert(ctx.idx >= 0);
      return c[ctx.idx];
    } else {
      return c;
    }
  }
private:
  size_t _pos;
};

class subselect_node : public expr_node {
public:
  subselect_node(size_t n, const child_vec& args)
    : expr_node(args), _n(n) {}
  virtual db_elem eval(eval_context& ctx);
private:
  size_t _n;
};
