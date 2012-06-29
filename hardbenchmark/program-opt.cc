// WARNING: This is an auto-generated file
#include "db_config_opt.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <crypto-old/CryptoManager.hh>
#include <crypto/paillier.hh>
#include <edb/ConnectNew.hh>
#include <execution/encryption.hh>
#include <execution/context.hh>
#include <execution/operator_types.hh>
#include <execution/eval_nodes.hh>
#include <execution/query_cache.hh>
#include <execution/commandline.hh>
#include <util/util.hh>
class param_generator_id0 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  {
    Binary key(ctx.crypto->cm->getKey(ctx.crypto->cm->getmkey(), fieldname(1, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary("khaki"));
    m[0] = db_elem((const char *)t.ciph.content, t.ciph.len);
    m[1] = db_elem((const char *)t.wordKey.content, t.wordKey.len);
  }
  return m;
}
};
class param_generator_id1 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id2 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = ctx.args->columns.at(0);
  m[1] = ctx.args->columns.at(1);
  m[2] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1022497 /*1997-1-1*/, 10, true));
  m[3] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1023009 /*1998-1-1*/, 10, true));
  return m;
}
};
class param_generator_id3 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "ALGERIA", 1, false));
  return m;
}
};
class param_generator_id4 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_u8_det(ctx.crypto, 70 /*F*/, 2, false));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "IRAN", 1, false));
  return m;
}
};
static void query_0(exec_context& ctx) {
  physical_operator* op = new local_decrypt_op(
    {0, 1},
    new remote_sql_op(new param_generator_id3, "select " SUPPLIER_ENC_NAME ".s_name_DET, " SUPPLIER_ENC_NAME ".s_address_DET from " SUPPLIER_ENC_NAME ", " NATION_ENC_NAME " where ((" SUPPLIER_ENC_NAME ".s_suppkey_DET in ( :_subselect$1 )) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET))) and ((" NATION_ENC_NAME ".n_name_DET) = (:0)) order by " SUPPLIER_ENC_NAME ".s_name_OPE ASC", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 40, oDET, SECLEVEL::DET, 2, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({std::pair<std::string, physical_operator*>("_subselect$1", new local_transform_op(
      {local_transform_op::trfm_desc(2), },
      new local_filter_op(
        new gt_node(new tuple_pos_node(0), new subselect_node(0, {new tuple_pos_node(1), new tuple_pos_node(2)})),
        new local_decrypt_op(
          {0},
          new remote_sql_op(new param_generator_id1, "select " PARTSUPP_ENC_NAME ".ps_availqty_DET, " PARTSUPP_ENC_NAME ".ps_partkey_DET, " PARTSUPP_ENC_NAME ".ps_suppkey_DET from " PARTSUPP_ENC_NAME " where " PARTSUPP_ENC_NAME ".ps_partkey_DET in ( :_subselect$0 )", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 1, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({std::pair<std::string, physical_operator*>("_subselect$0", new remote_sql_op(new param_generator_id0, "select " PART_ENC_NAME ".p_partkey_DET from " PART_ENC_NAME " where searchSWP(:0, :1, " PART_ENC_NAME ".p_name_SWP)", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))})))
        ,{
          new local_transform_op(
            {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new double_literal_node(0.500000), new sum_node(new tuple_pos_node(0), false)))), },
            new local_decrypt_op(
              {0},
              new remote_sql_op(new param_generator_id2, "select group_serializer(" LINEITEM_ENC_NAME ".l_quantity_DET) from " LINEITEM_ENC_NAME " where ((((" LINEITEM_ENC_NAME ".l_partkey_DET) = (:0)) and ((" LINEITEM_ENC_NAME ".l_suppkey_DET) = (:1))) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) >= (:2))) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) < (:3))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          )
          ,
        }
      )
    )
    )})))
  ;
  op->open(ctx);
  while (op->has_more(ctx)) {
    physical_operator::db_tuple_vec v;
    op->next(ctx, v);
    physical_operator::print_tuples(v);
  }
  op->close(ctx);
  delete op;
}
static void query_1(exec_context& ctx) {
  physical_operator* op = new local_limit(100,
    new local_order_by(
      {std::make_pair(1, true), std::make_pair(0, false)},
      new local_decrypt_op(
        {0},
        new remote_sql_op(new param_generator_id4, "select " SUPPLIER_ENC_NAME ".s_name_DET, count(*) as numwait from " SUPPLIER_ENC_NAME ", " LINEITEM_ENC_NAME " l1, " ORDERS_ENC_NAME ", " NATION_ENC_NAME " where ((((((((" SUPPLIER_ENC_NAME ".s_suppkey_DET) = (l1.l_suppkey_DET)) and ((" ORDERS_ENC_NAME ".o_orderkey_DET) = (l1.l_orderkey_DET))) and ((" ORDERS_ENC_NAME ".o_orderstatus_DET) = (:0))) and ((l1.l_receiptdate_OPE) > (l1.l_commitdate_OPE))) and (exists ( (select 1 from " LINEITEM_ENC_NAME " l2 where ((l2.l_orderkey_DET) = (l1.l_orderkey_DET)) and ((l2.l_suppkey_DET) <> (l1.l_suppkey_DET))) ))) and (not ( exists ( (select 1 from " LINEITEM_ENC_NAME " l3 where (((l3.l_orderkey_DET) = (l1.l_orderkey_DET)) and ((l3.l_suppkey_DET) <> (l1.l_suppkey_DET))) and ((l3.l_receiptdate_OPE) > (l3.l_commitdate_OPE))) ) ))) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET))) and ((" NATION_ENC_NAME ".n_name_DET) = (:1)) group by " SUPPLIER_ENC_NAME ".s_name_DET", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
    )
  )
  ;
  op->open(ctx);
  while (op->has_more(ctx)) {
    physical_operator::db_tuple_vec v;
    op->next(ctx, v);
    physical_operator::print_tuples(v);
  }
  op->close(ctx);
  delete op;
}
int main(int argc, char **argv) {
  command_line_opts opts(
    DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PORT);
  command_line_opts::parse_options(argc, argv, opts);
  std::cerr << opts << std::endl;
  if ((optind + 1) != argc) {
    std::cerr << "[Usage]: " << argv[0] << " [options] [query num]" << std::endl;
    return 1;
  }
  int q = atoi(argv[optind]);
  CryptoManager cm("12345");
  crypto_manager_stub cm_stub(&cm, CRYPTO_USE_OLD_OPE);
  PGConnect pg(
   opts.db_hostname, opts.db_username, opts.db_passwd,
   opts.db_database, opts.db_port, true);
  paillier_cache pp_cache;
  dict_compress_tables dict_tables;
  query_cache cache;
  exec_context ctx(&pg, &cm_stub, &pp_cache, &dict_tables, &cache);
  switch (q) {
    case 0: query_0(ctx); break;
    case 1: query_1(ctx); break;
    default: assert(false);
  }
  return 0;
}
