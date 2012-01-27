#!/bin/bash

stty -echo
read -p "sudo password: " PASSWORD; echo
stty echo

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

reset_exp

for factor in 0.50; do

    mkdir -p scale-$factor
    cd scale-$factor
    rm -f {orig,crypt}.query{1,2,11,14}

    for i in {1..5}; do
        /usr/bin/time -f '%e' -o orig.query1 -a $CDB_EXP --orig-query1 1998 1 tpch-$factor
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query1 -a $CDB_EXP --crypt-opt-query1 1998 1 tpch-$factor
        reset_exp

        /usr/bin/time -f '%e' -o orig.query2 -a $CDB_EXP --orig-query2 36 STEEL ASIA 1 tpch-$factor
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query2 -a $CDB_EXP --crypt-query2 36 STEEL ASIA 1 tpch-$factor
        reset_exp

        /usr/bin/time -f '%e' -o orig.query11 -a $CDB_EXP --orig-query11 ARGENTINA 0.0001 1 tpch-$factor
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query11 -a $CDB_EXP --crypt-opt-query11 ARGENTINA 0.0001 1 tpch-$factor
        reset_exp

        /usr/bin/time -f '%e' -o orig.query14 -a $CDB_EXP --orig-query14 1996 1 tpch-$factor
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query14 -a $CDB_EXP --crypt-opt-query14 1996 1 tpch-$factor
        reset_exp
    done

    cd ..

done
