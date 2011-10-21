#!/bin/bash

# WARNING: run make first, to compile the necessary binaries
if [ ! -x ./dbgen ]; then
  echo "run make first";
  exit 1;
fi

# Run this script to set up mysql to run tpc-h. Assumes mysql is running at
# localhost, and assumes running in the `dbgen` directory

MYSQL_ROOT_USER=root
MYSQL_ROOT_PASS=letmein  # fill me in

TPCH_USER=tpch
TPCH_PASS=password # can change if desired

TPCH_DB=tpch

TPCH_SCALING_FACTOR=1 # change this to generate more data

# Set up mysql tpch user and database
echo "Adding tpch user"
mysql -u$MYSQL_ROOT_USER -p$MYSQL_ROOT_PASS <<EOF
CREATE USER '$TPCH_USER'@'%' IDENTIFIED BY '$TPCH_PASS';
CREATE DATABASE $TPCH_DB;
GRANT ALL ON $TPCH_USER.* to '$TPCH_USER'@'%';
EOF

# Create tables for tpc-h, with no keys/references
echo "Creating tables"
mysql -u$TPCH_USER -p$TPCH_PASS --database=$TPCH_DB < dss.ddl

# Generate data to load, overriding existing files if necessary
echo "Generating data to load"
yes | ./dbgen -s $TPCH_SCALING_FACTOR 
echo 

# Now load the data. Warning, this will take a while
echo "Now loading data- Warning, this will take a while!"
mysql -u$TPCH_USER -p$TPCH_PASS --database=$TPCH_DB --local-infile=1 <<EOF
LOAD DATA LOCAL INFILE 'customer.tbl' INTO TABLE CUSTOMER FIELDS TERMINATED BY '|';
LOAD DATA LOCAL INFILE 'orders.tbl' INTO TABLE ORDERS FIELDS TERMINATED BY '|';
LOAD DATA LOCAL INFILE 'lineitem.tbl' INTO TABLE LINEITEM FIELDS TERMINATED BY '|';
LOAD DATA LOCAL INFILE 'nation.tbl' INTO TABLE NATION FIELDS TERMINATED BY '|';
LOAD DATA LOCAL INFILE 'partsupp.tbl' INTO TABLE PARTSUPP FIELDS TERMINATED BY '|';
LOAD DATA LOCAL INFILE 'part.tbl' INTO TABLE PART FIELDS TERMINATED BY '|';
LOAD DATA LOCAL INFILE 'region.tbl' INTO TABLE REGION FIELDS TERMINATED BY '|';
LOAD DATA LOCAL INFILE 'supplier.tbl' INTO TABLE SUPPLIER FIELDS TERMINATED BY '|';
EOF

# Enforce the foreign key constraints. This will also take a long time
echo "Now enforcing FK constraints- this will also take a while!"
mysql -u$TPCH_USER -p$TPCH_PASS --database=$TPCH_DB < dss.ri

# Done!
echo "Data loaded successfully"
