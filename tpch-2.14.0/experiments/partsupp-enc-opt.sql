create table partsupp_enc_opt (

  `ps_partkey_DET` bigint(20) unsigned DEFAULT NULL,
  `ps_partkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `ps_partkey_AGG` varbinary(256) DEFAULT NULL,
  `ps_partkey_SALT` bigint(20) unsigned DEFAULT NULL,

  `ps_suppkey_DET` bigint(20) unsigned DEFAULT NULL,
  `ps_suppkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `ps_suppkey_AGG` varbinary(256) DEFAULT NULL,
  `ps_suppkey_SALT` bigint(20) unsigned DEFAULT NULL,

  `ps_availqty_DET` bigint(20) unsigned DEFAULT NULL,
  `ps_availqty_OPE` bigint(20) unsigned DEFAULT NULL,
  `ps_availqty_AGG` varbinary(256) DEFAULT NULL,
  `ps_availqty_SALT` bigint(20) unsigned DEFAULT NULL,

  `ps_supplycost_DET` bigint(20) unsigned DEFAULT NULL,
  `ps_supplycost_OPE` bigint(20) unsigned DEFAULT NULL,
  `ps_supplycost_AGG` varbinary(256) DEFAULT NULL,
  `ps_supplycost_SALT` bigint(20) unsigned DEFAULT NULL,

  `ps_comment_DET` blob,
  `ps_comment_OPE` bigint(20) unsigned DEFAULT NULL,
  `ps_comment_SALT` bigint(20) unsigned DEFAULT NULL,

  `ps_value_DET` bigint(20) unsigned DEFAULT NULL,
  `ps_value_OPE` bigint(20) unsigned DEFAULT NULL,
  `ps_value_AGG` varbinary(256) DEFAULT NULL,
  `ps_value_SALT` bigint(20) unsigned DEFAULT NULL,

  PRIMARY KEY (ps_partkey_DET, ps_suppkey_DET)

) Engine=InnoDB;
