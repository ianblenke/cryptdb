create table lineitem_enc (

  -- field 0
  `l_orderkey_DET` integer  NOT NULL,

  -- field 1
  `l_partkey_DET` integer  NOT NULL,

  -- field 2
  `l_suppkey_DET` integer  NOT NULL,

  -- field 3
  `l_linenumber_DET` integer  NOT NULL,

  -- field 4
  `l_quantity_DET` bigint  NOT NULL,
  `l_quantity_OPE` bytea NOT NULL,

  -- field 5
  `l_extendedprice_DET` bigint  NOT NULL,

  -- field 6
  `l_discount_DET` bigint  NOT NULL,
  `l_discount_OPE` bytea NOT NULL,

  -- field 7
  `l_tax_DET` bigint  NOT NULL,

  -- field 8
  `l_returnflag_DET` smallint  NOT NULL,

  -- field 9
  `l_linestatus_DET` smallint  NOT NULL,

  -- field 10 
  `l_shipdate_DET` integer  NOT NULL,
  `l_shipdate_OPE` bigint  NOT NULL,

  -- field 11 
  `l_commitdate_DET` integer  NOT NULL,
  `l_commitdate_OPE` bigint  NOT NULL,

  -- field 12
  `l_receiptdate_DET` integer  NOT NULL,
  `l_receiptdate_OPE` bigint  NOT NULL,

  -- field 13
  `l_shipinstruct_DET` bytea NOT NULL,

  -- field 14
  `l_shipmode_DET` bytea NOT NULL,

  -- field 15
  `l_comment_DET` bytea NOT NULL,

  -- pre-computed fields

  `l_shipdate_year_DET` smallint  NOT NULL,
  `l_disc_price_DET` bigint  NOT NULL,

  PRIMARY KEY (l_orderkey_DET, l_linenumber_DET)

) ;
