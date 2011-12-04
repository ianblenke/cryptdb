CREATE TABLE part_enc (

  `p_partkey_DET` bigint(20) unsigned DEFAULT NULL,
  `p_partkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_partkey_AGG` varbinary(256) DEFAULT NULL,

  `p_name_DET` blob,
  `p_name_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_name_SWP` blob,

  `p_mfgr_DET` blob,
  `p_mfgr_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_mfgr_SWP` blob,

  `p_brand_DET` blob,
  `p_brand_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_brand_SWP` blob,

  `p_type_DET` blob,
  `p_type_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_type_SWP` blob,

  `p_size_DET` bigint(20) unsigned DEFAULT NULL,
  `p_size_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_size_AGG` varbinary(256) DEFAULT NULL,

  `p_container_DET` blob,
  `p_container_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_container_SWP` blob,

  `p_retailprice_DET` bigint(20) unsigned DEFAULT NULL,
  `p_retailprice_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_retailprice_AGG` varbinary(256) DEFAULT NULL,

  `p_comment_DET` blob,
  `p_comment_OPE` bigint(20) unsigned DEFAULT NULL,
  `p_comment_SWP` blob,

  `table_salt` bigint(20) unsigned DEFAULT NULL,

  PRIMARY KEY (p_partkey_DET)

) Engine=InnoDB;
