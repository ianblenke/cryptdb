#!/bin/bash

# must run from results folder
PWD=`pwd`

if [ $# -ne 1 -a $# -ne 2 ]; then
    echo "need one or two args"
    exit 1
fi

REMOTE_HOST=$1

if [ `basename $PWD` != "results" ]; then
    echo "Run from results folder"
    exit 1
fi

ssh $REMOTE_HOST "test -x $HOME/start_mysql.sh"
if [ ! $? ]; then
    echo "No start_mysql.sh file found in homedir"
    exit 1
fi

if [ $# -eq 1 ]; then
    stty -echo
    read -p "sudo password: " PASSWORD; echo
    stty echo
else
    PASSWORD=$2
fi

set -x
CDB_TOP=$HOME/cryptdb
CDB_EXP=$CDB_TOP/obj/parser/cdb_exp
MYSQL_SOCK_FILE=/tmp/mysql.sock

reset_exp() {
    cat <<EOF | ssh $REMOTE_HOST '/bin/bash' 
    # shut mysql down
    mysqladmin --socket=$MYSQL_SOCK_FILE -uroot shutdown

    # clear linux disk cache
    sync
    set +x
    echo $PASSWORD | sudo -S sh -c 'echo 1 > /proc/sys/vm/drop_caches'
    echo $PASSWORD | sudo -S sh -c 'echo 3 > /proc/sys/vm/drop_caches'
    set -x

    # start up mysql again
    # see http://www.snailbook.com/faq/background-jobs.auto.html
    # for an explanation of the redirects
    $HOME/start_mysql.sh < /dev/null >& /dev/null &

    # poll for the sock file to exist
    while [ ! -e $MYSQL_SOCK_FILE ]; do
        echo "waiting for $MYSQL_SOCK_FILE to appear..."
        sleep 1
    done

    sleep 2 # wait some more to give mysql time to initialize
EOF
}

EXP_HOSTNAME=$REMOTE_HOST
source $CDB_TOP/tpch-2.14.0/experiments/run_exp_body.sh
