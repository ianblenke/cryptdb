CREATE SEQUENCE lineitem_enc_cryptdb_opt_seq MINVALUE 0;
create table lineitem_enc_cryptdb_opt (

  l_orderkey_DET bigint  NOT NULL,

  l_partkey_DET bigint  NOT NULL,

  l_suppkey_DET bigint  NOT NULL,

  l_linenumber_DET bigint  NOT NULL,

  l_quantity_DET bigint  NOT NULL,
  l_quantity_OPE bytea NOT NULL,

  l_extendedprice_DET bigint  NOT NULL,

  l_discount_OPE bytea NOT NULL,

  l_tax_DET bigint  NOT NULL,

  l_returnflag_DET smallint  NOT NULL,

  l_linestatus_DET smallint  NOT NULL,

  l_shipdate_OPE bigint  NOT NULL,

  l_commitdate_OPE bigint  NOT NULL,

  l_receiptdate_OPE bigint  NOT NULL,

  l_shipinstruct_DET bytea NOT NULL,

  l_shipmode_DET bytea NOT NULL,

  l_comment_DET bytea NOT NULL,

  -- pre-computed fields

  l_shipdate_year_DET integer NOT NULL,
  l_disc_price_DET bigint  NOT NULL,

  row_id bigint NOT NULL DEFAULT nextval('lineitem_enc_cryptdb_opt_seq')

  -- PRIMARY KEY (l_orderkey_DET, l_linenumber_DET)

) ;

-- INSERT INTO lineitem_enc_cryptdb_opt 
-- SELECT
--   l_orderkey_DET,
--   l_partkey_DET,
--   l_suppkey_DET,
--   l_linenumber_DET,
--   l_quantity_DET,
--   l_quantity_OPE,
--   l_extendedprice_DET,
--   l_discount_OPE,
--   l_tax_DET,
--   l_returnflag_DET,
--   l_linestatus_DET,
--   l_shipdate_OPE,
--   l_commitdate_OPE,
--   l_receiptdate_OPE,
--   l_shipinstruct_DET,
--   l_shipmode_DET,
--   l_comment_DET,
--   l_shipdate_year_DET,
--   l_disc_price_DET,
--   row_id 
-- FROM lineitem_enc_rowid
-- ORDER BY l_orderkey_DET, l_linenumber_DET;
-- ALTER TABLE lineitem_enc_cryptdb_opt ADD PRIMARY KEY (l_orderkey_DET, l_linenumber_DET);
