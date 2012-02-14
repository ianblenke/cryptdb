CREATE TABLE orders_enc  ( 
    o_orderkey_DET       integer unsigned not null,
    o_custkey_DET        integer unsigned not null,
    o_orderstatus_DET    tinyint unsigned not null,
    o_totalprice_DET     bigint(20) unsigned not null,
    o_totalprice_OPE     binary(16) not null,
    o_orderdate_DET      integer unsigned not null,
    o_orderdate_OPE      bigint(20) unsigned not null,
    o_orderpriority_DET  binary(15) not null,  
    o_clerk_DET          binary(15) not null, 
    o_shippriority_DET   integer unsigned not null,
    o_comment_DET        varbinary(79) not null,
    PRIMARY KEY (o_orderkey_DET)) Engine=InnoDB;
