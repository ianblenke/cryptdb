#!/bin/bash

set -x
CDB_TOP=$HOME/cryptdb

mkdir -p scale-0.50
cd scale-0.50
rm -f *.enc
$CDB_TOP/tpch-2.14.0/experiments/bulk_enc.sh --lineitem-normal lineitem.tbl
cd ..
