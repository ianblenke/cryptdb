CREATE TABLE part_enc_cryptdb_opt_with_det (

  p_partkey_DET bigint  NOT NULL,

  p_name_DET bytea NOT NULL,
  p_name_SWP bytea NOT NULL,

  p_mfgr_DET bytea NOT NULL,

  p_brand_DET bytea NOT NULL,

  p_type_DET bytea NOT NULL,
  p_type_SWP bytea NOT NULL,

  p_size_DET bigint  NOT NULL,
  p_size_OPE bigint  NOT NULL,

  p_container_DET bytea NOT NULL,

  p_retailprice_DET bigint  NOT NULL,

  p_comment_DET bytea NOT NULL

  --PRIMARY KEY (p_partkey_DET)

) ;

-- INSERT INTO part_enc_cryptdb_opt_with_det
-- SELECT
--   p_partkey_DET,
--   p_name_DET,
--   p_name_SWP,
--   p_mfgr_DET,
--   p_brand_DET,
--   p_type_DET,
--   p_type_SWP,
--   p_size_DET,
--   p_size_OPE,
--   p_container_DET,
--   p_retailprice_DET,
--   p_comment_DET 
-- FROM part_enc
-- ORDER BY p_partkey_DET;
-- ALTER TABLE part_enc_cryptdb_opt_with_det ADD PRIMARY KEY (p_partkey_DET);
