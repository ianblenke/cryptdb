create table nation_enc (

  `n_nationkey_DET` bigint(20) unsigned DEFAULT NULL,
  `n_nationkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `n_nationkey_AGG` varbinary(256) DEFAULT NULL,
  `n_nationkey_SALT` bigint(20) unsigned DEFAULT NULL,

  `n_name_DET` blob,
  `n_name_OPE` bigint(20) unsigned DEFAULT NULL,
  `n_name_SALT` bigint(20) unsigned DEFAULT NULL,

  `n_regionkey_DET` bigint(20) unsigned DEFAULT NULL,
  `n_regionkey_OPE` bigint(20) unsigned DEFAULT NULL,
  `n_regionkey_AGG` varbinary(256) DEFAULT NULL,
  `n_regionkey_SALT` bigint(20) unsigned DEFAULT NULL,

  `n_comment_DET` blob,
  `n_comment_OPE` bigint(20) unsigned DEFAULT NULL,
  `n_comment_SALT` bigint(20) unsigned DEFAULT NULL,

  INDEX (n_nationkey_DET)

) Engine=InnoDB;
