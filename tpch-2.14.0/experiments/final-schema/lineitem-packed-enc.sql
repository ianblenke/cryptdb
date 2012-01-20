create table lineitem_packed_enc (

  `l_returnflag_DET` bigint(20) unsigned DEFAULT NULL,
  `l_linestatus_DET` bigint(20) unsigned DEFAULT NULL,

  `l_shipdate_OPE_start` bigint(20) unsigned DEFAULT NULL,
  `l_shipdate_OPE_end` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_1` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_1` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_2` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_2` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_3` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_3` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_4` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_4` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_5` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_5` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_6` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_6` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_7` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_7` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_8` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_8` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_9` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_9` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_10` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_10` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_11` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_11` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_12` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_12` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_13` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_13` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_14` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_14` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_15` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_15` bigint(20) unsigned DEFAULT NULL,

  `l_orderkey_DET_16` bigint(20) unsigned DEFAULT NULL,
  `l_linenumber_DET_16` bigint(20) unsigned DEFAULT NULL,

  `packed_count` tinyint unsigned,

  `z1` varbinary(256) DEFAULT NULL,
  `z2` varbinary(256) DEFAULT NULL,
  `z3` varbinary(256) DEFAULT NULL,
  `z4` varbinary(256) DEFAULT NULL,
  `z5` varbinary(256) DEFAULT NULL

) Engine=InnoDB;
