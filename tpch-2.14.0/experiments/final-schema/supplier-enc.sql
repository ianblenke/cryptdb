create table supplier_enc (

  `s_suppkey_DET` bigint(20) unsigned DEFAULT NULL,
  `s_suppkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `s_suppkey_AGG` varbinary(256) DEFAULT NULL,

  `s_name_DET` blob,
  `s_name_OPE` bigint(20) unsigned DEFAULT NULL,
  `s_name_SWP` blob,

  `s_address_DET` blob,
  `s_address_OPE` bigint(20) unsigned DEFAULT NULL,
  `s_address_SWP` blob,

  `s_nationkey_DET` bigint(20) unsigned DEFAULT NULL,
  `s_nationkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `s_nationkey_AGG` varbinary(256) DEFAULT NULL,

  `s_phone_DET` blob,
  `s_phone_OPE` bigint(20) unsigned DEFAULT NULL,
  `s_phone_SWP` blob,

  `s_acctbal_DET` bigint(20) unsigned DEFAULT NULL,
  `s_acctbal_OPE` bigint(20) unsigned DEFAULT NULL,
  `s_acctbal_AGG` varbinary(256) DEFAULT NULL,

  `s_comment_DET` blob,
  `s_comment_OPE` bigint(20) unsigned DEFAULT NULL,
  `s_comment_SWP` blob,

  `table_salt` bigint(20) unsigned DEFAULT NULL,

  PRIMARY KEY (s_suppkey_DET)

) Engine=InnoDB;
