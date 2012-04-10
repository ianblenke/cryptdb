CREATE TABLE part_enc (

  p_partkey_DET bigint  NOT NULL,
  p_partkey_OPE bigint  NOT NULL,

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

  p_comment_DET bytea NOT NULL,

  PRIMARY KEY (p_partkey_DET)

) ;
