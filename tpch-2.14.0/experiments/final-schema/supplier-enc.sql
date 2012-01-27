create table supplier_enc (

  `s_suppkey_DET` integer unsigned DEFAULT NULL,

  `s_name_DET` binary(25) DEFAULT NULL,
  -- OPE for strings is simply ope_enc(str[0..4]), and then
  -- treated as an integer...
  `s_name_OPE` bigint(20) unsigned DEFAULT NULL,

  `s_address_DET` varbinary(40) DEFAULT NULL,

  `s_nationkey_DET` integer unsigned DEFAULT NULL,

  `s_phone_DET` binary(15) DEFAULT NULL,

  `s_acctbal_DET` bigint(20) unsigned DEFAULT NULL,
  `s_acctbal_OPE` binary(16) DEFAULT NULL,

  `s_comment_DET` varbinary(101) DEFAULT NULL,

  PRIMARY KEY (s_suppkey_DET)
) Engine=InnoDB;
