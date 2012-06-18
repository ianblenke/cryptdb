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
  {
    static const size_t RowColPackPlainSize = 1024;
    static const size_t RowColPackCipherSize = RowColPackPlainSize * 2;
    auto pp = ctx.pp_cache->get_paillier_priv(RowColPackCipherSize / 2, RowColPackCipherSize / 8);
    auto pk = pp.pubkey();
    m[1] = (RowColPackCipherSize == 2048) ? db_elem(ctx.crypto->cm->getPKInfo()) : db_elem(StringFromZZ(pk[0] * pk[0]));
  }
  {
    static const size_t RowColPackPlainSize = 1024;
    static const size_t RowColPackCipherSize = RowColPackPlainSize * 2;
    auto pp = ctx.pp_cache->get_paillier_priv(RowColPackCipherSize / 2, RowColPackCipherSize / 8);
    auto pk = pp.pubkey();
    m[2] = (RowColPackCipherSize == 2048) ? db_elem(ctx.crypto->cm->getPKInfo()) : db_elem(StringFromZZ(pk[0] * pk[0]));
  }
  m[3] = db_elem((int64_t)encrypt_date_ope(ctx.crypto, 1023009 /*1998-1-1*/, 10, true));
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
static void query_0(exec_context& ctx) {
  physical_operator* op = new local_transform_op(
    {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new function_call_node("hom_get_pos", {new tuple_pos_node(2), new int_literal_node(0)}))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new function_call_node("hom_get_pos", {new tuple_pos_node(3), new int_literal_node(0)}))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new tuple_pos_node(4), new sub_node(new int_literal_node(1), new tuple_pos_node(5))), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sum_node(new mult_node(new mult_node(new tuple_pos_node(4), new sub_node(new int_literal_node(1), new tuple_pos_node(5))), new add_node(new int_literal_node(1), new tuple_pos_node(6))), false))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new function_call_node("hom_get_pos", {new tuple_pos_node(2), new int_literal_node(0)}), new tuple_pos_node(7)))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new function_call_node("hom_get_pos", {new tuple_pos_node(3), new int_literal_node(0)}), new tuple_pos_node(7)))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new div_node(new function_call_node("hom_get_pos", {new tuple_pos_node(8), new int_literal_node(0)}), new tuple_pos_node(7)))), local_transform_op::trfm_desc(7), },
    new local_decrypt_op(
      {0, 1, 2, 3, 4, 5, 6, 8},
      new remote_sql_op(new param_generator_id0, "select agg_ident(lineitem_enc.l_returnflag_DET), agg_ident(lineitem_enc.l_linestatus_DET), hom_agg(:0, lineitem_enc.l_quantity_AGG), hom_agg(:1, lineitem_enc.l_extendedprice_AGG), group_serializer(lineitem_enc.l_extendedprice_DET), group_serializer(lineitem_enc.l_discount_DET), group_serializer(lineitem_enc.l_tax_DET), count(*), hom_agg(:2, lineitem_enc.l_discount_AGG) from lineitem_enc where (lineitem_enc.l_shipdate_OPE) <= (:3) group by lineitem_enc.l_returnflag_OPE, lineitem_enc.l_linestatus_OPE order by lineitem_enc.l_returnflag_OPE ASC, lineitem_enc.l_linestatus_OPE ASC", {db_column_desc(db_elem::TYPE_CHAR, 1, oDET, SECLEVEL::DET, 8, false), db_column_desc(db_elem::TYPE_CHAR, 1, oDET, SECLEVEL::DET, 9, false), db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false), db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 7, true), db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false), db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
    new remote_sql_op(new param_generator_id1, "select supplier_enc.s_acctbal_DET, supplier_enc.s_name_DET, nation_enc.n_name_DET, part_enc.p_partkey_DET, part_enc.p_mfgr_DET, supplier_enc.s_address_DET, supplier_enc.s_phone_DET, supplier_enc.s_comment_DET from part_enc, supplier_enc, partsupp_enc, nation_enc, region_enc where ((((((((part_enc.p_partkey_DET) = (partsupp_enc.ps_partkey_DET)) and ((supplier_enc.s_suppkey_DET) = (partsupp_enc.ps_suppkey_DET))) and ((part_enc.p_size_DET) = (:0))) and (searchSWP(:1, :2, part_enc.p_type_SWP))) and ((supplier_enc.s_nationkey_DET) = (nation_enc.n_nationkey_DET))) and ((nation_enc.n_regionkey_DET) = (region_enc.r_regionkey_DET))) and ((region_enc.r_name_DET) = (:3))) and ((partsupp_enc.ps_supplycost_OPE) = ((select min(partsupp_enc.ps_supplycost_OPE) from partsupp_enc, supplier_enc, nation_enc, region_enc where (((((part_enc.p_partkey_DET) = (partsupp_enc.ps_partkey_DET)) and ((supplier_enc.s_suppkey_DET) = (partsupp_enc.ps_suppkey_DET))) and ((supplier_enc.s_nationkey_DET) = (nation_enc.n_nationkey_DET))) and ((nation_enc.n_regionkey_DET) = (region_enc.r_regionkey_DET))) and ((region_enc.r_name_DET) = (:4))))) order by supplier_enc.s_acctbal_OPE DESC, nation_enc.n_name_OPE ASC, supplier_enc.s_name_OPE ASC, part_enc.p_partkey_OPE ASC limit 100", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 40, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_STRING, 101, oDET, SECLEVEL::DET, 6, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
          new remote_sql_op(new param_generator_id2, "select lineitem_enc.l_orderkey_DET, group_serializer(lineitem_enc.l_extendedprice_DET), group_serializer(lineitem_enc.l_discount_DET), orders_enc.o_orderdate_DET, orders_enc.o_shippriority_DET from customer_enc, orders_enc, lineitem_enc where (((((customer_enc.c_mktsegment_DET) = (:0)) and ((customer_enc.c_custkey_DET) = (orders_enc.o_custkey_DET))) and ((lineitem_enc.l_orderkey_DET) = (orders_enc.o_orderkey_DET))) and ((orders_enc.o_orderdate_OPE) < (:1))) and ((lineitem_enc.l_shipdate_OPE) > (:2)) group by lineitem_enc.l_orderkey_DET, orders_enc.o_orderdate_DET, orders_enc.o_shippriority_DET", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 7, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
    new remote_sql_op(new param_generator_id3, "select agg_ident(orders_enc.o_orderpriority_DET), count(*) as order_count from orders_enc where (((orders_enc.o_orderdate_OPE) >= (:0)) and ((orders_enc.o_orderdate_OPE) < (:1))) and (exists ( (select 1 from lineitem_enc where ((lineitem_enc.l_orderkey_DET) = (orders_enc.o_orderkey_DET)) and ((lineitem_enc.l_commitdate_OPE) < (lineitem_enc.l_receiptdate_OPE))) )) group by orders_enc.o_orderpriority_OPE order by orders_enc.o_orderpriority_OPE ASC", {db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
        new remote_sql_op(new param_generator_id4, "select nation_enc.n_name_DET, group_serializer(lineitem_enc.l_extendedprice_DET), group_serializer(lineitem_enc.l_discount_DET) from customer_enc, orders_enc, lineitem_enc, supplier_enc, nation_enc, region_enc where (((((((((customer_enc.c_custkey_DET) = (orders_enc.o_custkey_DET)) and ((lineitem_enc.l_orderkey_DET) = (orders_enc.o_orderkey_DET))) and ((lineitem_enc.l_suppkey_DET) = (supplier_enc.s_suppkey_DET))) and ((customer_enc.c_nationkey_DET) = (supplier_enc.s_nationkey_DET))) and ((supplier_enc.s_nationkey_DET) = (nation_enc.n_nationkey_DET))) and ((nation_enc.n_regionkey_DET) = (region_enc.r_regionkey_DET))) and ((region_enc.r_name_DET) = (:0))) and ((orders_enc.o_orderdate_OPE) >= (:1))) and ((orders_enc.o_orderdate_OPE) < (:2)) group by nation_enc.n_name_DET", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
      new remote_sql_op(new param_generator_id5, "select group_serializer(lineitem_enc.l_extendedprice_DET), group_serializer(lineitem_enc.l_discount_DET) from lineitem_enc where (((((lineitem_enc.l_shipdate_OPE) >= (:0)) and ((lineitem_enc.l_shipdate_OPE) < (:1))) and ((lineitem_enc.l_discount_OPE) >= (:2))) and ((lineitem_enc.l_discount_OPE) <= (:3))) and ((lineitem_enc.l_quantity_OPE) < (:4))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
          {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(1), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new extract_node(new tuple_pos_node(2), extract_node::TYPE_YEAR))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new tuple_pos_node(3), new sub_node(new int_literal_node(1), new tuple_pos_node(4))))), },
          new local_decrypt_op(
            {2, 3, 4},
            new remote_sql_op(new param_generator_id6, "select n1.n_name_DET as supp_nation, n2.n_name_DET as cust_nation, lineitem_enc.l_shipdate_DET, lineitem_enc.l_extendedprice_DET, lineitem_enc.l_discount_DET from supplier_enc, lineitem_enc, orders_enc, customer_enc, nation_enc n1, nation_enc n2 where ((((((((supplier_enc.s_suppkey_DET) = (lineitem_enc.l_suppkey_DET)) and ((orders_enc.o_orderkey_DET) = (lineitem_enc.l_orderkey_DET))) and ((customer_enc.c_custkey_DET) = (orders_enc.o_custkey_DET))) and ((supplier_enc.s_nationkey_DET) = (n1.n_nationkey_DET))) and ((customer_enc.c_nationkey_DET) = (n2.n_nationkey_DET))) and ((((n1.n_name_DET) = (:0)) and ((n2.n_name_DET) = (:1))) or (((n1.n_name_DET) = (:2)) and ((n2.n_name_DET) = (:3))))) and ((lineitem_enc.l_shipdate_OPE) >= (:4))) and ((lineitem_enc.l_shipdate_OPE) <= (:5))", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 10, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
            new remote_sql_op(new param_generator_id7, "select orders_enc.o_orderdate_DET, lineitem_enc.l_extendedprice_DET, lineitem_enc.l_discount_DET, n2.n_name_DET as nation from part_enc, supplier_enc, lineitem_enc, orders_enc, customer_enc, nation_enc n1, nation_enc n2, region_enc where (((((((((((part_enc.p_partkey_DET) = (lineitem_enc.l_partkey_DET)) and ((supplier_enc.s_suppkey_DET) = (lineitem_enc.l_suppkey_DET))) and ((lineitem_enc.l_orderkey_DET) = (orders_enc.o_orderkey_DET))) and ((orders_enc.o_custkey_DET) = (customer_enc.c_custkey_DET))) and ((customer_enc.c_nationkey_DET) = (n1.n_nationkey_DET))) and ((n1.n_regionkey_DET) = (region_enc.r_regionkey_DET))) and ((region_enc.r_name_DET) = (:0))) and ((supplier_enc.s_nationkey_DET) = (n2.n_nationkey_DET))) and ((orders_enc.o_orderdate_OPE) >= (:1))) and ((orders_enc.o_orderdate_OPE) <= (:2))) and ((part_enc.p_type_DET) = (:3))", {db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
          {local_transform_op::trfm_desc(0), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_INT, 2, oNONE, SECLEVEL::PLAIN, 0, false), new extract_node(new tuple_pos_node(1), extract_node::TYPE_YEAR))), local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oNONE, SECLEVEL::PLAIN, 0, false), new sub_node(new mult_node(new tuple_pos_node(2), new sub_node(new int_literal_node(1), new tuple_pos_node(3))), new mult_node(new tuple_pos_node(4), new tuple_pos_node(5))))), },
          new local_decrypt_op(
            {1, 2, 3, 4, 5},
            new remote_sql_op(new param_generator_id8, "select nation_enc.n_name_OPE as nation, orders_enc.o_orderdate_DET, lineitem_enc.l_extendedprice_DET, lineitem_enc.l_discount_DET, partsupp_enc.ps_supplycost_DET, lineitem_enc.l_quantity_DET from part_enc, supplier_enc, lineitem_enc, partsupp_enc, orders_enc, nation_enc where (((((((supplier_enc.s_suppkey_DET) = (lineitem_enc.l_suppkey_DET)) and ((partsupp_enc.ps_suppkey_DET) = (lineitem_enc.l_suppkey_DET))) and ((partsupp_enc.ps_partkey_DET) = (lineitem_enc.l_partkey_DET))) and ((part_enc.p_partkey_DET) = (lineitem_enc.l_partkey_DET))) and ((orders_enc.o_orderkey_DET) = (lineitem_enc.l_orderkey_DET))) and ((supplier_enc.s_nationkey_DET) = (nation_enc.n_nationkey_DET))) and (searchSWP(:0, :1, part_enc.p_name_SWP))", {db_column_desc(db_elem::TYPE_STRING, 25, oOPE, SECLEVEL::OPE, 1, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 4, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
          new remote_sql_op(new param_generator_id9, "select customer_enc.c_custkey_DET, customer_enc.c_name_DET, group_serializer(lineitem_enc.l_extendedprice_DET), group_serializer(lineitem_enc.l_discount_DET), customer_enc.c_acctbal_DET, nation_enc.n_name_DET, customer_enc.c_address_DET, customer_enc.c_phone_DET, customer_enc.c_comment_DET from customer_enc, orders_enc, lineitem_enc, nation_enc where ((((((customer_enc.c_custkey_DET) = (orders_enc.o_custkey_DET)) and ((lineitem_enc.l_orderkey_DET) = (orders_enc.o_orderkey_DET))) and ((orders_enc.o_orderdate_OPE) >= (:0))) and ((orders_enc.o_orderdate_OPE) < (:1))) and ((lineitem_enc.l_returnflag_DET) = (:2))) and ((customer_enc.c_nationkey_DET) = (nation_enc.n_nationkey_DET)) group by customer_enc.c_custkey_DET, customer_enc.c_name_DET, customer_enc.c_acctbal_DET, customer_enc.c_phone_DET, nation_enc.n_name_DET, customer_enc.c_address_DET, customer_enc.c_comment_DET", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, false), db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 40, oDET, SECLEVEL::DET, 2, false), db_column_desc(db_elem::TYPE_STRING, 15, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_STRING, 117, oDET, SECLEVEL::DET, 7, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
            new remote_sql_op(new param_generator_id10, "select group_serializer(partsupp_enc.ps_supplycost_DET), array_to_string(array_agg(partsupp_enc.ps_availqty_DET), ','), partsupp_enc.ps_partkey_DET from partsupp_enc, supplier_enc, nation_enc where (((partsupp_enc.ps_suppkey_DET) = (supplier_enc.s_suppkey_DET)) and ((supplier_enc.s_nationkey_DET) = (nation_enc.n_nationkey_DET))) and ((nation_enc.n_name_DET) = (:0)) group by partsupp_enc.ps_partkey_DET", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
          , {
            new local_transform_op(
              {local_transform_op::trfm_desc(std::make_pair(db_column_desc(db_elem::TYPE_DOUBLE, 8, oNONE, SECLEVEL::PLAIN, 0, false), new mult_node(new sum_node(new mult_node(new tuple_pos_node(0), new tuple_pos_node(1)), false), new double_literal_node(0.000100)))), },
              new local_decrypt_op(
                {0, 1},
                new remote_sql_op(new param_generator_id11, "select group_serializer(partsupp_enc.ps_supplycost_DET), array_to_string(array_agg(partsupp_enc.ps_availqty_DET), ',') from partsupp_enc, supplier_enc, nation_enc where (((partsupp_enc.ps_suppkey_DET) = (supplier_enc.s_suppkey_DET)) and ((supplier_enc.s_nationkey_DET) = (nation_enc.n_nationkey_DET))) and ((nation_enc.n_name_DET) = (:0))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, true), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 2, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
    new remote_sql_op(new param_generator_id12, "select agg_ident(lineitem_enc.l_shipmode_DET), fast_sum(case when ((orders_enc.o_orderpriority_DET) = (:0)) or ((orders_enc.o_orderpriority_DET) = (:1)) then 1 else 0 end) as high_line_count, fast_sum(case when ((orders_enc.o_orderpriority_DET) <> (:2)) and ((orders_enc.o_orderpriority_DET) <> (:3)) then 1 else 0 end) as low_line_count from orders_enc, lineitem_enc where ((((((orders_enc.o_orderkey_DET) = (lineitem_enc.l_orderkey_DET)) and (lineitem_enc.l_shipmode_DET in ( :4, :5 ))) and ((lineitem_enc.l_commitdate_OPE) < (lineitem_enc.l_receiptdate_OPE))) and ((lineitem_enc.l_shipdate_OPE) < (lineitem_enc.l_commitdate_OPE))) and ((lineitem_enc.l_receiptdate_OPE) >= (:6))) and ((lineitem_enc.l_receiptdate_OPE) < (:7)) group by lineitem_enc.l_shipmode_OPE order by lineitem_enc.l_shipmode_OPE ASC", {db_column_desc(db_elem::TYPE_STRING, 10, oDET, SECLEVEL::DET, 14, false), db_column_desc(db_elem::TYPE_INT, 8, oNONE, SECLEVEL::PLAIN, 0, false), db_column_desc(db_elem::TYPE_INT, 8, oNONE, SECLEVEL::PLAIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
      new remote_sql_op(new param_generator_id13, "select array_to_string(array_agg(encode(part_enc.p_type_DET, 'hex')), ','), group_serializer(lineitem_enc.l_extendedprice_DET), group_serializer(lineitem_enc.l_discount_DET) from lineitem_enc, part_enc where (((lineitem_enc.l_partkey_DET) = (part_enc.p_partkey_DET)) and ((lineitem_enc.l_shipdate_OPE) >= (:0))) and ((lineitem_enc.l_shipdate_OPE) < (:1))", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 4, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
      new remote_sql_op(new param_generator_id14, "select lineitem_enc.l_partkey_DET, hom_agg(:0, lineitem_enc.l_quantity_AGG), count(*) from lineitem_enc, part_enc where (((part_enc.p_partkey_DET) = (lineitem_enc.l_partkey_DET)) and ((part_enc.p_brand_DET) = (:1))) and ((part_enc.p_container_DET) = (:2)) group by lineitem_enc.l_partkey_DET", {db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false), db_column_desc(db_elem::TYPE_INT, 4, oNONE, SECLEVEL::PLAIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
      new remote_sql_op(new param_generator_id16, "select customer_enc.c_name_DET, customer_enc.c_custkey_DET, orders_enc.o_orderkey_DET, agg_ident(orders_enc.o_orderdate_DET), agg_ident(orders_enc.o_totalprice_DET), hom_agg(:0, lineitem_enc.l_quantity_AGG) from customer_enc, orders_enc, lineitem_enc where ((orders_enc.o_orderkey_DET in ( :_subselect$0 )) and ((customer_enc.c_custkey_DET) = (orders_enc.o_custkey_DET))) and ((orders_enc.o_orderkey_DET) = (lineitem_enc.l_orderkey_DET)) group by customer_enc.c_name_DET, customer_enc.c_custkey_DET, orders_enc.o_orderkey_DET, orders_enc.o_orderdate_OPE, orders_enc.o_totalprice_OPE order by orders_enc.o_totalprice_OPE DESC, orders_enc.o_orderdate_OPE ASC limit 100", {db_column_desc(db_elem::TYPE_STRING, 25, oDET, SECLEVEL::DET, 1, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false), db_column_desc(db_elem::TYPE_DATE, 3, oDET, SECLEVEL::DET, 4, false), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 3, false), db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({std::pair<std::string, physical_operator*>("_subselect$0", new local_transform_op(
        {local_transform_op::trfm_desc(1), },
        new local_group_filter(
          new gt_node(new function_call_node("hom_get_pos", {new tuple_pos_node(0), new int_literal_node(0)}), new int_literal_node(315)),
          new local_decrypt_op(
            {0},
            new remote_sql_op(new param_generator_id15, "select hom_agg(:0, lineitem_enc.l_quantity_AGG), lineitem_enc.l_orderkey_DET from lineitem_enc group by lineitem_enc.l_orderkey_DET", {db_column_desc(db_elem::TYPE_STRING, 2147483647, oAGG_ORIGINAL, SECLEVEL::SEMANTIC_AGG, 0, false), db_column_desc(db_elem::TYPE_INT, 4, oDET, SECLEVEL::DETJOIN, 0, false)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
      new remote_sql_op(new param_generator_id17, "select group_serializer(lineitem_enc.l_extendedprice_DET), group_serializer(lineitem_enc.l_discount_DET) from lineitem_enc, part_enc where ((((((((((part_enc.p_partkey_DET) = (lineitem_enc.l_partkey_DET)) and ((part_enc.p_brand_DET) = (:0))) and (part_enc.p_container_DET in ( :1, :2, :3, :4 ))) and ((lineitem_enc.l_quantity_OPE) >= (:5))) and ((lineitem_enc.l_quantity_OPE) <= (:6))) and (((part_enc.p_size_OPE) >= (:7)) and ((part_enc.p_size_OPE) <= (:8)))) and (lineitem_enc.l_shipmode_DET in ( :9, :10 ))) and ((lineitem_enc.l_shipinstruct_DET) = (:11))) or (((((((((part_enc.p_partkey_DET) = (lineitem_enc.l_partkey_DET)) and ((part_enc.p_brand_DET) = (:12))) and (part_enc.p_container_DET in ( :13, :14, :15, :16 ))) and ((lineitem_enc.l_quantity_OPE) >= (:17))) and ((lineitem_enc.l_quantity_OPE) <= (:18))) and (((part_enc.p_size_OPE) >= (:19)) and ((part_enc.p_size_OPE) <= (:20)))) and (lineitem_enc.l_shipmode_DET in ( :21, :22 ))) and ((lineitem_enc.l_shipinstruct_DET) = (:23)))) or (((((((((part_enc.p_partkey_DET) = (lineitem_enc.l_partkey_DET)) and ((part_enc.p_brand_DET) = (:24))) and (part_enc.p_container_DET in ( :25, :26, :27, :28 ))) and ((lineitem_enc.l_quantity_OPE) >= (:29))) and ((lineitem_enc.l_quantity_OPE) <= (:30))) and (((part_enc.p_size_OPE) >= (:31)) and ((part_enc.p_size_OPE) <= (:32)))) and (lineitem_enc.l_shipmode_DET in ( :33, :34 ))) and ((lineitem_enc.l_shipinstruct_DET) = (:35)))", {db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 5, true), db_column_desc(db_elem::TYPE_DECIMAL_15_2, 15, oDET, SECLEVEL::DET, 6, true)}, {}, util::map_from_pair_vec<std::string, physical_operator*>({})))
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
  PGConnect pg(DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PORT);
  paillier_cache pp_cache;
  query_cache cache;
  exec_context ctx(&pg, &cm_stub, &pp_cache, &cache);
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
    default: assert(false);
  }
  return 0;
}
