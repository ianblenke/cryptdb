create table lineitem_enc_noopt (

  -- field 0
  `l_orderkey_DET` integer unsigned NOT NULL,

  -- field 1
  `l_partkey_DET` integer unsigned NOT NULL,

  -- field 2
  `l_suppkey_DET` integer unsigned NOT NULL,

  -- field 3
  `l_linenumber_DET` integer unsigned NOT NULL,

  -- field 4
  `l_quantity_DET` bigint(20) unsigned NOT NULL,
  `l_quantity_OPE` binary(16) NOT NULL,

  -- field 5
  `l_extendedprice_DET` bigint(20) unsigned NOT NULL,

  -- field 6
  `l_discount_DET` bigint(20) unsigned NOT NULL,
  `l_discount_OPE` binary(16) NOT NULL,

  -- field 7
  `l_tax_DET` bigint(20) unsigned NOT NULL,

  -- field 8
  `l_returnflag_DET` tinyint unsigned NOT NULL,

  -- field 9
  `l_linestatus_DET` tinyint unsigned NOT NULL,

  -- field 10 
  `l_shipdate_DET` mediumint unsigned NOT NULL,
  `l_shipdate_OPE` bigint(20) unsigned NOT NULL,

  -- field 11 
  `l_commitdate_DET` mediumint unsigned NOT NULL,
  `l_commitdate_OPE` bigint(20) unsigned NOT NULL,

  -- field 12
  `l_receiptdate_DET` mediumint unsigned NOT NULL,
  `l_receiptdate_OPE` bigint(20) unsigned NOT NULL,

  -- field 13
  `l_shipinstruct_DET` binary(25) NOT NULL,

  -- field 14
  `l_shipmode_DET` binary(10) NOT NULL,

  -- field 15
  `l_comment_DET` varbinary(44) NOT NULL,

  `l_bitpacked_AGG` varbinary(256) NOT NULL,

  PRIMARY KEY (l_orderkey_DET, l_linenumber_DET)

) Engine=InnoDB;

