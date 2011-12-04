create table lineitem_enc (

  -- field 0
  `l_orderkey_DET` bigint(20) unsigned DEFAULT NULL,
  `l_orderkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_orderkey_AGG` varbinary(256) DEFAULT NULL,

  -- field 1
  `l_partkey_DET` bigint(20) unsigned DEFAULT NULL,
  `l_partkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_partkey_AGG` varbinary(256) DEFAULT NULL,

  -- field 2
  `l_suppkey_DET` bigint(20) unsigned DEFAULT NULL,
  `l_suppkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_suppkey_AGG` varbinary(256) DEFAULT NULL,

  -- field 3
  `l_linenumber_DET` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_AGG` varbinary(256) DEFAULT NULL,

  -- field 4
  `l_quantity_DET` bigint(20) unsigned DEFAULT NULL,
  `l_quantity_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_quantity_AGG` varbinary(256) DEFAULT NULL,

  -- field 5
  `l_extendedprice_DET` bigint(20) unsigned DEFAULT NULL,
  `l_extendedprice_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_extendedprice_AGG` varbinary(256) DEFAULT NULL,

  -- field 6
  `l_discount_DET` bigint(20) unsigned DEFAULT NULL,
  `l_discount_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_discount_AGG` varbinary(256) DEFAULT NULL,

  -- field 7
  `l_tax_DET` bigint(20) unsigned DEFAULT NULL,
  `l_tax_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_tax_AGG` varbinary(256) DEFAULT NULL,

  -- field 8
  `l_returnflag_DET` bigint(20) unsigned DEFAULT NULL,
  `l_returnflag_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_returnflag_AGG` varbinary(256) DEFAULT NULL,

  -- field 9
  `l_linestatus_DET` bigint(20) unsigned DEFAULT NULL,
  `l_linestatus_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_linestatus_AGG` varbinary(256) DEFAULT NULL,

  -- field 10 
  `l_shipdate_DET` bigint(20) unsigned DEFAULT NULL,
  `l_shipdate_OPE` bigint(20) unsigned DEFAULT NULL,

  -- field 11 
  `l_commitdate_DET` bigint(20) unsigned DEFAULT NULL,
  `l_commitdate_OPE` bigint(20) unsigned DEFAULT NULL,

  -- field 12
  `l_receiptdate_DET` bigint(20) unsigned DEFAULT NULL,
  `l_receiptdate_OPE` bigint(20) unsigned DEFAULT NULL,

  -- field 13
  `l_shipinstruct_DET` blob,
  `l_shipinstruct_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_shipinstruct_SWP` blob,

  -- field 14
  `l_shipmode_DET` blob,
  `l_shipmode_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_shipmode_SWP` blob,

  -- field 15
  `l_comment_DET` blob,
  `l_comment_OPE` bigint(20) unsigned DEFAULT NULL,
  `l_comment_SWP` blob,

  -- bitpacked
  `l_bitpacked_AGG` varbinary(256) DEFAULT NULL,

  `table_salt` bigint(20) unsigned DEFAULT NULL,

  PRIMARY KEY (l_orderkey_DET, l_linenumber_DET)

) Engine=InnoDB;
