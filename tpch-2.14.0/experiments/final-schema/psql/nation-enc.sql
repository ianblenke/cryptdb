create table nation_enc (

  `n_nationkey_DET` integer  NOT NULL,

  `n_name_DET` bytea NOT NULL,

  -- OPE for strings is simply ope_enc(str[0..4]), and then
  -- treated as an integer...
  `n_name_OPE` bigint  NOT NULL,

  `n_regionkey_DET` integer  NOT NULL,

  `n_comment_DET` bytea,

  PRIMARY KEY (n_nationkey_DET)

) ;
