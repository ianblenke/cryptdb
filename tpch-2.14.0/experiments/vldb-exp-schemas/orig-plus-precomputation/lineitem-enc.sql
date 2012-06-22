create table lineitem_enc_orig_plus_precomputation (

  l_orderkey_DET bigint  NOT NULL, --

  l_partkey_DET bigint  NOT NULL, --

  l_suppkey_DET bigint  NOT NULL, --

  l_linenumber_DET bigint  NOT NULL,

  l_quantity_DET bigint  NOT NULL, --
  l_quantity_OPE bytea NOT NULL, --

  l_extendedprice_DET bigint  NOT NULL,

  l_discount_OPE bytea NOT NULL, --

  l_tax_DET bigint  NOT NULL,

  l_returnflag_DET smallint  NOT NULL,
  l_returnflag_OPE integer  NOT NULL,

  l_linestatus_DET smallint  NOT NULL,
  l_linestatus_OPE integer  NOT NULL,

  l_shipdate_OPE bigint  NOT NULL,

  l_commitdate_OPE bigint  NOT NULL, --

  l_receiptdate_OPE bigint  NOT NULL,

  l_shipinstruct_DET bytea NOT NULL,

  l_shipmode_DET bytea NOT NULL,
  l_shipmode_OPE bigint NOT NULL,

  l_comment_DET bytea NOT NULL,

  l_disc_price_DET bytea NOT NULL,

  l_shipdate_year_DET integer NOT NULL,
  l_shipdate_year_OPE bigint NOT NULL,

  l_packed_precompute_AGG bytea NOT NULL

  -- PRIMARY KEY (l_orderkey_DET, l_linenumber_DET)

) ;
