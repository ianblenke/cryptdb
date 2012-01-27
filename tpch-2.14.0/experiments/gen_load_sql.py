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

-- DROP TABLE IF EXISTS LINEITEM;
DROP TABLE IF EXISTS NATION;
DROP TABLE IF EXISTS PART;
DROP TABLE IF EXISTS PARTSUPP;
DROP TABLE IF EXISTS REGION;
DROP TABLE IF EXISTS SUPPLIER;

-- DROP TABLE IF EXISTS lineitem_enc;
DROP TABLE IF EXISTS nation_enc;
DROP TABLE IF EXISTS part_enc;
DROP TABLE IF EXISTS partsupp_enc;
DROP TABLE IF EXISTS partsupp_enc_proj;
DROP TABLE IF EXISTS region_enc;
DROP TABLE IF EXISTS supplier_enc;

-- \. tpch-2.14.0/experiments/final-schema/lineitem-enc.sql
-- \. tpch-2.14.0/experiments/final-schema/lineitem.sql
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

-- Encrypted data

-- LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/lineitem.tbl.enc' INTO TABLE lineitem_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE lineitem_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/nation.tbl.enc' INTO TABLE nation_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE nation_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/part.tbl.enc' INTO TABLE part_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE part_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/partsupp.tbl.enc' INTO TABLE partsupp_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE partsupp_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/region.tbl.enc' INTO TABLE region_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE region_enc;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/supplier.tbl.enc' INTO TABLE supplier_enc FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE supplier_enc;

-- Regular data

-- LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/lineitem.tbl' INTO TABLE LINEITEM FIELDS TERMINATED BY '|' ESCAPED BY '\\';
-- OPTIMIZE TABLE LINEITEM;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/nation.tbl' INTO TABLE NATION FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE NATION;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/part.tbl' INTO TABLE PART FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE PART;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/partsupp.tbl' INTO TABLE PARTSUPP FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE PARTSUPP;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/region.tbl' INTO TABLE REGION FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE REGION;

LOAD DATA LOCAL INFILE 'enc-data/scale-#{scale}/supplier.tbl' INTO TABLE SUPPLIER FIELDS TERMINATED BY '|' ESCAPED BY '\\';
OPTIMIZE TABLE SUPPLIER;

-- Display table sizes
SELECT lower(TABLE_NAME), table_rows, data_length, index_length, avg_row_length, round(((data_length + index_length) / 1024 / 1024),2) AS Size_in_MB
FROM information_schema.TABLES WHERE table_schema = "tpch-#{scale}" order by lower(TABLE_NAME);''')
    return script

#for x in ['0.05', '0.25', '0.50', '0.75']:
for x in ['0.05']:
    print mkScript(x)
