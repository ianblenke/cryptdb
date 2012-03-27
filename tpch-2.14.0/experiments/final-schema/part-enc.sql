CREATE TABLE part_enc (

  `p_partkey_DET` integer unsigned NOT NULL,
  `p_partkey_OPE` bigint(20) unsigned NOT NULL,

  `p_name_DET` varbinary(55) NOT NULL,
  `p_name_SWP` varbinary(255) NOT NULL,

  `p_mfgr_DET` varbinary(25) NOT NULL,

  `p_brand_DET` varbinary(10) NOT NULL,

  `p_type_DET` varbinary(25) NOT NULL,
  `p_type_SWP` varbinary(255) NOT NULL,

  `p_size_DET` integer unsigned NOT NULL,
  `p_size_OPE` bigint unsigned NOT NULL,

  `p_container_DET` varbinary(10) NOT NULL,

  `p_retailprice_DET` bigint(20) unsigned NOT NULL,

  `p_comment_DET` varbinary(23) NOT NULL,

  PRIMARY KEY (p_partkey_DET)

) Engine=InnoDB;
