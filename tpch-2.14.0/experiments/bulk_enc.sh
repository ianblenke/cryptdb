#!/bin/bash
set -x
CDB_TOP=$HOME/cryptdb
NPROCS=24
if [ $# -ne 2 ]; then 
    echo "[USAGE]: $0 [flag] [filename]"
    exit 1
fi
FLAG=$1
FNAME=$2
if [ ! -f $FNAME ]; then
    echo "No such file $FNAME"
    exit 1
fi
NUM_ENTRIES=`wc -l $FNAME | awk '{ print $1; }'`
LINES_PER_FILE=$(( $NUM_ENTRIES / $NPROCS ))
if [ $LINES_PER_FILE -eq 0 ]; then
    LINES_PER_FILE=1
fi
LINES_PER_FILE=`python $CDB_TOP/tpch-2.14.0/experiments/round.py $LINES_PER_FILE 12`
rm -f ${FNAME}_*
split -d -l $LINES_PER_FILE $FNAME ${FNAME}_
PIDS=()
for i in ${FNAME}_*; do
    $CDB_TOP/obj/parser/cdb_enc $FLAG < $i > ${i}.enc &
    PIDS+=($!)
done
for pid in "${PIDS[@]}"; do
    wait $pid
done
cat ${FNAME}_*.enc > ${FNAME}.enc
rm -f ${FNAME}_*
