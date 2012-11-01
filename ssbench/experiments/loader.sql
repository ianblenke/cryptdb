SET work_mem = '20GB';

\i /x/2/stephentu/cryptdb/ssbench/experiments/schema/customer.sql
\i /x/2/stephentu/cryptdb/ssbench/experiments/schema/date.sql
\i /x/2/stephentu/cryptdb/ssbench/experiments/schema/lineorder.sql
\i /x/2/stephentu/cryptdb/ssbench/experiments/schema/part.sql
\i /x/2/stephentu/cryptdb/ssbench/experiments/schema/supplier.sql

COPY customer FROM '/x/2/stephentu/cryptdb/enc-data-ssb/scale-0.1/customer.tbl' WITH DELIMITER AS '|';
COPY date_table FROM '/x/2/stephentu/cryptdb/enc-data-ssb/scale-0.1/date.tbl' WITH DELIMITER AS '|';
COPY lineorder FROM '/x/2/stephentu/cryptdb/enc-data-ssb/scale-0.1/lineorder.tbl' WITH DELIMITER AS '|';
COPY part FROM '/x/2/stephentu/cryptdb/enc-data-ssb/scale-0.1/part.tbl' WITH DELIMITER AS '|';
COPY supplier FROM '/x/2/stephentu/cryptdb/enc-data-ssb/scale-0.1/supplier.tbl' WITH DELIMITER AS '|';

alter table customer add primary key (C_CUSTKEY);
alter table date_table add primary key (D_DATEKEY);
alter table lineorder add primary key (LO_ORDERKEY, LO_LINENUMBER);
alter table part add primary key (P_PARTKEY);
alter table supplier add primary key (S_SUPPKEY); 

analyze verbose;
