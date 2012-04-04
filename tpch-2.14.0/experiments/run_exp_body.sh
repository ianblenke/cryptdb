### Meant to be sourced by {run_exp,remote_run_exp}.sh ###

reset_exp

source $CDB_TOP/tpch-2.14.0/experiments/scales.sh

for factor in $SCALES; do

    mkdir -p scale-$factor
    cd scale-$factor
    #rm -f {orig,crypt}.*query{1,2,11,14,20}

    OPE_TYPE=new_ope
    PAR=parallel

    for i in {1..2}; do
        #/usr/bin/time -f '%e' -o orig.query1 -a $CDB_EXP --orig-query1 1998 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o orig.nosort.int.query1 -a $CDB_EXP --orig-nosort-query1 1998 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.query1 -a $CDB_EXP --crypt-opt-query1 1998 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.noopt.query1 -a $CDB_EXP --crypt-query1 1998 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.nosort.query1 -a $CDB_EXP --crypt-opt-nosort-query1 1998 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        # orig-int
        #/usr/bin/time -f '%e' -o orig.nosort.int.query1 -a $CDB_EXP --orig-nosort-int-query1 1998 1 tpch-$factor $OPE_TYPE $PAR no_parallel $EXP_HOSTNAME
        #reset_exp
        # orig-double
        #/usr/bin/time -f '%e' -o orig.nosort.double.query1 -a $CDB_EXP --orig-nosort-double-query1 1998 1 tpch-$factor $OPE_TYPE $PAR no_parallel $EXP_HOSTNAME
        #reset_exp
        ## crypt (no agg) parallel
        #/usr/bin/time -f '%e' -o crypt.nosort.parallel.query1 -a $CDB_EXP --crypt-query1 1998 1 tpch-$factor $OPE_TYPE $PAR parallel $EXP_HOSTNAME
        #reset_exp
        # crypt-nosort-agg regular
        #/usr/bin/time -f '%e' -o crypt.opt.nosort.1thd.query1 -a $CDB_EXP --crypt-opt-nosort-query1 1998 1 tpch-$factor $OPE_TYPE $PAR parallel $EXP_HOSTNAME
        #reset_exp
        # crypt-nosort-agg parallel
        #/usr/bin/time -f '%e' -o crypt.opt.nosort.parallel.query1 -a $CDB_EXP --crypt-opt-nosort-query1 1998 1 tpch-$factor $OPE_TYPE $PAR parallel $EXP_HOSTNAME
        #reset_exp
        # crypt-packed regular
        #/usr/bin/time -f '%e' -o crypt.packed.1thd.query1 -a $CDB_EXP --crypt-opt-row-col-packed-query1 1998 1 tpch-$factor $OPE_TYPE $PAR parallel $EXP_HOSTNAME
        #reset_exp
        # crypt-packed parallel
        #/usr/bin/time -f '%e' -o crypt.packed.parallel.query1 -a $CDB_EXP --crypt-opt-row-col-packed-query1 1998 1 tpch-$factor $OPE_TYPE $PAR parallel $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query1 -a $CDB_EXP --orig-nosort-int-query1 1998 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query1 -a $CDB_EXP --crypt-opt-row-col-packed-query1 1998 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query2 -a $CDB_EXP --orig-query2 36 STEEL ASIA 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query2 -a $CDB_EXP --crypt-query2 36 STEEL ASIA 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query3 -a $CDB_EXP --orig-query3 FURNITURE 1995-03-12 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query3 -a $CDB_EXP --crypt-query3 FURNITURE 1995-03-12 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query4 -a $CDB_EXP --orig-query4 1994-11-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query4 -a $CDB_EXP --crypt-query4 1994-11-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query5 -a $CDB_EXP --orig-query5 'MIDDLE EAST' 1993-01-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query5 -a $CDB_EXP --crypt-query5 'MIDDLE EAST' 1993-01-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query6 -a $CDB_EXP --orig-query6 1993-01-01 0.05 25 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query6 -a $CDB_EXP --crypt-query6 1993-01-01 0.05 25 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query7 -a $CDB_EXP --orig-query7 'SAUDI ARABIA' ARGENTINA 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query7 -a $CDB_EXP --crypt-query7 'SAUDI ARABIA' ARGENTINA 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query8 -a $CDB_EXP --orig-query8 ARGENTINA AMERICA 'PROMO ANODIZED BRASS' 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query8 -a $CDB_EXP --crypt-query8 ARGENTINA AMERICA 'PROMO ANODIZED BRASS' 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query9 -a $CDB_EXP --orig-query9 sky 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query9 -a $CDB_EXP --crypt-query9 sky 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query10 -a $CDB_EXP --orig-query10 1994-08-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query10 -a $CDB_EXP --crypt-query10 1994-08-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        #/usr/bin/time -f '%e' -o orig.query11 -a $CDB_EXP --orig-query11 ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o orig.query11.nosubquery -a $CDB_EXP --orig-query11-nosubquery ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.query11 -a $CDB_EXP --crypt-opt-query11 ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.noopt.query11 -a $CDB_EXP --crypt-query11 ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query11 -a $CDB_EXP --orig-query11-nosubquery ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.noopt.query11 -a $CDB_EXP --crypt-query11 ARGENTINA 0.0001 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query12 -a $CDB_EXP --orig-query12 AIR FOB 1993-01-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query12 -a $CDB_EXP --crypt-query12 AIR FOB 1993-01-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        #/usr/bin/time -f '%e' -o orig.query14 -a $CDB_EXP --orig-query14 1996 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.query14 -a $CDB_EXP --crypt-opt-query14 1996 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.noopt.query14 -a $CDB_EXP --crypt-query14 1996 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.opt2.query14 -a $CDB_EXP --crypt-opt2-query14 1996 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.noopt.opttables.query14 -a $CDB_EXP --crypt-opt-tables-query14 1996 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query14 -a $CDB_EXP --orig-query14 1996 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query14 -a $CDB_EXP --crypt-query14 1996 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query15 -a $CDB_EXP --orig-query15 1995-11-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query15 -a $CDB_EXP --crypt-query15 1995-11-01 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query17 -a $CDB_EXP --orig-query17 'Brand#45' 'LG BOX' 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query17 -a $CDB_EXP --crypt-query17 'Brand#45' 'LG BOX' 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query18 -a $CDB_EXP --orig-query18 315 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query18 -a $CDB_EXP --crypt-query18 315 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query20 -a $CDB_EXP --orig-query20 1997 khaki ALGERIA 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query20 -a $CDB_EXP --crypt-noagg-query20 1997 khaki ALGERIA 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        #/usr/bin/time -f '%e' -o crypt.agg.query20 -a $CDB_EXP --crypt-agg-query20 1997 khaki ALGERIA 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

        /usr/bin/time -f '%e' -o orig.query22 -a $CDB_EXP --orig-query22 '41' '26' '36' '27' '38' '37' '22' 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp
        /usr/bin/time -f '%e' -o crypt.query22 -a $CDB_EXP --crypt-query22 '41' '26' '36' '27' '38' '37' '22' 1 tpch-$factor $OPE_TYPE $PAR $EXP_HOSTNAME
        #reset_exp

    done

    cd ..

done
