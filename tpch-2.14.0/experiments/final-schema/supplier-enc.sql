create table supplier_enc (

  `s_suppkey_DET` integer unsigned NOT NULL,

  `s_name_DET` binary(25) NOT NULL,
  -- OPE for strings is simply ope_enc(str[0..4]), and then
  -- treated as an integer...
  `s_name_OPE` bigint(20) unsigned NOT NULL,

  `s_address_DET` varbinary(40) NOT NULL,

  `s_nationkey_DET` integer unsigned NOT NULL,

  `s_phone_DET` binary(15) NOT NULL,

  `s_acctbal_DET` bigint(20) unsigned NOT NULL,
  `s_acctbal_OPE` binary(16) NOT NULL,

  `s_comment_DET` varbinary(101) NOT NULL,

  PRIMARY KEY (s_suppkey_DET)
) Engine=InnoDB;
