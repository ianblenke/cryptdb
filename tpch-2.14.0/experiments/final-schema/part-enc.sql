CREATE TABLE part_enc (

  `p_partkey_DET` integer unsigned DEFAULT NULL,
  `p_partkey_OPE` bigint(20) unsigned DEFAULT NULL,

  `p_name_DET` varbinary(55) DEFAULT NULL,

  `p_mfgr_DET` varbinary(25) DEFAULT NULL,

  `p_brand_DET` varbinary(10) DEFAULT NULL,

  `p_type_DET` varbinary(25) DEFAULT NULL,
  `p_type_SWP` varbinary(255) DEFAULT NULL,

  `p_size_DET` integer unsigned DEFAULT NULL,

  `p_container_DET` varbinary(10) DEFAULT NULL,

  `p_retailprice_DET` bigint(20) unsigned DEFAULT NULL,

  `p_comment_DET` varbinary(23) DEFAULT NULL,

  PRIMARY KEY (p_partkey_DET)

) Engine=InnoDB;
