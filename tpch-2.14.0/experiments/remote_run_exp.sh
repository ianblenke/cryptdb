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
PSQL_SOCK_FILE=/var/run/postgresql/.s.PGSQL.5432

source $CDB_TOP/tpch-2.14.0/experiments/scales.sh

reset_exp() {
    if [ $DB == "mysql" ]; then 
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
    else
        cat <<EOF | ssh $REMOTE_HOST '/bin/bash' 
        # shut postgres down
        set +x
        echo $PASSWORD | sudo -S su postgres -c '/usr/lib/postgresql/8.4/bin/pg_ctl stop -m fast -D /space/postgres/db'
        set -x

        # clear linux disk cache
        sync
        set +x
        echo $PASSWORD | sudo -S sh -c 'echo 1 > /proc/sys/vm/drop_caches'
        echo $PASSWORD | sudo -S sh -c 'echo 3 > /proc/sys/vm/drop_caches'
        set -x

        set +x
        echo $PASSWORD | sudo -S su postgres -c '/usr/lib/postgresql/8.4/bin/pg_ctl start -D /space/postgres/db -o "-c config_file=/space/postgres/postgresql.conf"' < /dev/null >& /dev/null
        set -x

        # poll for the sock file to exist
        while [ ! -e $PSQL_SOCK_FILE ]; do
            echo "waiting for $PSQL_SOCK_FILE to appear..."
            sleep 1
        done

        sleep 2 # wait some more to give postgres time to initialize
EOF
    fi
}

# throttle_bw_{start,stop} assume that sudo on remote machine
# and local machine can be done w/ no passwd

throttle_bw_start() {
    cat <<EOF | ssh $REMOTE_HOST 'sudo /bin/bash'
    tc qdisc del dev eth0 root
    tc qdisc add dev eth0 root handle 1: htb default 2
    tc class add dev eth0 parent 1: classid 1:1 htb rate 10mbit
    tc class add dev eth0 parent 1: classid 1:2 htb rate 10mbit
    tc qdisc show dev eth0
EOF
    cat <<EOF | sudo su -c /bin/bash 
    tc qdisc del dev eth0 root 
    tc qdisc add dev eth0 root handle 1: htb default 2
    tc class add dev eth0 parent 1: classid 1:1 htb rate 10mbit  
    tc class add dev eth0 parent 1: classid 1:2 htb rate 10mbit 
    tc qdisc show dev eth0
EOF
}

throttle_bw_end() {
    cat <<EOF | ssh $REMOTE_HOST 'sudo /bin/bash'
    tc qdisc del dev eth0 root
    tc qdisc show dev eth0
EOF
    cat <<EOF | sudo su -c /bin/bash 
    tc qdisc del dev eth0 root 
    tc qdisc show dev eth0
EOF
}

start_compression_channel() {
    # TODO: what compression level?
    ssh -f -N -c blowfish -C -L 10000:localhost:5432 $REMOTE_HOST
}

end_compression_channel() {
    kill `ps aux | grep 'ssh -f -N' | grep 10000 | grep -v grep | awk '{print $2}'`
}

EXP_HOSTNAME=$REMOTE_HOST
source $CDB_TOP/tpch-2.14.0/experiments/run_program.sh
