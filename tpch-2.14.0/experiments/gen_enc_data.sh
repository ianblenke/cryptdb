#!/bin/bash

set -x

PWD=`pwd`

if [ `basename $PWD` != "enc-data" ]; then
    echo "Run from enc-data folder"
    exit 1
fi

CDB_TOP=$HOME/cryptdb
CDB_EXP_FOLDER=$CDB_TOP/tpch-2.14.0/experiments
BULK_ENC=$CDB_EXP_FOLDER/bulk_enc.sh
REG_ENC=$CDB_TOP/obj/parser/cdb_enc
source $CDB_EXP_FOLDER/scales.sh

for factor in $SCALES; do
    mkdir -p scale-$factor
    cd scale-$factor

    $BULK_ENC --lineitem-normal lineitem.tbl

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
