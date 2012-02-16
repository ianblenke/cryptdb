#!/bin/bash

# must run from results folder
PWD=`pwd`

if [ `basename $PWD` != "results" ]; then
    echo "Run from results folder"
    exit 1
fi

if [ ! -x $HOME/start_mysql.sh ]; then
    echo "No start_mysql.sh file found in homedir"
    exit 1
fi

if [ $# -eq 0 ]; then
    stty -echo
    read -p "sudo password: " PASSWORD; echo
    stty echo
else
    PASSWORD=$1
fi

set -x
CDB_TOP=$HOME/cryptdb
CDB_EXP=$CDB_TOP/obj/parser/cdb_exp
MYSQL_SOCK_FILE=/tmp/mysql.sock

reset_exp() {
    # shut mysql down
    mysqladmin --socket=$MYSQL_SOCK_FILE -uroot shutdown

    # clear linux disk cache
    sync
    set +x
    echo $PASSWORD | sudo -S sh -c 'echo 1 > /proc/sys/vm/drop_caches'
    echo $PASSWORD | sudo -S sh -c 'echo 3 > /proc/sys/vm/drop_caches'
    set -x

    #start up mysql again
    $HOME/start_mysql.sh &

    # poll for the sock file to exist
    while [ ! -e $MYSQL_SOCK_FILE ]; do
        echo "waiting for $MYSQL_SOCK_FILE to appear..."
        sleep 1
    done

    sleep 2 # wait some more to give mysql time to initialize
}

EXP_HOSTNAME=localhost
source $CDB_TOP/tpch-2.14.0/experiments/run_exp_body.sh
