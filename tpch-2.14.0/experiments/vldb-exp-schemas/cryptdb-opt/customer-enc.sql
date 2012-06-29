CREATE SEQUENCE customer_enc_cryptdb_opt_seq MINVALUE 0;
CREATE TABLE customer_enc_cryptdb_opt ( 
  c_custkey_DET     bigint  not null,
  c_name_DET        bytea not null,
  c_address_DET     bytea not null,
  c_nationkey_DET   bigint  not null,
  c_phone_DET       bytea not null,
  c_acctbal_DET     bigint  not null,
  c_acctbal_OPE     bytea not null,
  c_mktsegment_DET  bytea not null,
  c_comment_DET     bytea not null,
  c_phone_prefix_DET bytea not null,
  row_id bigint not null DEFAULT nextval('customer_enc_cryptdb_opt_seq')
  --PRIMARY KEY (c_custkey_DET)
);

--INSERT INTO customer_enc_cryptdb_opt
--SELECT 
--  c_custkey_DET,
--  c_name_DET,
--  c_address_DET,
--  c_nationkey_DET,
--  c_phone_DET,
--  c_acctbal_DET,
--  c_acctbal_OPE,
--  c_mktsegment_DET,
--  c_comment_DET,
--  c_phone_prefix_DET,
--  row_id
--FROM customer_enc_rowid
--ORDER BY c_custkey_DET;
--ALTER TABLE customer_enc_cryptdb_opt ADD PRIMARY KEY (c_custkey_DET);
