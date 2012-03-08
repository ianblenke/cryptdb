### Meant to be sourced by {run_exp,remote_run_exp}.sh ###

reset_exp

source $CDB_TOP/tpch-2.14.0/experiments/scales.sh

for factor in $SCALES; do

    mkdir -p scale-$factor
    cd scale-$factor
    rm -f {orig,crypt}.*query{1,2,11,14,20}

    if [ $factor == "20.00" ]; then
        OPE_TYPE=new_ope
    else
        OPE_TYPE=old_ope
    fi

    for i in {1..5}; do
        /usr/bin/time -f '%e' -o orig.query1 -a $CDB_EXP --orig-query1 1998 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o orig.nosort.query1 -a $CDB_EXP --orig-nosort-query1 1998 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query1 -a $CDB_EXP --crypt-opt-query1 1998 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.noopt.query1 -a $CDB_EXP --crypt-query1 1998 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.nosort.query1 -a $CDB_EXP --crypt-opt-nosort-query1 1998 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp

        /usr/bin/time -f '%e' -o orig.query2 -a $CDB_EXP --orig-query2 36 STEEL ASIA 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query2 -a $CDB_EXP --crypt-query2 36 STEEL ASIA 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp

        /usr/bin/time -f '%e' -o orig.query11 -a $CDB_EXP --orig-query11 ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o orig.query11.nosubquery -a $CDB_EXP --orig-query11-nosubquery ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query11 -a $CDB_EXP --crypt-opt-query11 ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.noopt.query11 -a $CDB_EXP --crypt-query11 ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp

        /usr/bin/time -f '%e' -o orig.query14 -a $CDB_EXP --orig-query14 1996 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query14 -a $CDB_EXP --crypt-opt-query14 1996 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.noopt.query14 -a $CDB_EXP --crypt-query14 1996 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.noopt.opttables.query14 -a $CDB_EXP --crypt-opt-tables-query14 1996 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp

        /usr/bin/time -f '%e' -o orig.query18 -a $CDB_EXP --orig-query18 315 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.query18 -a $CDB_EXP --crypt-query18 315 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp

        /usr/bin/time -f '%e' -o orig.query20 -a $CDB_EXP --orig-query20 1997 khaki ALGERIA 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.noagg.query20 -a $CDB_EXP --crypt-noagg-query20 1997 khaki ALGERIA 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp
        /usr/bin/time -f '%e' -o crypt.agg.query20 -a $CDB_EXP --crypt-agg-query20 1997 khaki ALGERIA 1 tpch-$factor $OPE_TYPE $EXP_HOSTNAME
        reset_exp

    done

    cd ..

done
