#pragma once

#include <map>
#include <string>
#include <vector>
#include <utility>

#include <execution/operator.hh>
#include <execution/eval.hh>

#include <util/either.hh>
#include <util/stl.hh>

#define DECL_PHY_OP_NEXT \
  virtual void next(exec_context& ctx, db_tuple_vec& tuple);

#define DECL_PHY_OP_ITER_IFACE \
  virtual bool has_more(exec_context& ctx); \
  DECL_PHY_OP_NEXT

class sql_param_generator {
public:
  typedef std::map< size_t, db_elem > param_map;
  virtual ~sql_param_generator() {}
  virtual param_map get_param_map(exec_context& ctx) = 0;
};

class remote_sql_op : public physical_operator {
public:
  remote_sql_op(
      sql_param_generator* param_generator,
      const std::string& sql,
      const desc_vec& desc_vec,
      const std::vector< physical_operator* >& children) :
    physical_operator(children),
    _param_generator(param_generator), _sql(sql), _desc_vec(desc_vec), _res(NULL) {}

  ~remote_sql_op() {
    if (_param_generator) delete _param_generator;
  }

  virtual desc_vec tuple_desc() { return _desc_vec; }

  virtual void open(exec_context& ctx);
  virtual void close(exec_context& ctx);

  DECL_PHY_OP_ITER_IFACE;

protected:
  sql_param_generator* _param_generator;

  std::string _sql;
  desc_vec _desc_vec;

  DBResultNew* _res;

private:
  static db_elem CreateFromString(
      const std::string& data,
      db_elem::type type);
};

class local_decrypt_op : public physical_operator {
public:
  local_decrypt_op(
      const pos_vec& pos,
      physical_operator* child)
    : physical_operator({child}), _pos(pos) {
    assert(!pos.empty());
    assert(child);
  }

  virtual desc_vec tuple_desc();

  DECL_PHY_OP_NEXT;

protected:
  pos_vec _pos;
};

class local_filter_op : public physical_operator {
public:
  local_filter_op(
      expr_node* filter,
      physical_operator* child,
      const op_vec& subqueries)
    : physical_operator(util::prepend(child, subqueries)), _filter(filter) {
    assert(filter);
    assert(child);
  }

  ~local_filter_op() { delete _filter; }

  DECL_PHY_OP_NEXT;

protected:
  expr_node* _filter; // ownership
};

class local_transform_op : public physical_operator {
public:
  typedef either< size_t, std::pair<db_column_desc, expr_node*> > trfm_desc;

  typedef std::vector<trfm_desc> trfm_desc_vec;

  local_transform_op(
      const trfm_desc_vec& trfm_vec,
      physical_operator* child)
    : physical_operator({child}), _trfm_vec(trfm_vec) {}

  ~local_transform_op() {
    for (auto &e : _trfm_vec) {
      if (e.isRight()) delete e.right().second;
    }
  }

  virtual desc_vec tuple_desc();

  DECL_PHY_OP_NEXT;

protected:
  trfm_desc_vec _trfm_vec;
};

class local_order_by : public physical_operator {
public:
  typedef std::vector< std::pair< size_t, bool > > sort_keys_vec;

  local_order_by(
      const sort_keys_vec& sort_keys,
      physical_operator* child)
    : physical_operator({child}), _sort_keys(sort_keys) {}

  DECL_PHY_OP_NEXT;

protected:
  sort_keys_vec _sort_keys;
};

class local_limit : public physical_operator {
public:
  local_limit(
      size_t n,
      physical_operator* child)
    : physical_operator({child}), _i(0), _n(n) {
    assert(child);
  }

  DECL_PHY_OP_ITER_IFACE;

protected:
  size_t _i;
  size_t _n;
};
