create table supplier_enc_cryptdb_opt (

  s_suppkey_DET bigint  NOT NULL,

  s_name_DET bytea NOT NULL,
  -- OPE for strings is simply ope_enc(str[0..4]), and then
  -- treated as an bigint...
  s_name_OPE bigint  NOT NULL,

  s_address_DET bytea NOT NULL, /* could be RAND */

  s_nationkey_DET bigint  NOT NULL,

  s_phone_DET bytea NOT NULL, /* could be RAND */

  s_acctbal_DET bigint  NOT NULL, /* could be RAND */

  s_comment_DET bytea NOT NULL /* could be RAND */

  --PRIMARY KEY (s_suppkey_DET)
) ;

-- INSERT INTO supplier_enc_cryptdb_opt
-- SELECT 
--   s_suppkey_DET,
--   s_name_DET,
--   s_name_OPE,
--   s_address_DET,
--   s_nationkey_DET,
--   s_phone_DET,
--   s_acctbal_DET,
--   s_comment_DET 
-- FROM supplier_enc
-- ORDER BY s_suppkey_DET;
-- ALTER TABLE supplier_enc_cryptdb_opt ADD PRIMARY KEY (s_suppkey_DET);
