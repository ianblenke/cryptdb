create table partsupp_enc_proj (
  `nationkey_DET` bigint(20) unsigned DEFAULT NULL,
  `ps_value_bitpacked_AGG` varbinary(256) DEFAULT NULL
) Engine=InnoDB;
