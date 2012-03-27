CREATE TABLE customer_enc ( 
    c_custkey_DET     integer unsigned not null,
    c_name_DET        varbinary(25) not null,
    c_address_DET     varbinary(40) not null,
    c_nationkey_DET   integer unsigned not null,
    c_phone_DET       binary(15) not null,
    c_acctbal_DET     bigint(20) unsigned not null,
    c_acctbal_OPE     binary(16) not null,
    c_mktsegment_DET  binary(10) not null,
    c_comment_DET     varbinary(117) not null,
    PRIMARY KEY (c_custkey_DET)) Engine=InnoDB;
