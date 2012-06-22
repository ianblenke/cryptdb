CREATE TABLE orders_enc_orig_system  ( 
    o_orderkey_DET       bigint  not null,

    o_custkey_DET        bigint  not null,

    o_orderstatus_DET    smallint  not null,

    o_totalprice_DET     bigint  not null,
    o_totalprice_OPE     bytea not null,

    o_orderdate_DET      integer  not null,
    o_orderdate_OPE      bigint  not null,

    o_orderpriority_DET  bytea not null,  
    o_orderpriority_OPE  bigint not null,  

    o_clerk_DET          bytea not null, 

    o_shippriority_DET   bigint  not null,

    o_comment_DET        bytea not null

    --PRIMARY KEY (o_orderkey_DET)
    --INDEX (o_custkey_DET)
  
) ;

INSERT INTO orders_enc_orig_system
SELECT
    o_orderkey_DET,
    o_custkey_DET,
    o_orderstatus_DET,
    o_totalprice_DET,
    o_totalprice_OPE,
    o_orderdate_DET,
    o_orderdate_OPE,
    o_orderpriority_DET,  
    o_orderpriority_OPE,  
    o_clerk_DET, 
    o_shippriority_DET,
    o_comment_DET
FROM orders_enc_cryptdb_greedy
ORDER BY o_orderkey_DET;

ALTER TABLE orders_enc_orig_system ADD PRIMARY KEY (o_orderkey_DET);
CREATE INDEX orders_enc_orig_system_o_custkey_idx 
  ON orders_enc_orig_system (o_custkey_DET);
