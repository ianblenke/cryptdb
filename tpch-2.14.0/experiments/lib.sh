#!/bin/bash

PSQL_SOCK_FILE=/var/run/postgresql/.s.PGSQL.5432

reset_exp() {
    REMOTE_HOST=$1
    cat <<EOF | ssh $REMOTE_HOST '/bin/bash' 
        # shut postgres down
        sudo su postgres -c '/usr/lib/postgresql/8.4/bin/pg_ctl stop -m fast -D /space/postgres/db'

        # clear linux disk cache
        sync
        sudo sh -c 'echo 1 > /proc/sys/vm/drop_caches'
        sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'

        sudo -S su postgres -c '/usr/lib/postgresql/8.4/bin/pg_ctl start -D /space/postgres/db -o "-c config_file=/space/postgres/postgresql.conf"' < /dev/null >& /dev/null

        # poll for the sock file to exist
        while [ ! -e $PSQL_SOCK_FILE ]; do
            echo "waiting for $PSQL_SOCK_FILE to appear..."
            sleep 1
        done

        sleep 2 # wait some more to give postgres time to initialize
EOF
}

# throttle_bw_{start,stop} assume that sudo on remote machine
# and local machine can be done w/ no passwd

throttle_bw_start() {
    REMOTE_HOST=$1
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
    REMOTE_HOST=$1
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
    REMOTE_HOST=$1
    # TODO: what compression level?
    ssh -f -N -c blowfish -C -L 10000:localhost:5432 $REMOTE_HOST
}

end_compression_channel() {
    kill `ps aux | grep 'ssh -f -N' | grep 10000 | grep -v grep | awk '{print $2}'`
}
