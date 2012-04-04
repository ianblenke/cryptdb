create table partsupp_enc (

  `ps_partkey_DET` integer  NOT NULL,

  `ps_suppkey_DET` integer  NOT NULL,

  `ps_availqty_DET` integer  NOT NULL,

  `ps_supplycost_DET` bigint  NOT NULL,
  `ps_supplycost_OPE` bytea NOT NULL,

  `ps_comment_DET` bytea NOT NULL,

  PRIMARY KEY (ps_partkey_DET, ps_suppkey_DET)

) ;
