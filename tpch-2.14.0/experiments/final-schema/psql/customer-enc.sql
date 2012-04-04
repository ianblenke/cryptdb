CREATE TABLE customer_enc ( 
    c_custkey_DET     integer  not null,
    c_name_DET        bytea not null,
    c_address_DET     bytea not null,
    c_nationkey_DET   integer  not null,
    c_phone_DET       bytea not null,
    c_acctbal_DET     bigint  not null,
    c_acctbal_OPE     bytea not null,
    c_mktsegment_DET  bytea not null,
    c_comment_DET     bytea not null,
    c_phone_prefix_DET bytea not null,
    PRIMARY KEY (c_custkey_DET)) ;
