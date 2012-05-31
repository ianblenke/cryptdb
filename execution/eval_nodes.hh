#pragma once

#include <cstdint>
#include <string>

#include <execution/eval.hh>
#include <util/stl.hh>

template <typename T>.
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

class unop_node : public expr_node {
public:
  unop_node(expr_node* child) : expr_node({child}) {}
};

class not_node : public unop_node {
public:
  not_node(expr_node* child) : unop_node(child) {}
  virtual db_elem eval(eval_context& ctx) { return !first_child()->eval(ctx); }
}

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
    return negate ? !res : res;
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

class tuple_pos_node : public expr_node {
public:
  tuple_pos_node(size_t pos) : _pos(pos) {}
  virtual db_elem eval(eval_context& ctx) { return ctx.tuple->columns[_pos]; }
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
