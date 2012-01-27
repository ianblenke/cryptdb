#!/bin/bash

set -x

PWD=`pwd`

if [ `basename $PWD` != "enc-data" ]; then
    echo "Run from enc-data folder"
    exit 1
fi

CDB_TOP=$HOME/cryptdb
BULK_ENC=$CDB_TOP/tpch-2.14.0/experiments/bulk_enc.sh
REG_ENC=$CDB_TOP/obj/parser/cdb_enc

#for factor in 0.05 0.25 0.50 0.75; do
for factor in 0.50; do
    mkdir -p scale-$factor
    cd scale-$factor
    #rm -f *.enc

    #$BULK_ENC --lineitem-normal lineitem.tbl

    # This file is only 25 lines long (regardless of scale)
    # so use reg enc
    $REG_ENC --nation-none < nation.tbl > nation.tbl.enc

    $BULK_ENC --part-none part.tbl
    $BULK_ENC --partsupp-normal partsupp.tbl

    # This file is only 5 lines long (regardless of scale)
    # so use reg enc
    $REG_ENC --region-none < region.tbl > region.tbl.enc

    $BULK_ENC --supplier-none supplier.tbl

    cd ..
done
