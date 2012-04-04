#!/bin/bash
set -x
set -e

HOSTS=("local" "vise5.csail.mit.edu" "stephentu-vm1.csail.mit.edu" "stephentu-vm2.csail.mit.edu")
NHOSTS=${#HOSTS[@]}
CDB_TOP=$HOME/cryptdb

NPROCS=24 # procs per host. assume homogeneous
if [ $# -ne 2 ]; then 
    echo "[USAGE]: $0 [flag] [filename]"
    exit 1
fi
MULTIPLE=12
FLAG=$1
FNAME=$2
if [ ! -f $FNAME ]; then
    echo "No such file $FNAME"
    exit 1
fi

NUM_ENTRIES=`wc -l $FNAME | awk '{ print $1; }'`
LINES_PER_FILE=$(( $NUM_ENTRIES / ($NPROCS * $NHOSTS) ))
if [ $LINES_PER_FILE -eq 0 ]; then
    LINES_PER_FILE=1
fi
LINES_PER_FILE=`python $CDB_TOP/tpch-2.14.0/experiments/round.py $LINES_PER_FILE $MULTIPLE`

rm -f ${FNAME}_*
split -d -l $LINES_PER_FILE $FNAME ${FNAME}_
PIDS=()
FILENAMES=()
i=0
for fname in ${FNAME}_*; do
    IDX=$(( $i % $NHOSTS))
    HOST=${HOSTS[$IDX]}

    if [ $HOST == "local" ]; then
        $CDB_TOP/obj/parser/cdb_enc $FLAG < $fname > ${fname}.enc &
        PIDS+=($!)
    else 
        # copy the file
        ssh $HOST 'mkdir -p /tmp/cryptdb'
        scp $fname $HOST:/tmp/cryptdb

        # run the job remotely
        REMOTE_NAME=/tmp/cryptdb/$fname
        cat <<EOF | ssh $HOST '/bin/bash' &
$CDB_TOP/obj/parser/cdb_enc $FLAG < $REMOTE_NAME > ${REMOTE_NAME}.enc
EOF
        PIDS+=($!)
    fi

    FILENAMES+=($fname)

    i=$(( $i + 1 ))
done

set +e
for pid in "${PIDS[@]}"; do
    wait $pid
done
set -e

i=0
for fname in "${FILENAMES[@]}"; do
    IDX=$(( $i % $NHOSTS))
    HOST=${HOSTS[$IDX]}
    if [ $HOST != "local" ]; then
        # copy the remote file back locally
        ENC_NAME=$fname.enc
        REMOTE_NAME=/tmp/cryptdb/$ENC_NAME
        scp $HOST:$REMOTE_NAME $ENC_NAME
    fi
    i=$(( $i + 1 ))
done

cat ${FNAME}_*.enc > ${FNAME}.enc
rm -f ${FNAME}_*
