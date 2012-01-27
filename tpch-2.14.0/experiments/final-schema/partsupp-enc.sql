create table partsupp_enc (

  `ps_partkey_DET` integer unsigned DEFAULT NULL,

  `ps_suppkey_DET` integer unsigned DEFAULT NULL,

  `ps_availqty_DET` integer unsigned DEFAULT NULL,

  `ps_supplycost_DET` bigint(20) unsigned DEFAULT NULL,
  `ps_supplycost_OPE` binary(16) DEFAULT NULL,

  `ps_comment_DET` binary(199) DEFAULT NULL,

  -- ps_value = ps_supplycost * ps_availqty
  `ps_value_DET` bigint(20) unsigned DEFAULT NULL,
  `ps_value_OPE` binary(16) DEFAULT NULL,
  `ps_value_AGG` varbinary(256) DEFAULT NULL,

  PRIMARY KEY (ps_partkey_DET, ps_suppkey_DET)

) Engine=InnoDB;
