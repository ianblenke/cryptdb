create table lineitem_enc (

  -- bitpacked
  `l_bitpacked_AGG_0` binary(32) DEFAULT NULL,
  `l_bitpacked_AGG_1` binary(32) DEFAULT NULL,
  `l_bitpacked_AGG_2` binary(32) DEFAULT NULL,
  `l_bitpacked_AGG_3` binary(32) DEFAULT NULL,
  `l_bitpacked_AGG_4` binary(32) DEFAULT NULL,
  `l_bitpacked_AGG_5` binary(32) DEFAULT NULL,
  `l_bitpacked_AGG_6` binary(32) DEFAULT NULL,
  `l_bitpacked_AGG_7` binary(32) DEFAULT NULL,

  -- field 0
  `l_orderkey_DET` integer unsigned DEFAULT NULL,

  -- field 1
  `l_partkey_DET` integer unsigned DEFAULT NULL,

  -- field 2
  `l_suppkey_DET` integer unsigned DEFAULT NULL,

  -- field 3
  `l_linenumber_DET` integer unsigned DEFAULT NULL,

  -- field 4
  `l_quantity_DET` bigint(20) unsigned DEFAULT NULL,

  -- field 5
  `l_extendedprice_DET` bigint(20) unsigned DEFAULT NULL,

  -- field 6
  `l_discount_DET` bigint(20) unsigned DEFAULT NULL,

  -- field 7
  `l_tax_DET` bigint(20) unsigned DEFAULT NULL,

  -- field 8
  `l_returnflag_DET` tinyint unsigned DEFAULT NULL,
  `l_returnflag_OPE` smallint unsigned DEFAULT NULL,

  -- field 9
  `l_linestatus_DET` tinyint unsigned DEFAULT NULL,
  `l_linestatus_OPE` smallint unsigned DEFAULT NULL,

  -- field 10 
  `l_shipdate_DET` mediumint unsigned DEFAULT NULL,
  `l_shipdate_OPE` bigint(20) unsigned DEFAULT NULL,

  -- field 11 
  `l_commitdate_DET` mediumint unsigned DEFAULT NULL,

  -- field 12
  `l_receiptdate_DET` mediumint unsigned DEFAULT NULL,

  -- field 13
  `l_shipinstruct_DET` binary(25) DEFAULT NULL,

  -- field 14
  `l_shipmode_DET` binary(10) DEFAULT NULL,

  -- field 15
  `l_comment_DET` varbinary(44) DEFAULT NULL,

  PRIMARY KEY (l_orderkey_DET, l_linenumber_DET)

) Engine=InnoDB;
