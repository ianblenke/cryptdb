#!/bin/bash

NPROCS=24
if [ $# -ne 1 ]; then 
    echo "[USAGE]: $0 [filename]"
    exit 1
fi
FNAME=$1
if [ ! -f $FNAME ]; then
    echo "No such file $FNAME"
    exit 1
fi
NUM_ENTRIES=`wc -l $FNAME | awk '{ print $1; }'`
LINES_PER_FILE=$(( $NUM_ENTRIES / $NPROCS ))
if [ $LINES_PER_FILE -eq 0 ]; then
    LINES_PER_FILE=1
fi
split -d -l $LINES_PER_FILE $FNAME ${FNAME}_
for i in ${FNAME}_*; do
    obj/parser/cdb_enc < $i > ${i}.enc &
done
cat ${FNAME}_*.enc > ${FNAME}.enc
