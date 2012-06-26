CREATE TABLE orders_enc_orig_plus_precomputation  ( 
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

    o_comment_DET        bytea not null,

    o_orderdate_year_DET integer not null,
    o_orderdate_year_OPE bigint not null

    --PRIMARY KEY (o_orderkey_DET)
    --INDEX (o_custkey_DET)
  
) ;
