#!/bin/bash

set -x
CDB_TOP=$HOME/cryptdb
CDB_EXP=$CDB_TOP/obj/parser/cdb_exp

for factor in 0.05; do
   $CDB_EXP --orig-query1 1998 1 tpch-$factor
   $CDB_EXP --crypt-opt-query1 1998 1 tpch-$factor

   $CDB_EXP --orig-query2 36 STEEL ASIA 1 tpch-$factor
   $CDB_EXP --crypt-query2 36 STEEL ASIA 1 tpch-$factor

   $CDB_EXP --orig-query11 ARGENTINA 0.0001 1 tpch-$factor
   $CDB_EXP --crypt-opt-query11 ARGENTINA 0.0001 1 tpch-$factor

   $CDB_EXP --orig-query14 1996 1 tpch-$factor
   $CDB_EXP --crypt-opt-query14 1996 1 tpch-$factor
done
