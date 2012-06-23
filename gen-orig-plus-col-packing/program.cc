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
#include <util/util.hh>
class param_generator_id0 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  {
    static const size_t RowColPackPlainSize = 1024;
    static const size_t RowColPackCipherSize = RowColPackPlainSize * 2;
    auto pp = ctx.pp_cache->get_paillier_priv(RowColPackCipherSize / 2, RowColPackCipherSize / 8);
    auto pk = pp.pubkey();
    m[0] = (RowColPackCipherSize == 2048) ? db_elem(ctx.crypto->cm->getPKInfo()) : db_elem(StringFromZZ(pk[0] * pk[0]));
  }
  m[1] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1023009 /*1998-1-1*/, 10, true));
  return m;
}
};
class param_generator_id1 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_u32_det(ctx.crypto, 36, 5, false));
  {
    Binary key(ctx.crypto->cm->getKey(ctx.crypto->cm->getmkey(), fieldname(4, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary("steel"));
    m[1] = db_elem((const char *)t.ciph.content, t.ciph.len);
    m[2] = db_elem((const char *)t.wordKey.content, t.wordKey.len);
  }
  m[3] = db_elem(encrypt_string_det(ctx.crypto, "ASIA", 1, false));
  m[4] = db_elem(encrypt_string_det(ctx.crypto, "ASIA", 1, false));
  return m;
}
};
class param_generator_id2 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "FURNITURE", 6, false));
  m[1] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1021548 /*1995-3-12*/, 4, false));
  m[2] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1021548 /*1995-3-12*/, 10, true));
  return m;
}
};
class param_generator_id3 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1021281 /*1994-11-1*/, 4, false));
  m[1] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1021505 /*1995-2-1*/, 4, false));
  return m;
}
};
class param_generator_id4 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "MIDDLE EAST", 1, false));
  m[1] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1020449 /*1993-1-1*/, 4, false));
  m[2] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1020961 /*1994-1-1*/, 4, false));
  return m;
}
};
class param_generator_id5 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1020449 /*1993-1-1*/, 10, true));
  m[1] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1020961 /*1994-1-1*/, 10, true));
  m[2] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 4 /*0.04*/, 6, false), 16)));
  m[3] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 6 /*0.060000000000000005*/, 6, false), 16)));
  m[4] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 2500 /*25*/, 4, false), 16)));
  return m;
}
};
class param_generator_id6 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "SAUDI ARABIA", 1, false));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "ARGENTINA", 1, false));
  m[2] = db_elem(encrypt_string_det(ctx.crypto, "ARGENTINA", 1, false));
  m[3] = db_elem(encrypt_string_det(ctx.crypto, "SAUDI ARABIA", 1, false));
  m[4] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1021473 /*1995-1-1*/, 10, true));
  m[5] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1022367 /*1996-12-31*/, 10, true));
  return m;
}
};
class param_generator_id7 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "AMERICA", 1, false));
  m[1] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1021473 /*1995-1-1*/, 4, false));
  m[2] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1022367 /*1996-12-31*/, 4, false));
  m[3] = db_elem(encrypt_string_det(ctx.crypto, "PROMO ANODIZED BRASS", 4, false));
  return m;
}
};
class param_generator_id8 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  {
    Binary key(ctx.crypto->cm->getKey(ctx.crypto->cm->getmkey(), fieldname(1, "SWP"), SECLEVEL::SWP));
    Token t = CryptoManager::token(key, Binary("sky"));
    m[0] = db_elem((const char *)t.ciph.content, t.ciph.len);
    m[1] = db_elem((const char *)t.wordKey.content, t.wordKey.len);
  }
  return m;
}
};
class param_generator_id9 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1021185 /*1994-8-1*/, 4, false));
  m[1] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1021281 /*1994-11-1*/, 4, false));
  m[2] = db_elem((int64_t)encrypt_u8_det(ctx.crypto, 82 /*R*/, 8, false));
  return m;
}
};
class param_generator_id10 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "ARGENTINA", 1, false));
  return m;
}
};
class param_generator_id11 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "ARGENTINA", 1, false));
  return m;
}
};
class param_generator_id12 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "1-URGENT", 5, false));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "2-HIGH", 5, false));
  m[2] = db_elem(encrypt_string_det(ctx.crypto, "1-URGENT", 5, false));
  m[3] = db_elem(encrypt_string_det(ctx.crypto, "2-HIGH", 5, false));
  m[4] = db_elem(encrypt_string_det(ctx.crypto, "AIR", 14, false));
  m[5] = db_elem(encrypt_string_det(ctx.crypto, "FOB", 14, false));
  m[6] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1020449 /*1993-1-1*/, 12, true));
  m[7] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1020961 /*1994-1-1*/, 12, true));
  return m;
}
};
class param_generator_id13 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1022177 /*1996-7-1*/, 10, true));
  m[1] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1022209 /*1996-8-1*/, 10, true));
  return m;
}
};
class param_generator_id14 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  {
    static const size_t RowColPackPlainSize = 1024;
    static const size_t RowColPackCipherSize = RowColPackPlainSize * 2;
    auto pp = ctx.pp_cache->get_paillier_priv(RowColPackCipherSize / 2, RowColPackCipherSize / 8);
    auto pk = pp.pubkey();
    m[0] = (RowColPackCipherSize == 2048) ? db_elem(ctx.crypto->cm->getPKInfo()) : db_elem(StringFromZZ(pk[0] * pk[0]));
  }
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "Brand#45", 3, false));
  m[2] = db_elem(encrypt_string_det(ctx.crypto, "LG BOX", 6, false));
  return m;
}
};
class param_generator_id15 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  {
    static const size_t RowColPackPlainSize = 1024;
    static const size_t RowColPackCipherSize = RowColPackPlainSize * 2;
    auto pp = ctx.pp_cache->get_paillier_priv(RowColPackCipherSize / 2, RowColPackCipherSize / 8);
    auto pk = pp.pubkey();
    m[0] = (RowColPackCipherSize == 2048) ? db_elem(ctx.crypto->cm->getPKInfo()) : db_elem(StringFromZZ(pk[0] * pk[0]));
  }
  return m;
}
};
class param_generator_id16 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  {
    static const size_t RowColPackPlainSize = 1024;
    static const size_t RowColPackCipherSize = RowColPackPlainSize * 2;
    auto pp = ctx.pp_cache->get_paillier_priv(RowColPackCipherSize / 2, RowColPackCipherSize / 8);
    auto pk = pp.pubkey();
    m[0] = (RowColPackCipherSize == 2048) ? db_elem(ctx.crypto->cm->getPKInfo()) : db_elem(StringFromZZ(pk[0] * pk[0]));
  }
  return m;
}
};
class param_generator_id17 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(encrypt_string_det(ctx.crypto, "Brand#23", 3, false));
  m[1] = db_elem(encrypt_string_det(ctx.crypto, "SM CASE", 6, false));
  m[2] = db_elem(encrypt_string_det(ctx.crypto, "SM BOX", 6, false));
  m[3] = db_elem(encrypt_string_det(ctx.crypto, "SM PACK", 6, false));
  m[4] = db_elem(encrypt_string_det(ctx.crypto, "SM PKG", 6, false));
  m[5] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 1000 /*10*/, 4, false), 16)));
  m[6] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 2000 /*20*/, 4, false), 16)));
  m[7] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 1, 5, false));
  m[8] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 5, 5, false));
  m[9] = db_elem(encrypt_string_det(ctx.crypto, "AIR", 14, false));
  m[10] = db_elem(encrypt_string_det(ctx.crypto, "AIR REG", 14, false));
  m[11] = db_elem(encrypt_string_det(ctx.crypto, "DELIVER IN PERSON", 13, false));
  m[12] = db_elem(encrypt_string_det(ctx.crypto, "Brand#32", 3, false));
  m[13] = db_elem(encrypt_string_det(ctx.crypto, "MED BAG", 6, false));
  m[14] = db_elem(encrypt_string_det(ctx.crypto, "MED BOX", 6, false));
  m[15] = db_elem(encrypt_string_det(ctx.crypto, "MED PKG", 6, false));
  m[16] = db_elem(encrypt_string_det(ctx.crypto, "MED PACK", 6, false));
  m[17] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 2000 /*20*/, 4, false), 16)));
  m[18] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 3000 /*30*/, 4, false), 16)));
  m[19] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 1, 5, false));
  m[20] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 10, 5, false));
  m[21] = db_elem(encrypt_string_det(ctx.crypto, "AIR", 14, false));
  m[22] = db_elem(encrypt_string_det(ctx.crypto, "AIR REG", 14, false));
  m[23] = db_elem(encrypt_string_det(ctx.crypto, "DELIVER IN PERSON", 13, false));
  m[24] = db_elem(encrypt_string_det(ctx.crypto, "Brand#34", 3, false));
  m[25] = db_elem(encrypt_string_det(ctx.crypto, "LG CASE", 6, false));
  m[26] = db_elem(encrypt_string_det(ctx.crypto, "LG BOX", 6, false));
  m[27] = db_elem(encrypt_string_det(ctx.crypto, "LG PACK", 6, false));
  m[28] = db_elem(encrypt_string_det(ctx.crypto, "LG PKG", 6, false));
  m[29] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 2000 /*20*/, 4, false), 16)));
  m[30] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 3000 /*30*/, 4, false), 16)));
  m[31] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 1, 5, false));
  m[32] = db_elem((int64_t)encrypt_u32_ope(ctx.crypto, 15, 5, false));
  m[33] = db_elem(encrypt_string_det(ctx.crypto, "AIR", 14, false));
  m[34] = db_elem(encrypt_string_det(ctx.crypto, "AIR REG", 14, false));
  m[35] = db_elem(encrypt_string_det(ctx.crypto, "DELIVER IN PERSON", 13, false));
  return m;
}
};
class param_generator_id18 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  m[0] = db_elem(str_reverse(str_resize(encrypt_decimal_15_2_ope(ctx.crypto, 0 /*0.0*/, 5, false), 16)));
  return m;
}
};
class param_generator_id19 : public sql_param_generator {
public:
virtual param_map get_param_map(exec_context& ctx) {
  param_map m;
  return m;
}
};
static void query_0(exec_context& ctx) {
  physical_operator* op = new local_transform_op(
    {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new function_call_node("hom_get_pos", {new tuple_pos_node(2), new int_literal_node(1)}))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new function_call_node("hom_get_pos", {new tuple_pos_node(2), new int_literal_node(2)}))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(3), new sub_node(new int_literal_node(1), new tuple_pos_node(4))), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new mult_node(new tuple_pos_node(3), new sub_node(new int_literal_node(1), new tuple_pos_node(4))), new add_node(new int_literal_node(1), new tuple_pos_node(5))), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new function_call_node("hom_get_pos", {new tuple_pos_node(2), new int_literal_node(1)}), new tuple_pos_node(6)))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new function_call_node("hom_get_pos", {new tuple_pos_node(2), new int_literal_node(2)}), new tuple_pos_node(6)))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new function_call_node("hom_get_pos", {new tuple_pos_node(2), new int_literal_node(0)}), new tuple_pos_node(6)))), local_transform_op::trfm_desc(6), },
    new local_decrypt_op(
      {0, 1, 2, 3, 4, 5},
      new remote_sql_op(new param_generator_id0, "select agg_ident(" LINEITEM_ENC_NAME ".l_returnflag_DET), agg_ident(" LINEITEM_ENC_NAME ".l_linestatus_DET), hom_agg(:0, " LINEITEM_ENC_NAME ".l_packed_AGG), group_serializer(" LINEITEM_ENC_NAME ".l_extendedprice_DET), group_serializer(" LINEITEM_ENC_NAME ".l_discount_DET), group_serializer(" LINEITEM_ENC_NAME ".l_tax_DET), count(*) from " LINEITEM_ENC_NAME " where (" LINEITEM_ENC_NAME ".l_shipdate_OPE) <= (:1) group by " LINEITEM_ENC_NAME ".l_returnflag_OPE, " LINEITEM_ENC_NAME ".l_linestatus_OPE order by " LINEITEM_ENC_NAME ".l_returnflag_OPE ASC, " LINEITEM_ENC_NAME ".l_linestatus_OPE ASC", {db_column_desc(db_elem::TYPE_CHAR, 1, oDET, SECLEVEL::DET, 8, false), db_column_desc(db_elem::TYPE_CHAR, 1, oDET, SECLEVEL::DET, 9, false), db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 7, true), db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
  physical_operator* op = new local_decrypt_op(
    {0, 1, 2, 3, 4, 5, 6, 7},
    new remote_sql_op(new param_generator_id1, "select " SUPPLIER_ENC_NAME ".s_acctbal_DET, " SUPPLIER_ENC_NAME ".s_name_DET, " NATION_ENC_NAME ".n_name_DET, " PART_ENC_NAME ".p_partkey_DET, " PART_ENC_NAME ".p_mfgr_DET, " SUPPLIER_ENC_NAME ".s_address_DET, " SUPPLIER_ENC_NAME ".s_phone_DET, " SUPPLIER_ENC_NAME ".s_comment_DET from " PART_ENC_NAME ", " SUPPLIER_ENC_NAME ", " PARTSUPP_ENC_NAME ", " NATION_ENC_NAME ", " REGION_ENC_NAME " where ((((((((" PART_ENC_NAME ".p_partkey_DET) = (" PARTSUPP_ENC_NAME ".ps_partkey_DET)) and ((" SUPPLIER_ENC_NAME ".s_suppkey_DET) = (" PARTSUPP_ENC_NAME ".ps_suppkey_DET))) and ((" PART_ENC_NAME ".p_size_DET) = (:0))) and (searchSWP(:1, :2, " PART_ENC_NAME ".p_type_SWP))) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET))) and ((" NATION_ENC_NAME ".n_regionkey_DET) = (" REGION_ENC_NAME ".r_regionkey_DET))) and ((" REGION_ENC_NAME ".r_name_DET) = (:3))) and ((" PARTSUPP_ENC_NAME ".ps_supplycost_OPE) = ((select min(" PARTSUPP_ENC_NAME ".ps_supplycost_OPE) from " PARTSUPP_ENC_NAME ", " SUPPLIER_ENC_NAME ", " NATION_ENC_NAME ", " REGION_ENC_NAME " where (((((" PART_ENC_NAME ".p_partkey_DET) = (" PARTSUPP_ENC_NAME ".ps_partkey_DET)) and ((" SUPPLIER_ENC_NAME ".s_suppkey_DET) = (" PARTSUPP_ENC_NAME ".ps_suppkey_DET))) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET))) and ((" NATION_ENC_NAME ".n_regionkey_DET) = (" REGION_ENC_NAME ".r_regionkey_DET))) and ((" REGION_ENC_NAME ".r_name_DET) = (:4))))) order by " SUPPLIER_ENC_NAME ".s_acctbal_OPE DESC, " NATION_ENC_NAME ".n_name_OPE ASC, " SUPPLIER_ENC_NAME ".s_name_OPE ASC, " PART_ENC_NAME ".p_partkey_OPE ASC limit 100", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 40, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_STRING, 101, oDET, SECLEVEL::DET, 6, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
        {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(1), new sub_node(new int_literal_node(1), new tuple_pos_node(2))), false))), local_transform_op::trfm_desc(3), local_transform_op::trfm_desc(4), },
        new local_decrypt_op(
          {0, 1, 2, 3, 4},
          new remote_sql_op(new param_generator_id2, "select " LINEITEM_ENC_NAME ".l_orderkey_DET, group_serializer(" LINEITEM_ENC_NAME ".l_extendedprice_DET), group_serializer(" LINEITEM_ENC_NAME ".l_discount_DET), " ORDERS_ENC_NAME ".o_orderdate_DET, " ORDERS_ENC_NAME ".o_shippriority_DET from " CUSTOMER_ENC_NAME ", " ORDERS_ENC_NAME ", " LINEITEM_ENC_NAME " where (((((" CUSTOMER_ENC_NAME ".c_mktsegment_DET) = (:0)) and ((" CUSTOMER_ENC_NAME ".c_custkey_DET) = (" ORDERS_ENC_NAME ".o_custkey_DET))) and ((" LINEITEM_ENC_NAME ".l_orderkey_DET) = (" ORDERS_ENC_NAME ".o_orderkey_DET))) and ((" ORDERS_ENC_NAME ".o_orderdate_OPE) < (:1))) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) > (:2)) group by " LINEITEM_ENC_NAME ".l_orderkey_DET, " ORDERS_ENC_NAME ".o_orderdate_DET, " ORDERS_ENC_NAME ".o_shippriority_DET", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 7, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
  physical_operator* op = new local_decrypt_op(
    {0},
    new remote_sql_op(new param_generator_id3, "select agg_ident(" ORDERS_ENC_NAME ".o_orderpriority_DET), count(*) as order_count from " ORDERS_ENC_NAME " where (((" ORDERS_ENC_NAME ".o_orderdate_OPE) >= (:0)) and ((" ORDERS_ENC_NAME ".o_orderdate_OPE) < (:1))) and (exists ( (select 1 from " LINEITEM_ENC_NAME " where ((" LINEITEM_ENC_NAME ".l_orderkey_DET) = (" ORDERS_ENC_NAME ".o_orderkey_DET)) and ((" LINEITEM_ENC_NAME ".l_commitdate_OPE) < (" LINEITEM_ENC_NAME ".l_receiptdate_OPE))) )) group by " ORDERS_ENC_NAME ".o_orderpriority_OPE order by " ORDERS_ENC_NAME ".o_orderpriority_OPE ASC", {db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
      {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(1), new sub_node(new int_literal_node(1), new tuple_pos_node(2))), false))), },
      new local_decrypt_op(
        {0, 1, 2},
        new remote_sql_op(new param_generator_id4, "select " NATION_ENC_NAME ".n_name_DET, group_serializer(" LINEITEM_ENC_NAME ".l_extendedprice_DET), group_serializer(" LINEITEM_ENC_NAME ".l_discount_DET) from " CUSTOMER_ENC_NAME ", " ORDERS_ENC_NAME ", " LINEITEM_ENC_NAME ", " SUPPLIER_ENC_NAME ", " NATION_ENC_NAME ", " REGION_ENC_NAME " where (((((((((" CUSTOMER_ENC_NAME ".c_custkey_DET) = (" ORDERS_ENC_NAME ".o_custkey_DET)) and ((" LINEITEM_ENC_NAME ".l_orderkey_DET) = (" ORDERS_ENC_NAME ".o_orderkey_DET))) and ((" LINEITEM_ENC_NAME ".l_suppkey_DET) = (" SUPPLIER_ENC_NAME ".s_suppkey_DET))) and ((" CUSTOMER_ENC_NAME ".c_nationkey_DET) = (" SUPPLIER_ENC_NAME ".s_nationkey_DET))) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET))) and ((" NATION_ENC_NAME ".n_regionkey_DET) = (" REGION_ENC_NAME ".r_regionkey_DET))) and ((" REGION_ENC_NAME ".r_name_DET) = (:0))) and ((" ORDERS_ENC_NAME ".o_orderdate_OPE) >= (:1))) and ((" ORDERS_ENC_NAME ".o_orderdate_OPE) < (:2)) group by " NATION_ENC_NAME ".n_name_DET", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
    {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(0), new tuple_pos_node(1)), false))), },
    new local_decrypt_op(
      {0, 1},
      new remote_sql_op(new param_generator_id5, "select group_serializer(" LINEITEM_ENC_NAME ".l_extendedprice_DET), group_serializer(" LINEITEM_ENC_NAME ".l_discount_DET) from " LINEITEM_ENC_NAME " where (((((" LINEITEM_ENC_NAME ".l_shipdate_OPE) >= (:0)) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) < (:1))) and ((" LINEITEM_ENC_NAME ".l_discount_OPE) >= (:2))) and ((" LINEITEM_ENC_NAME ".l_discount_OPE) <= (:3))) and ((" LINEITEM_ENC_NAME ".l_quantity_OPE) < (:4))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
        new local_decrypt_op(
          {0, 1},
          new local_transform_op(
            {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new extract_node(new tuple_pos_node(2), extract_node::TYPE_YEAR))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new tuple_pos_node(3), new sub_node(new int_literal_node(1), new tuple_pos_node(4))))), },
            new local_decrypt_op(
              {2, 3, 4},
              new remote_sql_op(new param_generator_id6, "select n1.n_name_DET as supp_nation, n2.n_name_DET as cust_nation, " LINEITEM_ENC_NAME ".l_shipdate_DET, " LINEITEM_ENC_NAME ".l_extendedprice_DET, " LINEITEM_ENC_NAME ".l_discount_DET from " SUPPLIER_ENC_NAME ", " LINEITEM_ENC_NAME ", " ORDERS_ENC_NAME ", " CUSTOMER_ENC_NAME ", " NATION_ENC_NAME " n1, " NATION_ENC_NAME " n2 where ((((((((" SUPPLIER_ENC_NAME ".s_suppkey_DET) = (" LINEITEM_ENC_NAME ".l_suppkey_DET)) and ((" ORDERS_ENC_NAME ".o_orderkey_DET) = (" LINEITEM_ENC_NAME ".l_orderkey_DET))) and ((" CUSTOMER_ENC_NAME ".c_custkey_DET) = (" ORDERS_ENC_NAME ".o_custkey_DET))) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (n1.n_nationkey_DET))) and ((" CUSTOMER_ENC_NAME ".c_nationkey_DET) = (n2.n_nationkey_DET))) and ((((n1.n_name_DET) = (:0)) and ((n2.n_name_DET) = (:1))) or (((n1.n_name_DET) = (:2)) and ((n2.n_name_DET) = (:3))))) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) >= (:4))) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) <= (:5))", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
          {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new extract_node(new tuple_pos_node(0), extract_node::TYPE_YEAR))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new tuple_pos_node(1), new sub_node(new int_literal_node(1), new tuple_pos_node(2))))), local_transform_op::trfm_desc(3), },
          new local_decrypt_op(
            {0, 1, 2, 3},
            new remote_sql_op(new param_generator_id7, "select " ORDERS_ENC_NAME ".o_orderdate_DET, " LINEITEM_ENC_NAME ".l_extendedprice_DET, " LINEITEM_ENC_NAME ".l_discount_DET, n2.n_name_DET as nation from " PART_ENC_NAME ", " SUPPLIER_ENC_NAME ", " LINEITEM_ENC_NAME ", " ORDERS_ENC_NAME ", " CUSTOMER_ENC_NAME ", " NATION_ENC_NAME " n1, " NATION_ENC_NAME " n2, " REGION_ENC_NAME " where (((((((((((" PART_ENC_NAME ".p_partkey_DET) = (" LINEITEM_ENC_NAME ".l_partkey_DET)) and ((" SUPPLIER_ENC_NAME ".s_suppkey_DET) = (" LINEITEM_ENC_NAME ".l_suppkey_DET))) and ((" LINEITEM_ENC_NAME ".l_orderkey_DET) = (" ORDERS_ENC_NAME ".o_orderkey_DET))) and ((" ORDERS_ENC_NAME ".o_custkey_DET) = (" CUSTOMER_ENC_NAME ".c_custkey_DET))) and ((" CUSTOMER_ENC_NAME ".c_nationkey_DET) = (n1.n_nationkey_DET))) and ((n1.n_regionkey_DET) = (" REGION_ENC_NAME ".r_regionkey_DET))) and ((" REGION_ENC_NAME ".r_name_DET) = (:0))) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (n2.n_nationkey_DET))) and ((" ORDERS_ENC_NAME ".o_orderdate_OPE) >= (:1))) and ((" ORDERS_ENC_NAME ".o_orderdate_OPE) <= (:2))) and ((" PART_ENC_NAME ".p_type_DET) = (:3))", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
        new local_decrypt_op(
          {0},
          new local_transform_op(
            {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new extract_node(new tuple_pos_node(1), extract_node::TYPE_YEAR))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sub_node(new mult_node(new tuple_pos_node(2), new sub_node(new int_literal_node(1), new tuple_pos_node(3))), new mult_node(new tuple_pos_node(4), new tuple_pos_node(5))))), },
            new local_decrypt_op(
              {1, 2, 3, 4, 5},
              new remote_sql_op(new param_generator_id8, "select " NATION_ENC_NAME ".n_name_DET as nation, " ORDERS_ENC_NAME ".o_orderdate_DET, " LINEITEM_ENC_NAME ".l_extendedprice_DET, " LINEITEM_ENC_NAME ".l_discount_DET, " PARTSUPP_ENC_NAME ".ps_supplycost_DET, " LINEITEM_ENC_NAME ".l_quantity_DET from " PART_ENC_NAME ", " SUPPLIER_ENC_NAME ", " LINEITEM_ENC_NAME ", " PARTSUPP_ENC_NAME ", " ORDERS_ENC_NAME ", " NATION_ENC_NAME " where (((((((" SUPPLIER_ENC_NAME ".s_suppkey_DET) = (" LINEITEM_ENC_NAME ".l_suppkey_DET)) and ((" PARTSUPP_ENC_NAME ".ps_suppkey_DET) = (" LINEITEM_ENC_NAME ".l_suppkey_DET))) and ((" PARTSUPP_ENC_NAME ".ps_partkey_DET) = (" LINEITEM_ENC_NAME ".l_partkey_DET))) and ((" PART_ENC_NAME ".p_partkey_DET) = (" LINEITEM_ENC_NAME ".l_partkey_DET))) and ((" ORDERS_ENC_NAME ".o_orderkey_DET) = (" LINEITEM_ENC_NAME ".l_orderkey_DET))) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET))) and (searchSWP(:0, :1, " PART_ENC_NAME ".p_name_SWP))", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
        {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(2), new sub_node(new int_literal_node(1), new tuple_pos_node(3))), false))), local_transform_op::trfm_desc(4), local_transform_op::trfm_desc(5), local_transform_op::trfm_desc(6), local_transform_op::trfm_desc(7), local_transform_op::trfm_desc(8), },
        new local_decrypt_op(
          {0, 1, 2, 3, 4, 5, 6, 7, 8},
          new remote_sql_op(new param_generator_id9, "select " CUSTOMER_ENC_NAME ".c_custkey_DET, " CUSTOMER_ENC_NAME ".c_name_DET, group_serializer(" LINEITEM_ENC_NAME ".l_extendedprice_DET), group_serializer(" LINEITEM_ENC_NAME ".l_discount_DET), " CUSTOMER_ENC_NAME ".c_acctbal_DET, " NATION_ENC_NAME ".n_name_DET, " CUSTOMER_ENC_NAME ".c_address_DET, " CUSTOMER_ENC_NAME ".c_phone_DET, " CUSTOMER_ENC_NAME ".c_comment_DET from " CUSTOMER_ENC_NAME ", " ORDERS_ENC_NAME ", " LINEITEM_ENC_NAME ", " NATION_ENC_NAME " where ((((((" CUSTOMER_ENC_NAME ".c_custkey_DET) = (" ORDERS_ENC_NAME ".o_custkey_DET)) and ((" LINEITEM_ENC_NAME ".l_orderkey_DET) = (" ORDERS_ENC_NAME ".o_orderkey_DET))) and ((" ORDERS_ENC_NAME ".o_orderdate_OPE) >= (:0))) and ((" ORDERS_ENC_NAME ".o_orderdate_OPE) < (:1))) and ((" LINEITEM_ENC_NAME ".l_returnflag_DET) = (:2))) and ((" CUSTOMER_ENC_NAME ".c_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET)) group by " CUSTOMER_ENC_NAME ".c_custkey_DET, " CUSTOMER_ENC_NAME ".c_name_DET, " CUSTOMER_ENC_NAME ".c_acctbal_DET, " CUSTOMER_ENC_NAME ".c_phone_DET, " NATION_ENC_NAME ".n_name_DET, " CUSTOMER_ENC_NAME ".c_address_DET, " CUSTOMER_ENC_NAME ".c_comment_DET", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 40, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_STRING, 117, oDET, SECLEVEL::DET, 7, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
            new remote_sql_op(new param_generator_id10, "select group_serializer(" PARTSUPP_ENC_NAME ".ps_supplycost_DET), array_to_string(array_agg(" PARTSUPP_ENC_NAME ".ps_availqty_DET), ','), " PARTSUPP_ENC_NAME ".ps_partkey_DET from " PARTSUPP_ENC_NAME ", " SUPPLIER_ENC_NAME ", " NATION_ENC_NAME " where (((" PARTSUPP_ENC_NAME ".ps_suppkey_DET) = (" SUPPLIER_ENC_NAME ".s_suppkey_DET)) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET))) and ((" NATION_ENC_NAME ".n_name_DET) = (:0)) group by " PARTSUPP_ENC_NAME ".ps_partkey_DET", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          , {
            new local_transform_op(
              {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new sum_node(new mult_node(new tuple_pos_node(0), new tuple_pos_node(1)), false), new double_literal_node(0.000100)))), },
              new local_decrypt_op(
                {0, 1},
                new remote_sql_op(new param_generator_id11, "select group_serializer(" PARTSUPP_ENC_NAME ".ps_supplycost_DET), array_to_string(array_agg(" PARTSUPP_ENC_NAME ".ps_availqty_DET), ',') from " PARTSUPP_ENC_NAME ", " SUPPLIER_ENC_NAME ", " NATION_ENC_NAME " where (((" PARTSUPP_ENC_NAME ".ps_suppkey_DET) = (" SUPPLIER_ENC_NAME ".s_suppkey_DET)) and ((" SUPPLIER_ENC_NAME ".s_nationkey_DET) = (" NATION_ENC_NAME ".n_nationkey_DET))) and ((" NATION_ENC_NAME ".n_name_DET) = (:0))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
  physical_operator* op = new local_decrypt_op(
    {0},
    new remote_sql_op(new param_generator_id12, "select agg_ident(" LINEITEM_ENC_NAME ".l_shipmode_DET), sum(case when ((" ORDERS_ENC_NAME ".o_orderpriority_DET) = (:0)) or ((" ORDERS_ENC_NAME ".o_orderpriority_DET) = (:1)) then 1 else 0 end) as high_line_count, sum(case when ((" ORDERS_ENC_NAME ".o_orderpriority_DET) <> (:2)) and ((" ORDERS_ENC_NAME ".o_orderpriority_DET) <> (:3)) then 1 else 0 end) as low_line_count from " ORDERS_ENC_NAME ", " LINEITEM_ENC_NAME " where ((((((" ORDERS_ENC_NAME ".o_orderkey_DET) = (" LINEITEM_ENC_NAME ".l_orderkey_DET)) and (" LINEITEM_ENC_NAME ".l_shipmode_DET in ( :4, :5 ))) and ((" LINEITEM_ENC_NAME ".l_commitdate_OPE) < (" LINEITEM_ENC_NAME ".l_receiptdate_OPE))) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) < (" LINEITEM_ENC_NAME ".l_commitdate_OPE))) and ((" LINEITEM_ENC_NAME ".l_receiptdate_OPE) >= (:6))) and ((" LINEITEM_ENC_NAME ".l_receiptdate_OPE) < (:7)) group by " LINEITEM_ENC_NAME ".l_shipmode_OPE order by " LINEITEM_ENC_NAME ".l_shipmode_OPE ASC", {db_column_desc(db_elem::TYPE_STRING, 10, oDET, SECLEVEL::DET, 14, false), db_column_desc(db_elem::TYPE_INT, 8, oNONE, SECLEVEL::PLAIN, 0, false), db_column_desc(db_elem::TYPE_INT, 8, oNONE, SECLEVEL::PLAIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
    {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new mult_node(new double_literal_node(100.000000), new sum_node(new case_when_node({new case_expr_case_node(new like_node(new tuple_pos_node(0), new string_literal_node("PROMO%"), false), new mult_node(new tuple_pos_node(1), new sub_node(new int_literal_node(1), new tuple_pos_node(2))))}, new int_literal_node(0)), false)), new sum_node(new mult_node(new tuple_pos_node(1), new sub_node(new int_literal_node(1), new tuple_pos_node(2))), false)))), },
    new local_decrypt_op(
      {0, 1, 2},
      new remote_sql_op(new param_generator_id13, "select array_to_string(array_agg(encode(" PART_ENC_NAME ".p_type_DET, 'hex')), ','), group_serializer(" LINEITEM_ENC_NAME ".l_extendedprice_DET), group_serializer(" LINEITEM_ENC_NAME ".l_discount_DET) from " LINEITEM_ENC_NAME ", " PART_ENC_NAME " where (((" LINEITEM_ENC_NAME ".l_partkey_DET) = (" PART_ENC_NAME ".p_partkey_DET)) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) >= (:0))) and ((" LINEITEM_ENC_NAME ".l_shipdate_OPE) < (:1))", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
    {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new double_literal_node(0.200000), new div_node(new function_call_node("hom_get_pos", {new tuple_pos_node(1), new int_literal_node(0)}), new tuple_pos_node(2))))), },
    new local_decrypt_op(
      {0, 1},
      new remote_sql_op(new param_generator_id14, "select " LINEITEM_ENC_NAME ".l_partkey_DET, hom_agg(:0, " LINEITEM_ENC_NAME ".l_packed_AGG), count(*) from " LINEITEM_ENC_NAME ", " PART_ENC_NAME " where (((" PART_ENC_NAME ".p_partkey_DET) = (" LINEITEM_ENC_NAME ".l_partkey_DET)) and ((" PART_ENC_NAME ".p_brand_DET) = (:1))) and ((" PART_ENC_NAME ".p_container_DET) = (:2)) group by " LINEITEM_ENC_NAME ".l_partkey_DET", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 1, false), db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false), db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
  physical_operator* op = new local_transform_op(
    {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(2), local_transform_op::trfm_desc(3), local_transform_op::trfm_desc(4), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new function_call_node("hom_get_pos", {new tuple_pos_node(5), new int_literal_node(0)}))), },
    new local_decrypt_op(
      {0, 1, 2, 3, 4, 5},
      new remote_sql_op(new param_generator_id16, "select " CUSTOMER_ENC_NAME ".c_name_DET, " CUSTOMER_ENC_NAME ".c_custkey_DET, " ORDERS_ENC_NAME ".o_orderkey_DET, agg_ident(" ORDERS_ENC_NAME ".o_orderdate_DET), agg_ident(" ORDERS_ENC_NAME ".o_totalprice_DET), hom_agg(:0, " LINEITEM_ENC_NAME ".l_packed_AGG) from " CUSTOMER_ENC_NAME ", " ORDERS_ENC_NAME ", " LINEITEM_ENC_NAME " where ((" ORDERS_ENC_NAME ".o_orderkey_DET in ( :_subselect$2 )) and ((" CUSTOMER_ENC_NAME ".c_custkey_DET) = (" ORDERS_ENC_NAME ".o_custkey_DET))) and ((" ORDERS_ENC_NAME ".o_orderkey_DET) = (" LINEITEM_ENC_NAME ".l_orderkey_DET)) group by " CUSTOMER_ENC_NAME ".c_name_DET, " CUSTOMER_ENC_NAME ".c_custkey_DET, " ORDERS_ENC_NAME ".o_orderkey_DET, " ORDERS_ENC_NAME ".o_orderdate_OPE, " ORDERS_ENC_NAME ".o_totalprice_OPE order by " ORDERS_ENC_NAME ".o_totalprice_OPE DESC, " ORDERS_ENC_NAME ".o_orderdate_OPE ASC limit 100", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, false), db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({std::pair<std::string, physical_operator*>("_subselect$2", new local_transform_op(
        {local_transform_op::trfm_desc(1), },
        new local_group_filter(
          new gt_node(new function_call_node("hom_get_pos", {new tuple_pos_node(0), new int_literal_node(0)}), new int_literal_node(315)),
          new local_decrypt_op(
            {0},
            new remote_sql_op(new param_generator_id15, "select hom_agg(:0, " LINEITEM_ENC_NAME ".l_packed_AGG), " LINEITEM_ENC_NAME ".l_orderkey_DET from " LINEITEM_ENC_NAME " group by " LINEITEM_ENC_NAME ".l_orderkey_DET", {db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          , {
          }
        )
      )
      )})))
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
    {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(0), new sub_node(new int_literal_node(1), new tuple_pos_node(1))), false))), },
    new local_decrypt_op(
      {0, 1},
      new remote_sql_op(new param_generator_id17, "select group_serializer(" LINEITEM_ENC_NAME ".l_extendedprice_DET), group_serializer(" LINEITEM_ENC_NAME ".l_discount_DET) from " LINEITEM_ENC_NAME ", " PART_ENC_NAME " where ((((((((((" PART_ENC_NAME ".p_partkey_DET) = (" LINEITEM_ENC_NAME ".l_partkey_DET)) and ((" PART_ENC_NAME ".p_brand_DET) = (:0))) and (" PART_ENC_NAME ".p_container_DET in ( :1, :2, :3, :4 ))) and ((" LINEITEM_ENC_NAME ".l_quantity_OPE) >= (:5))) and ((" LINEITEM_ENC_NAME ".l_quantity_OPE) <= (:6))) and (((" PART_ENC_NAME ".p_size_OPE) >= (:7)) and ((" PART_ENC_NAME ".p_size_OPE) <= (:8)))) and (" LINEITEM_ENC_NAME ".l_shipmode_DET in ( :9, :10 ))) and ((" LINEITEM_ENC_NAME ".l_shipinstruct_DET) = (:11))) or (((((((((" PART_ENC_NAME ".p_partkey_DET) = (" LINEITEM_ENC_NAME ".l_partkey_DET)) and ((" PART_ENC_NAME ".p_brand_DET) = (:12))) and (" PART_ENC_NAME ".p_container_DET in ( :13, :14, :15, :16 ))) and ((" LINEITEM_ENC_NAME ".l_quantity_OPE) >= (:17))) and ((" LINEITEM_ENC_NAME ".l_quantity_OPE) <= (:18))) and (((" PART_ENC_NAME ".p_size_OPE) >= (:19)) and ((" PART_ENC_NAME ".p_size_OPE) <= (:20)))) and (" LINEITEM_ENC_NAME ".l_shipmode_DET in ( :21, :22 ))) and ((" LINEITEM_ENC_NAME ".l_shipinstruct_DET) = (:23)))) or (((((((((" PART_ENC_NAME ".p_partkey_DET) = (" LINEITEM_ENC_NAME ".l_partkey_DET)) and ((" PART_ENC_NAME ".p_brand_DET) = (:24))) and (" PART_ENC_NAME ".p_container_DET in ( :25, :26, :27, :28 ))) and ((" LINEITEM_ENC_NAME ".l_quantity_OPE) >= (:29))) and ((" LINEITEM_ENC_NAME ".l_quantity_OPE) <= (:30))) and (((" PART_ENC_NAME ".p_size_OPE) >= (:31)) and ((" PART_ENC_NAME ".p_size_OPE) <= (:32)))) and (" LINEITEM_ENC_NAME ".l_shipmode_DET in ( :33, :34 ))) and ((" LINEITEM_ENC_NAME ".l_shipinstruct_DET) = (:35)))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
  // our query optimizer couldn't generate a useful plan for this one,
  // so we'll just hand-code it instead of modifying the optimizer to
  physical_operator* op = new local_transform_op(
    {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false), new count_star_node)), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new tuple_pos_node(1), false)))},
    new local_order_by(
      {std::make_pair(0, false)},
      new local_group_by(
        {0},
        new local_transform_op({local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_STRING, 15, oNONE, SECLEVEL::PLAIN, 4, false), new substring_node(new tuple_pos_node(0), 1, 2))), local_transform_op::trfm_desc(1), },
          new local_decrypt_op(
            {1},
            new local_filter_op(
              new in_node(new substring_node(new tuple_pos_node(0), 1, 2), {new string_literal_node("41"), new string_literal_node("26"), new string_literal_node("36"), new string_literal_node("27"), new string_literal_node("38"), new string_literal_node("37"), new string_literal_node("22")}),
              new local_decrypt_op(
                {0},
                new remote_sql_op(new param_generator_id19, "select " CUSTOMER_ENC_NAME ".c_phone_DET, " CUSTOMER_ENC_NAME ".c_acctbal_DET as abal from " CUSTOMER_ENC_NAME " where ((" CUSTOMER_ENC_NAME ".c_acctbal_OPE) > (:_subselect$1)) and (not ( exists ( (select 1 from " ORDERS_ENC_NAME " where (" ORDERS_ENC_NAME ".o_custkey_DET) = (" CUSTOMER_ENC_NAME ".c_custkey_DET)) ) ))", {db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({std::pair<std::string, physical_operator*>("_subselect$1", new local_encrypt_op({std::pair<size_t, db_column_desc>(0, db_column_desc(db_elem::TYPE_DECIMAL_15_2, 8, oOPE, SECLEVEL::OPE, 5, false))}, new local_transform_op(
                  {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new avg_node(new tuple_pos_node(1), false))), },
                  new local_decrypt_op(
                    {1},
                    new local_filter_op(
                      new in_node(new substring_node(new tuple_pos_node(0), 1, 2), {new string_literal_node("41"), new string_literal_node("26"), new string_literal_node("36"), new string_literal_node("27"), new string_literal_node("38"), new string_literal_node("37"), new string_literal_node("22")}),
                      new local_decrypt_op(
                        {0},
                        new remote_sql_op(new param_generator_id18, "select array_to_string(array_agg(encode(" CUSTOMER_ENC_NAME ".c_phone_DET, 'hex')), ','), group_serializer(" CUSTOMER_ENC_NAME ".c_acctbal_DET) from " CUSTOMER_ENC_NAME " where (" CUSTOMER_ENC_NAME ".c_acctbal_OPE) > (:0)", {db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
                      ,{
                      }
                    )
                  )
                )
                ))})))
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
int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "[Usage]: " << argv[0] << " [query num]" << std::endl;
    return 1;
  }
  int q = atoi(argv[1]);
  CryptoManager cm("12345");
  crypto_manager_stub cm_stub(&cm, CRYPTO_USE_OLD_OPE);
  PGConnect pg(DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PORT, true);
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
    default: assert(false);
  }
  return 0;
}
