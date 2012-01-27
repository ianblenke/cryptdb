#!/bin/bash

# the big button
# MUST RUN FROM THE TOP OF THE cryptdb SOURCE TREE

stty -echo
read -p "sudo password: " PASSWORD; echo
stty echo

set -x

PWD=`pwd`
MYSQL_SOCK=/tmp/mysql.sock

if [ `basename $PWD` != "cryptdb" ]; then
    echo "Run from cryptdb folder"
    exit 1
fi

# test sudo password
set +x
echo $PASSWORD | sudo -S sh -c 'echo testing sudo password'
set -x

CDB_TOP=$HOME/cryptdb
CDB_EXP_FOLDER=$CDB_TOP/tpch-2.14.0/experiments

# generate all the datas
cd $CDB_TOP/enc-data
#$CDB_EXP_FOLDER/gen_data.sh
cd $CDB_TOP

# encrypt all the datas
cd $CDB_TOP/enc-data
$CDB_EXP_FOLDER/gen_enc_data.sh
cd $CDB_TOP

# load all the datas
echo '\. load.sql' | mysql -uroot --socket=$MYSQL_SOCK

# run the exps
cd $CDB_TOP/results
set +x
$CDB_EXP_FOLDER/run_exp.sh $PASSWORD
set -x
cd $CDB_TOP
