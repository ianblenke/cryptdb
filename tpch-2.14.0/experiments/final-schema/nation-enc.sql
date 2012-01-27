create table nation_enc (

  `n_nationkey_DET` integer unsigned DEFAULT NULL,

  `n_name_DET` binary(25) DEFAULT NULL,

  -- OPE for strings is simply ope_enc(str[0..4]), and then
  -- treated as an integer...
  `n_name_OPE` bigint(20) unsigned DEFAULT NULL,

  `n_regionkey_DET` integer unsigned DEFAULT NULL,

  `n_comment_DET` varbinary(152) DEFAULT NULL,

  PRIMARY KEY (n_nationkey_DET)

) Engine=InnoDB;
