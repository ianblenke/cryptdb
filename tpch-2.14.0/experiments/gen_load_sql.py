#!/usr/bin/env python

import sys, re
def interp(string):
  locals  = sys._getframe(1).f_locals
  globals = sys._getframe(1).f_globals
  for item in re.findall(r'#\{([^{]*)\}', string):
    string = string.replace('#{%s}' % item,
                            str(eval(item, globals, locals)))
  return string

## scale is assumed to be a string
def mkScript(scale):
    script = interp(
r'''CREATE DATABASE IF NOT EXISTS `tpch-#{scale}`;
USE `tpch-#{scale}`;

DROP TABLE IF EXISTS LINEITEM;
DROP TABLE IF EXISTS NATION;
DROP TABLE IF EXISTS PART;
DROP TABLE IF EXISTS PARTSUPP;
DROP TABLE IF EXISTS REGION;
DROP TABLE IF EXISTS SUPPLIER;
DROP TABLE IF EXISTS CUSTOMER;
DROP TABLE IF EXISTS ORDERS;

DROP TABLE IF EXISTS LINEITEM_INT;
DROP TABLE IF EXISTS NATION_INT;
DROP TABLE IF EXISTS PART_INT;
DROP TABLE IF EXISTS PARTSUPP_INT;
DROP TABLE IF EXISTS REGION_INT;
DROP TABLE IF EXISTS SUPPLIER_INT;
DROP TABLE IF EXISTS CUSTOMER_INT;
DROP TABLE IF EXISTS ORDERS_INT;

DROP TABLE IF EXISTS lineitem_enc;
DROP TABLE IF EXISTS lineitem_enc_rowid;
DROP TABLE IF EXISTS nation_enc;
DROP TABLE IF EXISTS part_enc;
DROP TABLE IF EXISTS partsupp_enc;
DROP TABLE IF EXISTS region_enc;
DROP TABLE IF EXISTS supplier_enc;
DROP TABLE IF EXISTS customer_enc;
DROP TABLE IF EXISTS customer_enc_rowid;
DROP TABLE IF EXISTS orders_enc;

\. tpch-2.14.0/experiments/final-schema/lineitem-enc.sql
\. tpch-2.14.0/experiments/final-schema/lineitem.sql
\. tpch-2.14.0/experiments/final-schema/nation-enc.sql
\. tpch-2.14.0/experiments/final-schema/nation.sql
\. tpch-2.14.0/experiments/final-schema/part-enc.sql
\. tpch-2.14.0/experiments/final-schema/part.sql
\. tpch-2.14.0/experiments/final-schema/partsupp-enc.sql
\. tpch-2.14.0/experiments/final-schema/partsupp.sql
\. tpch-2.14.0/experiments/final-schema/region-enc.sql
\. tpch-2.14.0/experiments/final-schema/region.sql
\. tpch-2.14.0/experiments/final-schema/supplier-enc.sql
\. tpch-2.14.0/experiments/final-schema/supplier.sql
\. tpch-2.14.0/experiments/final-schema/customer-enc.sql
\. tpch-2.14.0/experiments/final-schema/customer.sql
\. tpch-2.14.0/experiments/final-schema/orders-enc.sql
\. tpch-2.14.0/experiments/final-schema/orders.sql

-- Encrypted data

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/lineitem.tbl.enc.sorted' INTO TABLE lineitem_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE lineitem_enc;

CREATE TABLE lineitem_enc_rowid LIKE lineitem_enc;
ALTER TABLE lineitem_enc_rowid ADD COLUMN row_id INTEGER UNSIGNED NOT NULL;
SET @cnt := -1; INSERT INTO lineitem_enc_rowid SELECT *, @cnt := @cnt + 1 FROM lineitem_enc;
OPTIMIZE TABLE lineitem_enc_rowid;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/nation.tbl.enc.sorted' INTO TABLE nation_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE nation_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/part.tbl.enc.sorted' INTO TABLE part_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE part_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/partsupp.tbl.enc.sorted' INTO TABLE partsupp_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE partsupp_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/region.tbl.enc.sorted' INTO TABLE region_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE region_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/supplier.tbl.enc.sorted' INTO TABLE supplier_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE supplier_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/customer.tbl.enc.sorted' INTO TABLE customer_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE customer_enc;

CREATE TABLE customer_enc_rowid LIKE customer_enc;
ALTER TABLE customer_enc_rowid ADD COLUMN row_id INTEGER UNSIGNED NOT NULL;
SET @cnt := -1; INSERT INTO customer_enc_rowid SELECT *, @cnt := @cnt + 1 FROM customer_enc;
OPTIMIZE TABLE customer_enc_rowid;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/orders.tbl.enc.sorted' INTO TABLE orders_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE orders_enc;

-- Regular data

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/lineitem.tbl' INTO TABLE LINEITEM FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE LINEITEM;

CREATE TABLE LINEITEM_INT LIKE LINEITEM;
ALTER TABLE LINEITEM_INT
  MODIFY COLUMN L_QUANTITY BIGINT NOT NULL,
  MODIFY COLUMN L_EXTENDEDPRICE BIGINT NOT NULL,
  MODIFY COLUMN L_DISCOUNT BIGINT NOT NULL,
  MODIFY COLUMN L_TAX BIGINT NOT NULL ;

INSERT INTO LINEITEM_INT
SELECT
  L_ORDERKEY,
  L_PARTKEY,
  L_SUPPKEY,
  L_LINENUMBER,
  L_QUANTITY * 100.0,
  L_EXTENDEDPRICE * 100.0,
  L_DISCOUNT * 100.0,
  L_TAX * 100.0,
  L_RETURNFLAG,
  L_LINESTATUS,
  L_SHIPDATE,
  L_COMMITDATE,
  L_RECEIPTDATE,
  L_SHIPINSTRUCT,
  L_SHIPMODE,
  L_COMMENT
FROM LINEITEM;
OPTIMIZE TABLE LINEITEM_INT;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/nation.tbl' INTO TABLE NATION FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE NATION;

CREATE TABLE NATION_INT LIKE NATION;
INSERT INTO NATION_INT SELECT * FROM NATION;
OPTIMIZE TABLE NATION_INT;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/part.tbl' INTO TABLE PART FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE PART;

CREATE TABLE PART_INT LIKE PART;
ALTER TABLE PART_INT
  MODIFY COLUMN P_RETAILPRICE BIGINT NOT NULL;

INSERT INTO PART_INT
SELECT
  P_PARTKEY,
  P_NAME,
  P_MFGR,
  P_BRAND,
  P_TYPE,
  P_SIZE,
  P_CONTAINER,
  P_RETAILPRICE * 100.0,
  P_COMMENT
FROM PART;
OPTIMIZE TABLE PART_INT;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/partsupp.tbl' INTO TABLE PARTSUPP FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE PARTSUPP;

CREATE TABLE PARTSUPP_INT LIKE PARTSUPP;
ALTER TABLE PARTSUPP_INT
  MODIFY COLUMN PS_SUPPLYCOST BIGINT NOT NULL;

INSERT INTO PARTSUPP_INT
SELECT
  PS_PARTKEY,
  PS_SUPPKEY,
  PS_AVAILQTY,
  PS_SUPPLYCOST * 100.0,
  PS_COMMENT
FROM PARTSUPP;
OPTIMIZE TABLE PARTSUPP_INT;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/region.tbl' INTO TABLE REGION FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE REGION;

CREATE TABLE REGION_INT LIKE REGION;
INSERT INTO REGION_INT SELECT * FROM REGION;
OPTIMIZE TABLE REGION_INT;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/supplier.tbl' INTO TABLE SUPPLIER FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE SUPPLIER;

CREATE TABLE SUPPLIER_INT LIKE SUPPLIER;
ALTER TABLE SUPPLIER_INT
  MODIFY COLUMN S_ACCTBAL BIGINT NOT NULL;

INSERT INTO SUPPLIER_INT
SELECT
  S_SUPPKEY,
  S_NAME,
  S_ADDRESS,
  S_NATIONKEY,
  S_PHONE,
  S_ACCTBAL * 100.0,
  S_COMMENT
FROM SUPPLIER;
OPTIMIZE TABLE SUPPLIER_INT;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/customer.tbl' INTO TABLE CUSTOMER FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE CUSTOMER;

CREATE TABLE CUSTOMER_INT LIKE CUSTOMER;
ALTER TABLE CUSTOMER_INT
  MODIFY COLUMN C_ACCTBAL BIGINT NOT NULL;

INSERT INTO CUSTOMER_INT
SELECT
  C_CUSTKEY,
  C_NAME,
  C_ADDRESS,
  C_NATIONKEY,
  C_PHONE,
  C_ACCTBAL * 100.0,
  C_MKTSEGMENT,
  C_COMMENT
FROM CUSTOMER;
OPTIMIZE TABLE CUSTOMER_INT;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/orders.tbl' INTO TABLE ORDERS FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE ORDERS;

CREATE TABLE ORDERS_INT LIKE ORDERS;
ALTER TABLE ORDERS_INT
  MODIFY COLUMN O_TOTALPRICE BIGINT NOT NULL;

INSERT INTO ORDERS_INT
SELECT
  O_ORDERKEY,
  O_CUSTKEY,
  O_ORDERSTATUS,
  O_TOTALPRICE * 100.0,
  O_ORDERDATE,
  O_ORDERPRIORITY,
  O_CLERK,
  O_SHIPPRIORITY,
  O_COMMENT
FROM ORDERS;
OPTIMIZE TABLE ORDERS_INT;

-- Indexes

ALTER TABLE orders_enc ADD INDEX (o_custkey_DET);
ALTER TABLE ORDERS_INT ADD INDEX (O_CUSTKEY);

-- Display table sizes
SELECT lower(TABLE_NAME), table_rows, data_length, index_length, avg_row_length, round(((data_length + index_length) / 1024 / 1024),2) AS Size_in_MB
FROM information_schema.TABLES WHERE table_schema = "tpch-#{scale}" order by lower(TABLE_NAME);''')
    return script

if __name__ == "__main__":
    if len(sys.argv) == 1:
        print >>sys.stderr, "Need scale arguments"
        sys.exit(1)
    for x in sys.argv[1:]:
        print mkScript(x)
