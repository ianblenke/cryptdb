create table partsupp_enc_cryptdb_opt (

  ps_partkey_DET bigint  NOT NULL,

  ps_suppkey_DET bigint  NOT NULL,

  ps_availqty_DET bigint  NOT NULL,

  ps_supplycost_DET bigint  NOT NULL,
  ps_supplycost_OPE bytea NOT NULL,

  ps_comment_DET bytea NOT NULL,

  ps_volume_DET bigint NOT NULL

  --PRIMARY KEY (ps_partkey_DET, ps_suppkey_DET)

) ;

-- INSERT INTO partsupp_enc_cryptdb_opt
-- SELECT
--   ps_partkey_DET,
--   ps_suppkey_DET,
--   ps_availqty_DET,
--   ps_supplycost_DET,
--   ps_supplycost_OPE,
--   ps_comment_DET,
--   ps_volume_DET
-- FROM partsupp_enc_rowid
-- ORDER BY ps_partkey_DET, ps_suppkey_DET;
-- ALTER TABLE partsupp_enc_cryptdb_opt ADD PRIMARY KEY (ps_partkey_DET, ps_suppkey_DET);
