#!/bin/bash

set -x

PWD=`pwd`

if [ `basename $PWD` != "enc-data" ]; then
    echo "Run from enc-data folder"
    exit 1
fi

CDB_TOP=$HOME/cryptdb
CDB_EXP_FOLDER=$CDB_TOP/tpch-2.14.0/experiments
#BULK_ENC=$CDB_EXP_FOLDER/bulk_enc.sh
BULK_ENC=$CDB_EXP_FOLDER/dist_bulk_enc.sh
REG_ENC=$CDB_TOP/obj/parser/cdb_enc
SORTER=$CDB_TOP/obj/parser/sorter
PG_BINFMT=$CDB_TOP/obj/parser/pg_binfmt

source $CDB_EXP_FOLDER/scales.sh

for factor in $SCALES; do
    mkdir -p scale-$factor
    cd scale-$factor

    #$BULK_ENC --lineitem-normal lineitem.tbl

    ## This file is only 25 lines long (regardless of scale)
    ## so use reg enc
    #$REG_ENC --nation-none < nation.tbl > nation.tbl.enc

    #$BULK_ENC --part-none part.tbl
    #$BULK_ENC --partsupp-normal partsupp.tbl

    ## This file is only 5 lines long (regardless of scale)
    ## so use reg enc
    #$REG_ENC --region-none < region.tbl > region.tbl.enc

    #$BULK_ENC --supplier-none supplier.tbl

    #$BULK_ENC --orders-none orders.tbl

    #$BULK_ENC --customer-none customer.tbl

    # TODO: generate the packed-agg files

    # sort
    $SORTER --interpret-as-int 0 3 < lineitem.tbl.enc > lineitem.tbl.enc.sorted
    $SORTER --interpret-as-int 0 < nation.tbl.enc > nation.tbl.enc.sorted
    $SORTER --interpret-as-int 0 < part.tbl.enc > part.tbl.enc.sorted
    $SORTER --interpret-as-int 0 1 < partsupp.tbl.enc > partsupp.tbl.enc.sorted
    $SORTER --interpret-as-int 0 < region.tbl.enc > region.tbl.enc.sorted
    $SORTER --interpret-as-int 0 < supplier.tbl.enc > supplier.tbl.enc.sorted
    $SORTER --interpret-as-int 0 < orders.tbl.enc > orders.tbl.enc.sorted
    $SORTER --interpret-as-int 0 < customer.tbl.enc > customer.tbl.enc.sorted

    # generate the postgres version
    $PG_BINFMT iiiilbllblssilililbbbsl  < lineitem.tbl.enc.sorted > lineitem.tbl.enc.sorted.pg
    $PG_BINFMT iblib < nation.tbl.enc.sorted > nation.tbl.enc.sorted.pg
    $PG_BINFMT ilbbbbbbilblb < part.tbl.enc.sorted > part.tbl.enc.sorted.pg
    $PG_BINFMT iiilbb < partsupp.tbl.enc.sorted > partsupp.tbl.enc.sorted.pg
    $PG_BINFMT ibb < region.tbl.enc.sorted > region.tbl.enc.sorted.pg
    $PG_BINFMT iblbiblbb < supplier.tbl.enc.sorted > supplier.tbl.enc.sorted.pg
    $PG_BINFMT iislbilbbibs < orders.tbl.enc.sorted > orders.tbl.enc.sorted.pg
    $PG_BINFMT ibbiblbbbb < customer.tbl.enc.sorted > customer.tbl.enc.sorted.pg

    cd ..
done
