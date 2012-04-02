create table partsupp_enc (

  `ps_partkey_DET` integer unsigned NOT NULL,

  `ps_suppkey_DET` integer unsigned NOT NULL,

  `ps_availqty_DET` integer unsigned NOT NULL,

  `ps_supplycost_DET` bigint(20) unsigned NOT NULL,
  `ps_supplycost_OPE` binary(16) NOT NULL,

  `ps_comment_DET` binary(199) NOT NULL,

  PRIMARY KEY (ps_partkey_DET, ps_suppkey_DET)

) Engine=InnoDB;
