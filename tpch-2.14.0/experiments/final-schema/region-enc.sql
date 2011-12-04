CREATE TABLE region_enc (

  `r_regionkey_DET` bigint(20) unsigned DEFAULT NULL,
  `r_regionkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `r_regionkey_AGG` varbinary(256) DEFAULT NULL,

  `r_name_DET` blob,
  `r_name_OPE` bigint(20) unsigned DEFAULT NULL,
  `r_name_SWP` blob,

  `r_comment_DET` blob,
  `r_comment_OPE` bigint(20) unsigned DEFAULT NULL,
  `r_comment_SWP` blob,

  `r_regionkey_SALT` bigint(20) unsigned DEFAULT NULL,

  PRIMARY KEY (r_regionkey_DET)

) Engine=InnoDB;

