create table nation_enc_orig_plus_col_packing (

  n_nationkey_DET bigint  NOT NULL,

  n_name_DET bytea NOT NULL,

  -- OPE for strings is simply ope_enc(str[0..4]), and then
  -- treated as an bigint...
  n_name_OPE bigint  NOT NULL,

  n_regionkey_DET bigint  NOT NULL,

  n_comment_DET bytea

  --PRIMARY KEY (n_nationkey_DET)

) ;

--INSERT INTO nation_enc_cryptdb_opt
--SELECT 
--  n_nationkey_DET,
--  n_name_DET,
--  n_name_OPE,
--  n_regionkey_DET,
--  n_comment_DET 
--FROM nation_enc;
--ALTER TABLE nation_enc_cryptdb_opt ADD PRIMARY KEY (n_nationkey_DET);
