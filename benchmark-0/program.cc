// WARNING: This is an auto-generated file
#include "db_config.h"
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
static inline size_t _FP(size_t i) {
  #ifdef ALL_SAME_KEY
  return 0;
  #else
  return i;
  #endif /* ALL_SAME_KEY */
}
static inline bool _FJ(bool join) {
  #ifdef ALL_SAME_KEY
  return false;
  #else
  return join;
  #endif /* ALL_SAME_KEY */
}
class param_generator_id0 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id1 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_u32_det(ctx.crypto, 36, _FP(5), _FJ(false)));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "ASIA", _FP(1), _FJ(false)));
  return m;
}
};
class param_generator_id2 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = ctx.args->columns.at(0);
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "ASIA", _FP(1), _FJ(false)));
  return m;
}
};
class param_generator_id3 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "FURNITURE", _FP(6), _FJ(false)));
  return m;
}
};
class param_generator_id4 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id5 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = ctx.args->columns.at(0);
  return m;
}
};
class param_generator_id6 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "MIDDLE EAST", _FP(1), _FJ(false)));
  return m;
}
};
class param_generator_id7 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 2500 /*25*/, _FP(4), _FJ(false)), 16)));
  return m;
}
};
class param_generator_id8 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "SAUDI ARABIA", _FP(1), _FJ(false)));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "ARGENTINA", _FP(1), _FJ(false)));
  m[2] = db_elem(encrypt_string_det(ctx.crypto, "ARGENTINA", _FP(1), _FJ(false)));
  m[3] = db_elem(encrypt_string_det(ctx.crypto, "SAUDI ARABIA", _FP(1), _FJ(false)));
  return m;
}
};
class param_generator_id9 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "AMERICA", _FP(1), _FJ(false)));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "PROMO ANODIZED BRASS", _FP(4), _FJ(false)));
  return m;
}
};
class param_generator_id10 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id11 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_u8_det(ctx.crypto, 82 /*R*/, _FP(8), _FJ(false)));
  return m;
}
};
class param_generator_id12 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "ARGENTINA", _FP(1), _FJ(false)));
  return m;
}
};
class param_generator_id13 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "ARGENTINA", _FP(1), _FJ(false)));
  return m;
}
};
class param_generator_id14 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "AIR", _FP(14), _FJ(false)));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "FOB", _FP(14), _FJ(false)));
  return m;
}
};
class param_generator_id15 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id16 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "Brand#45", _FP(3), _FJ(false)));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "LG BOX", _FP(6), _FJ(false)));
  return m;
}
};
class param_generator_id17 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id18 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id19 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "Brand#23", _FP(3), _FJ(false)));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "SM CASE", _FP(6), _FJ(false)));
  m[2] = db_elem(encrypt_string_det(ctx.crypto, "SM BOX", _FP(6), _FJ(false)));
  m[3] = db_elem(encrypt_string_det(ctx.crypto, "SM PACK", _FP(6), _FJ(false)));
  m[4] = db_elem(encrypt_string_det(ctx.crypto, "SM PKG", _FP(6), _FJ(false)));
  m[5] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 1000 /*10*/, _FP(4), _FJ(false)), 16)));
  m[6] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 2000 /*20*/, _FP(4), _FJ(false)), 16)));
  m[7] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 1, _FP(5), _FJ(false)));
  m[8] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 5, _FP(5), _FJ(false)));
  m[9] = db_elem(encrypt_string_det(ctx.crypto, "AIR", _FP(14), _FJ(false)));
  m[10] = db_elem(encrypt_string_det(ctx.crypto, "AIR REG", _FP(14), _FJ(false)));
  m[11] = db_elem(encrypt_string_det(ctx.crypto, "DELIVER IN PERSON", _FP(13), _FJ(false)));
  m[12] = db_elem(encrypt_string_det(ctx.crypto, "Brand#32", _FP(3), _FJ(false)));
  m[13] = db_elem(encrypt_string_det(ctx.crypto, "MED BAG", _FP(6), _FJ(false)));
  m[14] = db_elem(encrypt_string_det(ctx.crypto, "MED BOX", _FP(6), _FJ(false)));
  m[15] = db_elem(encrypt_string_det(ctx.crypto, "MED PKG", _FP(6), _FJ(false)));
  m[16] = db_elem(encrypt_string_det(ctx.crypto, "MED PACK", _FP(6), _FJ(false)));
  m[17] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 2000 /*20*/, _FP(4), _FJ(false)), 16)));
  m[18] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 3000 /*30*/, _FP(4), _FJ(false)), 16)));
  m[19] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 1, _FP(5), _FJ(false)));
  m[20] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 10, _FP(5), _FJ(false)));
  m[21] = db_elem(encrypt_string_det(ctx.crypto, "AIR", _FP(14), _FJ(false)));
  m[22] = db_elem(encrypt_string_det(ctx.crypto, "AIR REG", _FP(14), _FJ(false)));
  m[23] = db_elem(encrypt_string_det(ctx.crypto, "DELIVER IN PERSON", _FP(13), _FJ(false)));
  m[24] = db_elem(encrypt_string_det(ctx.crypto, "Brand#34", _FP(3), _FJ(false)));
  m[25] = db_elem(encrypt_string_det(ctx.crypto, "LG CASE", _FP(6), _FJ(false)));
  m[26] = db_elem(encrypt_string_det(ctx.crypto, "LG BOX", _FP(6), _FJ(false)));
  m[27] = db_elem(encrypt_string_det(ctx.crypto, "LG PACK", _FP(6), _FJ(false)));
  m[28] = db_elem(encrypt_string_det(ctx.crypto, "LG PKG", _FP(6), _FJ(false)));
  m[29] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 2000 /*20*/, _FP(4), _FJ(false)), 16)));
  m[30] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 3000 /*30*/, _FP(4), _FJ(false)), 16)));
  m[31] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 1, _FP(5), _FJ(false)));
  m[32] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 15, _FP(5), _FJ(false)));
  m[33] = db_elem(encrypt_string_det(ctx.crypto, "AIR", _FP(14), _FJ(false)));
  m[34] = db_elem(encrypt_string_det(ctx.crypto, "AIR REG", _FP(14), _FJ(false)));
  m[35] = db_elem(encrypt_string_det(ctx.crypto, "DELIVER IN PERSON", _FP(13), _FJ(false)));
  return m;
}
};
class param_generator_id20 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id21 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id22 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "ALGERIA", _FP(1), _FJ(false)));
  return m;
}
};
class param_generator_id23 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_u8_det(ctx.crypto, 70 /*F*/, _FP(2), _FJ(false)));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "IRAN", _FP(1), _FJ(false)));
  return m;
}
};
class param_generator_id24 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = ctx.args->columns.at(0);
  m[1] = ctx.args->columns.at(1);
  return m;
}
};
class param_generator_id25 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
class param_generator_id26 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
static void query_0(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(0, false), std::make_pair(1, false)},
    new local_transform_op(
      {local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(3), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(4), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(4), new sub_node(new int_literal_node(1), new tuple_pos_node(5))), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new mult_node(new tuple_pos_node(4), new sub_node(new int_literal_node(1), new tuple_pos_node(5))), new add_node(new int_literal_node(1), new tuple_pos_node(6))), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new avg_node(new tuple_pos_node(3), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new avg_node(new tuple_pos_node(4), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new avg_node(new tuple_pos_node(5), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false), new count_star_node)), },
      new local_decrypt_op(
        {1, 2, 3, 4, 5, 6},
        new local_filter_op(
          new le_node(new tuple_pos_node(0), new date_literal_node("1998-1-1")),
          new local_decrypt_op(
            {0},
            new remote_sql_op(new param_generator_id0, "select group_serializer(lineitem_enc_cryptdb_opt_with_det.l_shipdate_DET), lineitem_enc_cryptdb_opt_with_det.l_returnflag_DET, lineitem_enc_cryptdb_opt_with_det.l_linestatus_DET, group_serializer(lineitem_enc_cryptdb_opt_with_det.l_quantity_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_extendedprice_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_discount_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_tax_DET) from lineitem_enc_cryptdb_opt_with_det group by lineitem_enc_cryptdb_opt_with_det.l_returnflag_DET, lineitem_enc_cryptdb_opt_with_det.l_linestatus_DET", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, true), db_column_desc(db_elem::TYPE_CHAR, 1, oDET, SECLEVEL::DET, 8, false), db_column_desc(db_elem::TYPE_CHAR, 1, oDET, SECLEVEL::DET, 9, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 7, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          ,{
          }
        )
      )
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
static void query_1(exec_context& ctx) {
  physical_operator* op = new local_limit(100, 
    new local_order_by(
      {std::make_pair(0, true), std::make_pair(2, false), std::make_pair(1, false), std::make_pair(3, false)},
      new local_transform_op(
        {local_transform_op::trfm_desc(3), local_transform_op::trfm_desc(4), local_transform_op::trfm_desc(5), local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(6), local_transform_op::trfm_desc(7), local_transform_op::trfm_desc(8), local_transform_op::trfm_desc(9), },
        new local_decrypt_op(
          {2, 3, 4, 5, 6, 7, 8, 9},
          new local_filter_op(
            new and_node(new like_node(new tuple_pos_node(0), new string_literal_node("%STEEL"), false), new eq_node(new tuple_pos_node(1), new subselect_node(0, {new tuple_pos_node(2)}))),
            new local_decrypt_op(
              {0, 1},
              new remote_sql_op(new param_generator_id1, "select part_enc_cryptdb_opt_with_det.p_type_DET, partsupp_enc_cryptdb_opt_with_det.ps_supplycost_DET, part_enc_cryptdb_opt_with_det.p_partkey_DET, supplier_enc_cryptdb_opt_with_det.s_acctbal_DET, supplier_enc_cryptdb_opt_with_det.s_name_DET, nation_enc_cryptdb_opt_with_det.n_name_DET, part_enc_cryptdb_opt_with_det.p_mfgr_DET, supplier_enc_cryptdb_opt_with_det.s_address_DET, supplier_enc_cryptdb_opt_with_det.s_phone_DET, supplier_enc_cryptdb_opt_with_det.s_comment_DET from part_enc_cryptdb_opt_with_det, supplier_enc_cryptdb_opt_with_det, partsupp_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det, region_enc_cryptdb_opt_with_det where ((((((part_enc_cryptdb_opt_with_det.p_partkey_DET) = (partsupp_enc_cryptdb_opt_with_det.ps_partkey_DET)) and ((supplier_enc_cryptdb_opt_with_det.s_suppkey_DET) = (partsupp_enc_cryptdb_opt_with_det.ps_suppkey_DET))) and ((part_enc_cryptdb_opt_with_det.p_size_DET) = (:0))) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET))) and ((nation_enc_cryptdb_opt_with_det.n_regionkey_DET) = (region_enc_cryptdb_opt_with_det.r_regionkey_DET))) and ((region_enc_cryptdb_opt_with_det.r_name_DET) = (:1))", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 40, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_STRING, 101, oDET, SECLEVEL::DET, 6, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
            ,{
              new local_transform_op(
                {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 3, false), new min_node(new tuple_pos_node(0)))), },
                new local_decrypt_op(
                  {0},
                  new remote_sql_op(new param_generator_id2, "select group_serializer(partsupp_enc_cryptdb_opt_with_det.ps_supplycost_DET) from partsupp_enc_cryptdb_opt_with_det, supplier_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det, region_enc_cryptdb_opt_with_det where (((((:0) = (partsupp_enc_cryptdb_opt_with_det.ps_partkey_DET)) and ((supplier_enc_cryptdb_opt_with_det.s_suppkey_DET) = (partsupp_enc_cryptdb_opt_with_det.ps_suppkey_DET))) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET))) and ((nation_enc_cryptdb_opt_with_det.n_regionkey_DET) = (region_enc_cryptdb_opt_with_det.r_regionkey_DET))) and ((region_enc_cryptdb_opt_with_det.r_name_DET) = (:1))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
              )
              ,
            }
          )
        )
      )
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
static void query_2(exec_context& ctx) {
  physical_operator* op = new local_limit(10, 
    new local_order_by(
      {std::make_pair(1, true), std::make_pair(2, false)},
      new local_transform_op(
        {local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(3), false))), local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(4), },
        new local_decrypt_op(
          {2, 3, 4},
          new local_filter_op(
            new and_node(new lt_node(new tuple_pos_node(0), new date_literal_node("1995-3-12")), new gt_node(new tuple_pos_node(1), new date_literal_node("1995-3-12"))),
            new local_decrypt_op(
              {0, 1},
              new remote_sql_op(new param_generator_id3, "select orders_enc_cryptdb_opt_with_det.o_orderdate_DET, group_serializer(lineitem_enc_cryptdb_opt_with_det.l_shipdate_DET), lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET, group_serializer(lineitem_enc_cryptdb_opt_with_det.l_disc_price_DET), orders_enc_cryptdb_opt_with_det.o_shippriority_DET from customer_enc_cryptdb_opt_with_det, orders_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det where (((customer_enc_cryptdb_opt_with_det.c_mktsegment_DET) = (:0)) and ((customer_enc_cryptdb_opt_with_det.c_custkey_DET) = (orders_enc_cryptdb_opt_with_det.o_custkey_DET))) and ((lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET) = (orders_enc_cryptdb_opt_with_det.o_orderkey_DET)) group by lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET, orders_enc_cryptdb_opt_with_det.o_orderdate_DET, orders_enc_cryptdb_opt_with_det.o_shippriority_DET", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 0, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 7, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
            ,{
            }
          )
        )
      )
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
static void query_3(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(0, false)},
    new local_transform_op(
      {local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false), new count_star_node)), },
      new local_decrypt_op(
        {2},
        new local_filter_op(
          new and_node(new and_node(new ge_node(new tuple_pos_node(0), new date_literal_node("1994-11-1")), new lt_node(new tuple_pos_node(0), new date_literal_node("1995-2-1"))), new exists_node(new subselect_node(0, {new tuple_pos_node(1)}))),
          new local_decrypt_op(
            {0},
            new remote_sql_op(new param_generator_id4, "select group_serializer(orders_enc_cryptdb_opt_with_det.o_orderdate_DET), group_serializer(orders_enc_cryptdb_opt_with_det.o_orderkey_DET), orders_enc_cryptdb_opt_with_det.o_orderpriority_DET from orders_enc_cryptdb_opt_with_det group by orders_enc_cryptdb_opt_with_det.o_orderpriority_DET", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, true), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 5, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          ,{
            new local_filter_op(
              new lt_node(new tuple_pos_node(0), new tuple_pos_node(1)),
              new local_decrypt_op(
                {0, 1},
                new remote_sql_op(new param_generator_id5, "select lineitem_enc_cryptdb_opt_with_det.l_commitdate_DET, lineitem_enc_cryptdb_opt_with_det.l_receiptdate_DET from lineitem_enc_cryptdb_opt_with_det where (lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET) = (:0)", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 11, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 12, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
              ,{
              }
            )
            ,
          }
        )
      )
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
static void query_4(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(1, true)},
    new local_transform_op(
      {local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(2), false))), },
      new local_decrypt_op(
        {1, 2},
        new local_filter_op(
          new and_node(new ge_node(new tuple_pos_node(0), new date_literal_node("1993-1-1")), new lt_node(new tuple_pos_node(0), new date_literal_node("1994-1-1"))),
          new local_decrypt_op(
            {0},
            new remote_sql_op(new param_generator_id6, "select group_serializer(orders_enc_cryptdb_opt_with_det.o_orderdate_DET), nation_enc_cryptdb_opt_with_det.n_name_DET, group_serializer(lineitem_enc_cryptdb_opt_with_det.l_disc_price_DET) from customer_enc_cryptdb_opt_with_det, orders_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det, supplier_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det, region_enc_cryptdb_opt_with_det where (((((((customer_enc_cryptdb_opt_with_det.c_custkey_DET) = (orders_enc_cryptdb_opt_with_det.o_custkey_DET)) and ((lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET) = (orders_enc_cryptdb_opt_with_det.o_orderkey_DET))) and ((lineitem_enc_cryptdb_opt_with_det.l_suppkey_DET) = (supplier_enc_cryptdb_opt_with_det.s_suppkey_DET))) and ((customer_enc_cryptdb_opt_with_det.c_nationkey_DET) = (supplier_enc_cryptdb_opt_with_det.s_nationkey_DET))) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET))) and ((nation_enc_cryptdb_opt_with_det.n_regionkey_DET) = (region_enc_cryptdb_opt_with_det.r_regionkey_DET))) and ((region_enc_cryptdb_opt_with_det.r_name_DET) = (:0)) group by nation_enc_cryptdb_opt_with_det.n_name_DET", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 0, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          ,{
          }
        )
      )
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
static void query_5(exec_context& ctx) {
  physical_operator* op = new local_transform_op(
    {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(2), new tuple_pos_node(1)), false))), },
    new local_decrypt_op(
      {2},
      new local_filter_op(
        new and_node(new and_node(new and_node(new ge_node(new tuple_pos_node(0), new date_literal_node("1993-1-1")), new lt_node(new tuple_pos_node(0), new date_literal_node("1994-1-1"))), new ge_node(new tuple_pos_node(1), new double_literal_node(0.040000))), new le_node(new tuple_pos_node(1), new double_literal_node(0.060000))),
        new local_decrypt_op(
          {0, 1},
          new remote_sql_op(new param_generator_id7, "select group_serializer(lineitem_enc_cryptdb_opt_with_det.l_shipdate_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_discount_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_extendedprice_DET) from lineitem_enc_cryptdb_opt_with_det where (lineitem_enc_cryptdb_opt_with_det.l_quantity_OPE) < (:0)", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
        ,{
        }
      )
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
static void query_6(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(0, false), std::make_pair(1, false), std::make_pair(2, false)},
    new local_transform_op(
      {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_STRING, 25, oNONE, SECLEVEL::PLAIN, 1, false), new tuple_pos_node(0))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_STRING, 25, oNONE, SECLEVEL::PLAIN, 1, false), new tuple_pos_node(1))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new tuple_pos_node(2))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(3), false))), },
      new local_group_by(
        {0, 1, 2},
        new local_transform_op(
          {local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new extract_node(new tuple_pos_node(0), extract_node::TYPE_YEAR))), local_transform_op::trfm_desc(3), },
          new local_decrypt_op(
            {1, 2, 3},
            new local_filter_op(
              new and_node(new ge_node(new tuple_pos_node(0), new date_literal_node("1995-1-1")), new le_node(new tuple_pos_node(0), new date_literal_node("1996-12-31"))),
              new local_decrypt_op(
                {0},
                new remote_sql_op(new param_generator_id8, "select lineitem_enc_cryptdb_opt_with_det.l_shipdate_DET, n1.n_name_DET as supp_nation, n2.n_name_DET as cust_nation, lineitem_enc_cryptdb_opt_with_det.l_disc_price_DET as volume from supplier_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det, orders_enc_cryptdb_opt_with_det, customer_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det n1, nation_enc_cryptdb_opt_with_det n2 where ((((((supplier_enc_cryptdb_opt_with_det.s_suppkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_suppkey_DET)) and ((orders_enc_cryptdb_opt_with_det.o_orderkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET))) and ((customer_enc_cryptdb_opt_with_det.c_custkey_DET) = (orders_enc_cryptdb_opt_with_det.o_custkey_DET))) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (n1.n_nationkey_DET))) and ((customer_enc_cryptdb_opt_with_det.c_nationkey_DET) = (n2.n_nationkey_DET))) and ((((n1.n_name_DET) = (:0)) and ((n2.n_name_DET) = (:1))) or (((n1.n_name_DET) = (:2)) and ((n2.n_name_DET) = (:3))))", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
              ,{
              }
            )
          )
        )
      )
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
static void query_7(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(0, false)},
    new local_transform_op(
      {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new tuple_pos_node(0))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new sum_node(new case_when_node({new case_expr_case_node(new eq_node(new tuple_pos_node(2), new string_literal_node("ARGENTINA")), new tuple_pos_node(1))}, new int_literal_node(0)), false), new sum_node(new tuple_pos_node(1), false)))), },
      new local_group_by(
        {0},
        new local_transform_op(
          {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new extract_node(new tuple_pos_node(0), extract_node::TYPE_YEAR))), local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(2), },
          new local_decrypt_op(
            {1, 2},
            new local_filter_op(
              new and_node(new ge_node(new tuple_pos_node(0), new date_literal_node("1995-1-1")), new le_node(new tuple_pos_node(0), new date_literal_node("1996-12-31"))),
              new local_decrypt_op(
                {0},
                new remote_sql_op(new param_generator_id9, "select orders_enc_cryptdb_opt_with_det.o_orderdate_DET, lineitem_enc_cryptdb_opt_with_det.l_disc_price_DET as volume, n2.n_name_DET as nation from part_enc_cryptdb_opt_with_det, supplier_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det, orders_enc_cryptdb_opt_with_det, customer_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det n1, nation_enc_cryptdb_opt_with_det n2, region_enc_cryptdb_opt_with_det where (((((((((part_enc_cryptdb_opt_with_det.p_partkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_partkey_DET)) and ((supplier_enc_cryptdb_opt_with_det.s_suppkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_suppkey_DET))) and ((lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET) = (orders_enc_cryptdb_opt_with_det.o_orderkey_DET))) and ((orders_enc_cryptdb_opt_with_det.o_custkey_DET) = (customer_enc_cryptdb_opt_with_det.c_custkey_DET))) and ((customer_enc_cryptdb_opt_with_det.c_nationkey_DET) = (n1.n_nationkey_DET))) and ((n1.n_regionkey_DET) = (region_enc_cryptdb_opt_with_det.r_regionkey_DET))) and ((region_enc_cryptdb_opt_with_det.r_name_DET) = (:0))) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (n2.n_nationkey_DET))) and ((part_enc_cryptdb_opt_with_det.p_type_DET) = (:1))", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 0, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
              ,{
              }
            )
          )
        )
      )
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
static void query_8(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(0, false), std::make_pair(1, true)},
    new local_transform_op(
      {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_STRING, 25, oNONE, SECLEVEL::PLAIN, 1, false), new tuple_pos_node(0))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new tuple_pos_node(1))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(2), false))), },
      new local_group_by(
        {0, 1},
        new local_transform_op(
          {local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new extract_node(new tuple_pos_node(2), extract_node::TYPE_YEAR))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sub_node(new tuple_pos_node(3), new mult_node(new tuple_pos_node(4), new tuple_pos_node(5))))), },
          new local_decrypt_op(
            {1, 2, 3, 4, 5},
            new local_filter_op(
              new like_node(new tuple_pos_node(0), new string_literal_node("%sky%"), false),
              new local_decrypt_op(
                {0},
                new remote_sql_op(new param_generator_id10, "select part_enc_cryptdb_opt_with_det.p_name_DET, nation_enc_cryptdb_opt_with_det.n_name_DET as nation, orders_enc_cryptdb_opt_with_det.o_orderdate_DET, lineitem_enc_cryptdb_opt_with_det.l_disc_price_DET, partsupp_enc_cryptdb_opt_with_det.ps_supplycost_DET, lineitem_enc_cryptdb_opt_with_det.l_quantity_DET from part_enc_cryptdb_opt_with_det, supplier_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det, partsupp_enc_cryptdb_opt_with_det, orders_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det where ((((((supplier_enc_cryptdb_opt_with_det.s_suppkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_suppkey_DET)) and ((partsupp_enc_cryptdb_opt_with_det.ps_suppkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_suppkey_DET))) and ((partsupp_enc_cryptdb_opt_with_det.ps_partkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_partkey_DET))) and ((part_enc_cryptdb_opt_with_det.p_partkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_partkey_DET))) and ((orders_enc_cryptdb_opt_with_det.o_orderkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET))) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET))", {db_column_desc(db_elem::TYPE_STRING, 55, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 0, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
              ,{
              }
            )
          )
        )
      )
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
static void query_9(exec_context& ctx) {
  physical_operator* op = new local_limit(20, 
    new local_order_by(
      {std::make_pair(2, true)},
      new local_transform_op(
        {local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(3), false))), local_transform_op::trfm_desc(4), local_transform_op::trfm_desc(5), local_transform_op::trfm_desc(6), local_transform_op::trfm_desc(7), local_transform_op::trfm_desc(8), },
        new local_decrypt_op(
          {1, 2, 3, 4, 5, 6, 7, 8},
          new local_filter_op(
            new and_node(new ge_node(new tuple_pos_node(0), new date_literal_node("1994-8-1")), new lt_node(new tuple_pos_node(0), new date_literal_node("1994-11-1"))),
            new local_decrypt_op(
              {0},
              new remote_sql_op(new param_generator_id11, "select group_serializer(orders_enc_cryptdb_opt_with_det.o_orderdate_DET), customer_enc_cryptdb_opt_with_det.c_custkey_DET, customer_enc_cryptdb_opt_with_det.c_name_DET, group_serializer(lineitem_enc_cryptdb_opt_with_det.l_disc_price_DET), customer_enc_cryptdb_opt_with_det.c_acctbal_DET, nation_enc_cryptdb_opt_with_det.n_name_DET, customer_enc_cryptdb_opt_with_det.c_address_DET, customer_enc_cryptdb_opt_with_det.c_phone_DET, customer_enc_cryptdb_opt_with_det.c_comment_DET from customer_enc_cryptdb_opt_with_det, orders_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det where ((((customer_enc_cryptdb_opt_with_det.c_custkey_DET) = (orders_enc_cryptdb_opt_with_det.o_custkey_DET)) and ((lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET) = (orders_enc_cryptdb_opt_with_det.o_orderkey_DET))) and ((lineitem_enc_cryptdb_opt_with_det.l_returnflag_DET) = (:0))) and ((customer_enc_cryptdb_opt_with_det.c_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET)) group by customer_enc_cryptdb_opt_with_det.c_custkey_DET, customer_enc_cryptdb_opt_with_det.c_name_DET, customer_enc_cryptdb_opt_with_det.c_acctbal_DET, customer_enc_cryptdb_opt_with_det.c_phone_DET, nation_enc_cryptdb_opt_with_det.n_name_DET, customer_enc_cryptdb_opt_with_det.c_address_DET, customer_enc_cryptdb_opt_with_det.c_comment_DET", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 0, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 40, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_STRING, 117, oDET, SECLEVEL::DET, 7, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
            ,{
            }
          )
        )
      )
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
static void query_10(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(1, true)},
    new local_transform_op(
      {local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(0), new tuple_pos_node(1)), false))), },
      new local_decrypt_op(
        {2},
        new local_group_filter(
          new gt_node(new sum_node(new mult_node(new tuple_pos_node(0), new tuple_pos_node(1)), false), new subselect_node(0, {})),
          new local_decrypt_op(
            {0, 1},
            new remote_sql_op(new param_generator_id12, "select group_serializer(partsupp_enc_cryptdb_opt_with_det.ps_supplycost_DET), group_serializer(partsupp_enc_cryptdb_opt_with_det.ps_availqty_DET), partsupp_enc_cryptdb_opt_with_det.ps_partkey_DET from partsupp_enc_cryptdb_opt_with_det, supplier_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det where (((partsupp_enc_cryptdb_opt_with_det.ps_suppkey_DET) = (supplier_enc_cryptdb_opt_with_det.s_suppkey_DET)) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET))) and ((nation_enc_cryptdb_opt_with_det.n_name_DET) = (:0)) group by partsupp_enc_cryptdb_opt_with_det.ps_partkey_DET", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          , {
            new local_transform_op(
              {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new sum_node(new mult_node(new tuple_pos_node(0), new tuple_pos_node(1)), false), new double_literal_node(0.000100)))), },
              new local_decrypt_op(
                {0, 1},
                new remote_sql_op(new param_generator_id13, "select group_serializer(partsupp_enc_cryptdb_opt_with_det.ps_supplycost_DET), group_serializer(partsupp_enc_cryptdb_opt_with_det.ps_availqty_DET) from partsupp_enc_cryptdb_opt_with_det, supplier_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det where (((partsupp_enc_cryptdb_opt_with_det.ps_suppkey_DET) = (supplier_enc_cryptdb_opt_with_det.s_suppkey_DET)) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET))) and ((nation_enc_cryptdb_opt_with_det.n_name_DET) = (:0))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
            )
            ,
          }
        )
      )
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
static void query_11(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(0, false)},
    new local_transform_op(
      {local_transform_op::trfm_desc(3), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 8, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new case_when_node({new case_expr_case_node(new or_node(new eq_node(new tuple_pos_node(4), new string_literal_node("1-URGENT")), new eq_node(new tuple_pos_node(4), new string_literal_node("2-HIGH"))), new int_literal_node(1))}, new int_literal_node(0)), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 8, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new case_when_node({new case_expr_case_node(new and_node(new neq_node(new tuple_pos_node(4), new string_literal_node("1-URGENT")), new neq_node(new tuple_pos_node(4), new string_literal_node("2-HIGH"))), new int_literal_node(1))}, new int_literal_node(0)), false))), },
      new local_decrypt_op(
        {3, 4},
        new local_filter_op(
          new and_node(new and_node(new and_node(new lt_node(new tuple_pos_node(0), new tuple_pos_node(1)), new lt_node(new tuple_pos_node(2), new tuple_pos_node(0))), new ge_node(new tuple_pos_node(1), new date_literal_node("1993-1-1"))), new lt_node(new tuple_pos_node(1), new date_literal_node("1994-1-1"))),
          new local_decrypt_op(
            {0, 1, 2},
            new remote_sql_op(new param_generator_id14, "select group_serializer(lineitem_enc_cryptdb_opt_with_det.l_commitdate_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_receiptdate_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_shipdate_DET), lineitem_enc_cryptdb_opt_with_det.l_shipmode_DET, array_to_string(array_agg(encode(orders_enc_cryptdb_opt_with_det.o_orderpriority_DET, 'hex')), ',') from orders_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det where ((orders_enc_cryptdb_opt_with_det.o_orderkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET)) and (lineitem_enc_cryptdb_opt_with_det.l_shipmode_DET in ( :0, :1 )) group by lineitem_enc_cryptdb_opt_with_det.l_shipmode_DET", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 11, true), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 12, true), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, true), db_column_desc(db_elem::TYPE_STRING, 10, oDET, SECLEVEL::DET, 14, false), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 5, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          ,{
          }
        )
      )
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
static void query_12(exec_context& ctx) {
  physical_operator* op = new local_transform_op(
    {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new mult_node(new double_literal_node(100.000000), new sum_node(new case_when_node({new case_expr_case_node(new like_node(new tuple_pos_node(1), new string_literal_node("PROMO%"), false), new mult_node(new tuple_pos_node(2), new sub_node(new int_literal_node(1), new tuple_pos_node(3))))}, new int_literal_node(0)), false)), new sum_node(new mult_node(new tuple_pos_node(2), new sub_node(new int_literal_node(1), new tuple_pos_node(3))), false)))), },
    new local_decrypt_op(
      {1, 2, 3},
      new local_filter_op(
        new and_node(new ge_node(new tuple_pos_node(0), new date_literal_node("1996-7-1")), new lt_node(new tuple_pos_node(0), new date_literal_node("1996-8-1"))),
        new local_decrypt_op(
          {0},
          new remote_sql_op(new param_generator_id15, "select group_serializer(lineitem_enc_cryptdb_opt_with_det.l_shipdate_DET), array_to_string(array_agg(encode(part_enc_cryptdb_opt_with_det.p_type_DET, 'hex')), ','), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_extendedprice_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_discount_DET) from lineitem_enc_cryptdb_opt_with_det, part_enc_cryptdb_opt_with_det where (lineitem_enc_cryptdb_opt_with_det.l_partkey_DET) = (part_enc_cryptdb_opt_with_det.p_partkey_DET)", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, true), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
        ,{
        }
      )
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
static void query_13(exec_context& ctx) {
  physical_operator* op = new local_transform_op(
    {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new double_literal_node(0.200000), new avg_node(new tuple_pos_node(1), false)))), },
    new local_decrypt_op(
      {0, 1},
      new remote_sql_op(new param_generator_id16, "select lineitem_enc_cryptdb_opt_with_det.l_partkey_DET, group_serializer(lineitem_enc_cryptdb_opt_with_det.l_quantity_DET) from lineitem_enc_cryptdb_opt_with_det, part_enc_cryptdb_opt_with_det where (((part_enc_cryptdb_opt_with_det.p_partkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_partkey_DET)) and ((part_enc_cryptdb_opt_with_det.p_brand_DET) = (:0))) and ((part_enc_cryptdb_opt_with_det.p_container_DET) = (:1)) group by lineitem_enc_cryptdb_opt_with_det.l_partkey_DET", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
static void query_14(exec_context& ctx) {
  physical_operator* op = new local_limit(100, 
    new local_order_by(
      {std::make_pair(4, true), std::make_pair(3, false)},
      new local_transform_op(
        {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(3), local_transform_op::trfm_desc(4), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(5), false))), },
        new local_decrypt_op(
          {0, 1, 2, 3, 4, 5},
          new remote_sql_op(new param_generator_id18, "select customer_enc_cryptdb_opt_with_det.c_name_DET, customer_enc_cryptdb_opt_with_det.c_custkey_DET, orders_enc_cryptdb_opt_with_det.o_orderkey_DET, orders_enc_cryptdb_opt_with_det.o_orderdate_DET, orders_enc_cryptdb_opt_with_det.o_totalprice_DET, group_serializer(lineitem_enc_cryptdb_opt_with_det.l_quantity_DET) from customer_enc_cryptdb_opt_with_det, orders_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det where ((orders_enc_cryptdb_opt_with_det.o_orderkey_DET in ( :_subselect$0 )) and ((customer_enc_cryptdb_opt_with_det.c_custkey_DET) = (orders_enc_cryptdb_opt_with_det.o_custkey_DET))) and ((orders_enc_cryptdb_opt_with_det.o_orderkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET)) group by customer_enc_cryptdb_opt_with_det.c_name_DET, customer_enc_cryptdb_opt_with_det.c_custkey_DET, orders_enc_cryptdb_opt_with_det.o_orderkey_DET, orders_enc_cryptdb_opt_with_det.o_orderdate_DET, orders_enc_cryptdb_opt_with_det.o_totalprice_DET", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({std::pair<std::string, physical_operator*>("_subselect$0", new local_transform_op(
            {local_transform_op::trfm_desc(1), },
            new local_group_filter(
              new gt_node(new sum_node(new tuple_pos_node(0), false), new int_literal_node(315)),
              new local_decrypt_op(
                {0},
                new remote_sql_op(new param_generator_id17, "select group_serializer(lineitem_enc_cryptdb_opt_with_det.l_quantity_DET), lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET from lineitem_enc_cryptdb_opt_with_det group by lineitem_enc_cryptdb_opt_with_det.l_orderkey_DET", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
              , {
              }
            )
          )
          )})))
      )
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
static void query_15(exec_context& ctx) {
  physical_operator* op = new local_transform_op(
    {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(0), false))), },
    new local_decrypt_op(
      {0},
      new remote_sql_op(new param_generator_id19, "select group_serializer(lineitem_enc_cryptdb_opt_with_det.l_disc_price_DET) from lineitem_enc_cryptdb_opt_with_det, part_enc_cryptdb_opt_with_det where ((((((((((part_enc_cryptdb_opt_with_det.p_partkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_partkey_DET)) and ((part_enc_cryptdb_opt_with_det.p_brand_DET) = (:0))) and (part_enc_cryptdb_opt_with_det.p_container_DET in ( :1, :2, :3, :4 ))) and ((lineitem_enc_cryptdb_opt_with_det.l_quantity_OPE) >= (:5))) and ((lineitem_enc_cryptdb_opt_with_det.l_quantity_OPE) <= (:6))) and (((part_enc_cryptdb_opt_with_det.p_size_OPE) >= (:7)) and ((part_enc_cryptdb_opt_with_det.p_size_OPE) <= (:8)))) and (lineitem_enc_cryptdb_opt_with_det.l_shipmode_DET in ( :9, :10 ))) and ((lineitem_enc_cryptdb_opt_with_det.l_shipinstruct_DET) = (:11))) or (((((((((part_enc_cryptdb_opt_with_det.p_partkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_partkey_DET)) and ((part_enc_cryptdb_opt_with_det.p_brand_DET) = (:12))) and (part_enc_cryptdb_opt_with_det.p_container_DET in ( :13, :14, :15, :16 ))) and ((lineitem_enc_cryptdb_opt_with_det.l_quantity_OPE) >= (:17))) and ((lineitem_enc_cryptdb_opt_with_det.l_quantity_OPE) <= (:18))) and (((part_enc_cryptdb_opt_with_det.p_size_OPE) >= (:19)) and ((part_enc_cryptdb_opt_with_det.p_size_OPE) <= (:20)))) and (lineitem_enc_cryptdb_opt_with_det.l_shipmode_DET in ( :21, :22 ))) and ((lineitem_enc_cryptdb_opt_with_det.l_shipinstruct_DET) = (:23)))) or (((((((((part_enc_cryptdb_opt_with_det.p_partkey_DET) = (lineitem_enc_cryptdb_opt_with_det.l_partkey_DET)) and ((part_enc_cryptdb_opt_with_det.p_brand_DET) = (:24))) and (part_enc_cryptdb_opt_with_det.p_container_DET in ( :25, :26, :27, :28 ))) and ((lineitem_enc_cryptdb_opt_with_det.l_quantity_OPE) >= (:29))) and ((lineitem_enc_cryptdb_opt_with_det.l_quantity_OPE) <= (:30))) and (((part_enc_cryptdb_opt_with_det.p_size_OPE) >= (:31)) and ((part_enc_cryptdb_opt_with_det.p_size_OPE) <= (:32)))) and (lineitem_enc_cryptdb_opt_with_det.l_shipmode_DET in ( :33, :34 ))) and ((lineitem_enc_cryptdb_opt_with_det.l_shipinstruct_DET) = (:35)))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 0, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
static void query_16(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(0, false)},
    new local_decrypt_op(
      {0, 1},
      new remote_sql_op(new param_generator_id22, "select supplier_enc_cryptdb_opt_with_det.s_name_DET, supplier_enc_cryptdb_opt_with_det.s_address_DET from supplier_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det where ((supplier_enc_cryptdb_opt_with_det.s_suppkey_DET in ( :_subselect$2 )) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET))) and ((nation_enc_cryptdb_opt_with_det.n_name_DET) = (:0))", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 40, oDET, SECLEVEL::DET, 2, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({std::pair<std::string, physical_operator*>("_subselect$2", new local_encrypt_op({std::pair<size_t, db_column_desc>(0, db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 1, false))}, new local_transform_op(
        {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 1, false), new tuple_pos_node(0))), },
        new local_filter_op(
          new gt_node(new tuple_pos_node(1), new mult_node(new double_literal_node(0.500000), new tuple_pos_node(2))),
          new local_transform_op(
            {local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 2, false), new min_node(new tuple_pos_node(2)))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(3), false))), },
            new local_decrypt_op(
              {1, 2, 3},
              new local_filter_op(
                new and_node(new ge_node(new tuple_pos_node(0), new date_literal_node("1997-1-1")), new lt_node(new tuple_pos_node(0), new date_literal_node("1998-1-1"))),
                new local_decrypt_op(
                  {0},
                  new remote_sql_op(new param_generator_id21, "select group_serializer(lineitem_enc_cryptdb_opt_with_det.l_shipdate_DET), partsupp_enc_cryptdb_opt_with_det.ps_suppkey_DET, group_serializer(partsupp_enc_cryptdb_opt_with_det.ps_availqty_DET), group_serializer(lineitem_enc_cryptdb_opt_with_det.l_quantity_DET) from partsupp_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det where (((lineitem_enc_cryptdb_opt_with_det.l_partkey_DET) = (partsupp_enc_cryptdb_opt_with_det.ps_partkey_DET)) and ((lineitem_enc_cryptdb_opt_with_det.l_suppkey_DET) = (partsupp_enc_cryptdb_opt_with_det.ps_suppkey_DET))) and (partsupp_enc_cryptdb_opt_with_det.ps_partkey_DET in ( :_subselect$1 )) group by partsupp_enc_cryptdb_opt_with_det.ps_partkey_DET, partsupp_enc_cryptdb_opt_with_det.ps_suppkey_DET", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 1, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({std::pair<std::string, physical_operator*>("_subselect$1", new local_transform_op(
                    {local_transform_op::trfm_desc(1), },
                    new local_filter_op(
                      new like_node(new tuple_pos_node(0), new string_literal_node("%khaki%"), false),
                      new local_decrypt_op(
                        {0},
                        new remote_sql_op(new param_generator_id20, "select part_enc_cryptdb_opt_with_det.p_name_DET, part_enc_cryptdb_opt_with_det.p_partkey_DET from part_enc_cryptdb_opt_with_det", {db_column_desc(db_elem::TYPE_STRING, 55, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
                      ,{
                      }
                    )
                  )
                  )})))
                ,{
                }
              )
            )
          )
          ,{
          }
        )
      )
      ))})))
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
static void query_17(exec_context& ctx) {
  physical_operator* op = new local_limit(100, 
    new local_order_by(
      {std::make_pair(1, true), std::make_pair(0, false)},
      new local_transform_op(
        {local_transform_op::trfm_desc(4), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false), new count_star_node)), },
        new local_decrypt_op(
          {4},
          new local_filter_op(
            new and_node(new gt_node(new tuple_pos_node(0), new tuple_pos_node(1)), new not_node(new exists_node(new subselect_node(0, {new tuple_pos_node(2), new tuple_pos_node(3)})))),
            new local_decrypt_op(
              {0, 1},
              new remote_sql_op(new param_generator_id23, "select group_serializer(l1.l_receiptdate_DET), group_serializer(l1.l_commitdate_DET), group_serializer(l1.l_orderkey_DET), group_serializer(l1.l_suppkey_DET), supplier_enc_cryptdb_opt_with_det.s_name_DET from supplier_enc_cryptdb_opt_with_det, lineitem_enc_cryptdb_opt_with_det l1, orders_enc_cryptdb_opt_with_det, nation_enc_cryptdb_opt_with_det where ((((((supplier_enc_cryptdb_opt_with_det.s_suppkey_DET) = (l1.l_suppkey_DET)) and ((orders_enc_cryptdb_opt_with_det.o_orderkey_DET) = (l1.l_orderkey_DET))) and ((orders_enc_cryptdb_opt_with_det.o_orderstatus_DET) = (:0))) and (exists ( (select 1 from lineitem_enc_cryptdb_opt_with_det l2 where ((l2.l_orderkey_DET) = (l1.l_orderkey_DET)) and ((l2.l_suppkey_DET) <> (l1.l_suppkey_DET))) ))) and ((supplier_enc_cryptdb_opt_with_det.s_nationkey_DET) = (nation_enc_cryptdb_opt_with_det.n_nationkey_DET))) and ((nation_enc_cryptdb_opt_with_det.n_name_DET) = (:1)) group by supplier_enc_cryptdb_opt_with_det.s_name_DET", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 12, true), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 11, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, true), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
            ,{
              new local_filter_op(
                new gt_node(new tuple_pos_node(0), new tuple_pos_node(1)),
                new local_decrypt_op(
                  {0, 1},
                  new remote_sql_op(new param_generator_id24, "select l3.l_receiptdate_DET, l3.l_commitdate_DET from lineitem_enc_cryptdb_opt_with_det l3 where ((l3.l_orderkey_DET) = (:0)) and ((l3.l_suppkey_DET) <> (:1))", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 12, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 11, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
                ,{
                }
              )
              ,
            }
          )
        )
      )
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
static void query_18(exec_context& ctx) {
  physical_operator* op = new local_order_by(
    {std::make_pair(0, false)},
    new local_transform_op(
      {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_STRING, 15, oNONE, SECLEVEL::PLAIN, 4, false), new tuple_pos_node(0))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false), new count_star_node)), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(1), false))), },
      new local_group_by(
        {0},
        new local_transform_op(
          {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_STRING, 15, oNONE, SECLEVEL::PLAIN, 4, false), new substring_node(new tuple_pos_node(0), 1, 2))), local_transform_op::trfm_desc(1), },
          new local_filter_op(
            new and_node(new in_node(new substring_node(new tuple_pos_node(0), 1, 2), {new string_literal_node("41"), new string_literal_node("26"), new string_literal_node("36"), new string_literal_node("27"), new string_literal_node("38"), new string_literal_node("37"), new string_literal_node("22")}), new gt_node(new tuple_pos_node(1), new subselect_node(0, {}))),
            new local_decrypt_op(
              {0, 1},
              new remote_sql_op(new param_generator_id25, "select customer_enc_cryptdb_opt_with_det.c_phone_DET, customer_enc_cryptdb_opt_with_det.c_acctbal_DET from customer_enc_cryptdb_opt_with_det where not ( exists ( (select 1 from orders_enc_cryptdb_opt_with_det where (orders_enc_cryptdb_opt_with_det.o_custkey_DET) = (customer_enc_cryptdb_opt_with_det.c_custkey_DET)) ) )", {db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
            ,{
              new local_transform_op(
                {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new avg_node(new tuple_pos_node(0), false))), },
                new local_filter_op(
                  new and_node(new gt_node(new tuple_pos_node(0), new double_literal_node(0.000000)), new in_node(new substring_node(new tuple_pos_node(1), 1, 2), {new string_literal_node("41"), new string_literal_node("26"), new string_literal_node("36"), new string_literal_node("27"), new string_literal_node("38"), new string_literal_node("37"), new string_literal_node("22")})),
                  new local_decrypt_op(
                    {0, 1},
                    new remote_sql_op(new param_generator_id26, "select group_serializer(customer_enc_cryptdb_opt_with_det.c_acctbal_DET), array_to_string(array_agg(encode(customer_enc_cryptdb_opt_with_det.c_phone_DET, 'hex')), ',') from customer_enc_cryptdb_opt_with_det", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
                  ,{
                  }
                )
              )
              ,
            }
          )
        )
      )
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
  command_line_opts opts(DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PORT);
  command_line_opts::parse_options(argc, argv, opts);
  std::cerr << opts << std::endl;
  if ((optind + 1) != argc) {
    std::cerr << "[Usage]: " << argv[0] << " [options] [query num]" << std::endl;
    return 1;
  }
  int q = atoi(argv[optind]);
  CryptoManager cm("12345");
  crypto_manager_stub cm_stub(&cm, CRYPTO_USE_OLD_OPE);
  PGConnect pg(opts.db_hostname, opts.db_username, opts.db_passwd, opts.db_database, opts.db_port, true);
  paillier_cache pp_cache;
  query_cache cache;
  exec_context ctx(&pg, &cm_stub, &pp_cache, NULL, &cache);
  switch (q) {
    case 0: query_0(ctx); break;
    case 1: query_1(ctx); break;
    case 2: query_2(ctx); break;
    case 3: query_3(ctx); break;
    case 4: query_4(ctx); break;
    case 5: query_5(ctx); break;
    case 6: query_6(ctx); break;
    case 7: query_7(ctx); break;
    case 8: query_8(ctx); break;
    case 9: query_9(ctx); break;
    case 10: query_10(ctx); break;
    case 11: query_11(ctx); break;
    case 12: query_12(ctx); break;
    case 13: query_13(ctx); break;
    case 14: query_14(ctx); break;
    case 15: query_15(ctx); break;
    case 16: query_16(ctx); break;
    case 17: query_17(ctx); break;
    case 18: query_18(ctx); break;
    default: assert(false);
  }
  return 0;
}
