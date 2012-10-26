#!/bin/bash
REMOTE_HOST=$1
RESULTS_DIR=$2
shift
shift
PROG_ARGS="--hostname localhost --port 10000"
source /home/stephentu/cryptdb/tpch-2.14.0/experiments/lib.sh
mkdir -p $RESULTS_DIR/benchmark-0
mkdir -p $RESULTS_DIR/benchmark-1
mkdir -p $RESULTS_DIR/benchmark-2
mkdir -p $RESULTS_DIR/benchmark-3
mkdir -p $RESULTS_DIR/benchmark-4
mkdir -p $RESULTS_DIR/benchmark-5
mkdir -p $RESULTS_DIR/benchmark-6
mkdir -p $RESULTS_DIR/benchmark-7
mkdir -p $RESULTS_DIR/benchmark-8
mkdir -p $RESULTS_DIR/benchmark-9
mkdir -p $RESULTS_DIR/benchmark-10
mkdir -p $RESULTS_DIR/benchmark-11
mkdir -p $RESULTS_DIR/benchmark-12
mkdir -p $RESULTS_DIR/benchmark-13
mkdir -p $RESULTS_DIR/benchmark-14
mkdir -p $RESULTS_DIR/benchmark-15
mkdir -p $RESULTS_DIR/benchmark-16
mkdir -p $RESULTS_DIR/benchmark-17
mkdir -p $RESULTS_DIR/benchmark-18
throttle_bw_end $REMOTE_HOST
end_compression_channel
throttle_bw_start $REMOTE_HOST
start_compression_channel $REMOTE_HOST
reset_exp $REMOTE_HOST
for i in {1..3}; do
# benchmark 0
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q0.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 0 2>&1 | tee $RESULTS_DIR/benchmark-0/q0.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q0.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q1.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 1 2>&1 | tee $RESULTS_DIR/benchmark-0/q1.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q1.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q2.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 2 2>&1 | tee $RESULTS_DIR/benchmark-0/q2.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q2.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q3.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 3 2>&1 | tee $RESULTS_DIR/benchmark-0/q3.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q3.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q4.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 4 2>&1 | tee $RESULTS_DIR/benchmark-0/q4.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q4.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q5.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 5 2>&1 | tee $RESULTS_DIR/benchmark-0/q5.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q5.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q6.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 6 2>&1 | tee $RESULTS_DIR/benchmark-0/q6.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q6.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q7.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 7 2>&1 | tee $RESULTS_DIR/benchmark-0/q7.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q7.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q8.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 8 2>&1 | tee $RESULTS_DIR/benchmark-0/q8.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q8.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q9.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 9 2>&1 | tee $RESULTS_DIR/benchmark-0/q9.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q9.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q10.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 10 2>&1 | tee $RESULTS_DIR/benchmark-0/q10.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q10.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q11.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 11 2>&1 | tee $RESULTS_DIR/benchmark-0/q11.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q11.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q12.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 12 2>&1 | tee $RESULTS_DIR/benchmark-0/q12.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q12.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q13.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 13 2>&1 | tee $RESULTS_DIR/benchmark-0/q13.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q13.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q14.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 14 2>&1 | tee $RESULTS_DIR/benchmark-0/q14.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q14.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q15.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 15 2>&1 | tee $RESULTS_DIR/benchmark-0/q15.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q15.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q16.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 16 2>&1 | tee $RESULTS_DIR/benchmark-0/q16.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q16.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q17.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 17 2>&1 | tee $RESULTS_DIR/benchmark-0/q17.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q17.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-0/q18.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-0/program $PROG_ARGS 18 2>&1 | tee $RESULTS_DIR/benchmark-0/q18.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-0/q18.out
fi
reset_exp $REMOTE_HOST

# benchmark 1
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q0.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 0 2>&1 | tee $RESULTS_DIR/benchmark-1/q0.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q0.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q1.out $RESULTS_DIR/benchmark-1/q1.out
cp $RESULTS_DIR/benchmark-0/q1.time $RESULTS_DIR/benchmark-1/q1.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q2.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 2 2>&1 | tee $RESULTS_DIR/benchmark-1/q2.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q2.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q3.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 3 2>&1 | tee $RESULTS_DIR/benchmark-1/q3.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q3.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q4.out $RESULTS_DIR/benchmark-1/q4.out
cp $RESULTS_DIR/benchmark-0/q4.time $RESULTS_DIR/benchmark-1/q4.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q5.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 5 2>&1 | tee $RESULTS_DIR/benchmark-1/q5.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q5.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q6.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 6 2>&1 | tee $RESULTS_DIR/benchmark-1/q6.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q6.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q7.out $RESULTS_DIR/benchmark-1/q7.out
cp $RESULTS_DIR/benchmark-0/q7.time $RESULTS_DIR/benchmark-1/q7.time
cp $RESULTS_DIR/benchmark-0/q8.out $RESULTS_DIR/benchmark-1/q8.out
cp $RESULTS_DIR/benchmark-0/q8.time $RESULTS_DIR/benchmark-1/q8.time
cp $RESULTS_DIR/benchmark-0/q9.out $RESULTS_DIR/benchmark-1/q9.out
cp $RESULTS_DIR/benchmark-0/q9.time $RESULTS_DIR/benchmark-1/q9.time
cp $RESULTS_DIR/benchmark-0/q10.out $RESULTS_DIR/benchmark-1/q10.out
cp $RESULTS_DIR/benchmark-0/q10.time $RESULTS_DIR/benchmark-1/q10.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q11.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 11 2>&1 | tee $RESULTS_DIR/benchmark-1/q11.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q11.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q12.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 12 2>&1 | tee $RESULTS_DIR/benchmark-1/q12.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q12.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-1/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-1/q13.time
cp $RESULTS_DIR/benchmark-0/q14.out $RESULTS_DIR/benchmark-1/q14.out
cp $RESULTS_DIR/benchmark-0/q14.time $RESULTS_DIR/benchmark-1/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-1/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-1/q15.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q16.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 16 2>&1 | tee $RESULTS_DIR/benchmark-1/q16.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q16.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-1/q17.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-1/program $PROG_ARGS 17 2>&1 | tee $RESULTS_DIR/benchmark-1/q17.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-1/q17.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q18.out $RESULTS_DIR/benchmark-1/q18.out
cp $RESULTS_DIR/benchmark-0/q18.time $RESULTS_DIR/benchmark-1/q18.time

# benchmark 2
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-2/q0.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-2/program $PROG_ARGS 0 2>&1 | tee $RESULTS_DIR/benchmark-2/q0.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-2/q0.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q1.out $RESULTS_DIR/benchmark-2/q1.out
cp $RESULTS_DIR/benchmark-0/q1.time $RESULTS_DIR/benchmark-2/q1.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-2/q2.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-2/program $PROG_ARGS 2 2>&1 | tee $RESULTS_DIR/benchmark-2/q2.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-2/q2.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-2/q3.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-2/program $PROG_ARGS 3 2>&1 | tee $RESULTS_DIR/benchmark-2/q3.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-2/q3.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-2/q4.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-2/program $PROG_ARGS 4 2>&1 | tee $RESULTS_DIR/benchmark-2/q4.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-2/q4.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-1/q5.out $RESULTS_DIR/benchmark-2/q5.out
cp $RESULTS_DIR/benchmark-1/q5.time $RESULTS_DIR/benchmark-2/q5.time
cp $RESULTS_DIR/benchmark-1/q6.out $RESULTS_DIR/benchmark-2/q6.out
cp $RESULTS_DIR/benchmark-1/q6.time $RESULTS_DIR/benchmark-2/q6.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-2/q7.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-2/program $PROG_ARGS 7 2>&1 | tee $RESULTS_DIR/benchmark-2/q7.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-2/q7.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q8.out $RESULTS_DIR/benchmark-2/q8.out
cp $RESULTS_DIR/benchmark-0/q8.time $RESULTS_DIR/benchmark-2/q8.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-2/q9.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-2/program $PROG_ARGS 9 2>&1 | tee $RESULTS_DIR/benchmark-2/q9.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-2/q9.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q10.out $RESULTS_DIR/benchmark-2/q10.out
cp $RESULTS_DIR/benchmark-0/q10.time $RESULTS_DIR/benchmark-2/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-2/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-2/q11.time
cp $RESULTS_DIR/benchmark-1/q12.out $RESULTS_DIR/benchmark-2/q12.out
cp $RESULTS_DIR/benchmark-1/q12.time $RESULTS_DIR/benchmark-2/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-2/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-2/q13.time
cp $RESULTS_DIR/benchmark-0/q14.out $RESULTS_DIR/benchmark-2/q14.out
cp $RESULTS_DIR/benchmark-0/q14.time $RESULTS_DIR/benchmark-2/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-2/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-2/q15.time
cp $RESULTS_DIR/benchmark-1/q16.out $RESULTS_DIR/benchmark-2/q16.out
cp $RESULTS_DIR/benchmark-1/q16.time $RESULTS_DIR/benchmark-2/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-2/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-2/q17.time
cp $RESULTS_DIR/benchmark-0/q18.out $RESULTS_DIR/benchmark-2/q18.out
cp $RESULTS_DIR/benchmark-0/q18.time $RESULTS_DIR/benchmark-2/q18.time

# benchmark 3
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-3/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-3/q0.time
cp $RESULTS_DIR/benchmark-0/q1.out $RESULTS_DIR/benchmark-3/q1.out
cp $RESULTS_DIR/benchmark-0/q1.time $RESULTS_DIR/benchmark-3/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-3/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-3/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-3/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-3/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-3/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-3/q4.time
cp $RESULTS_DIR/benchmark-1/q5.out $RESULTS_DIR/benchmark-3/q5.out
cp $RESULTS_DIR/benchmark-1/q5.time $RESULTS_DIR/benchmark-3/q5.time
cp $RESULTS_DIR/benchmark-1/q6.out $RESULTS_DIR/benchmark-3/q6.out
cp $RESULTS_DIR/benchmark-1/q6.time $RESULTS_DIR/benchmark-3/q6.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-3/q7.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-3/program $PROG_ARGS 7 2>&1 | tee $RESULTS_DIR/benchmark-3/q7.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-3/q7.out
fi
reset_exp $REMOTE_HOST
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-3/q8.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-3/program $PROG_ARGS 8 2>&1 | tee $RESULTS_DIR/benchmark-3/q8.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-3/q8.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-3/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-3/q9.time
cp $RESULTS_DIR/benchmark-0/q10.out $RESULTS_DIR/benchmark-3/q10.out
cp $RESULTS_DIR/benchmark-0/q10.time $RESULTS_DIR/benchmark-3/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-3/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-3/q11.time
cp $RESULTS_DIR/benchmark-1/q12.out $RESULTS_DIR/benchmark-3/q12.out
cp $RESULTS_DIR/benchmark-1/q12.time $RESULTS_DIR/benchmark-3/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-3/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-3/q13.time
cp $RESULTS_DIR/benchmark-0/q14.out $RESULTS_DIR/benchmark-3/q14.out
cp $RESULTS_DIR/benchmark-0/q14.time $RESULTS_DIR/benchmark-3/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-3/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-3/q15.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-3/q16.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-3/program $PROG_ARGS 16 2>&1 | tee $RESULTS_DIR/benchmark-3/q16.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-3/q16.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-3/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-3/q17.time
cp $RESULTS_DIR/benchmark-0/q18.out $RESULTS_DIR/benchmark-3/q18.out
cp $RESULTS_DIR/benchmark-0/q18.time $RESULTS_DIR/benchmark-3/q18.time

# benchmark 4
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-4/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-4/q0.time
cp $RESULTS_DIR/benchmark-0/q1.out $RESULTS_DIR/benchmark-4/q1.out
cp $RESULTS_DIR/benchmark-0/q1.time $RESULTS_DIR/benchmark-4/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-4/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-4/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-4/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-4/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-4/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-4/q4.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-4/q5.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-4/program $PROG_ARGS 5 2>&1 | tee $RESULTS_DIR/benchmark-4/q5.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-4/q5.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-1/q6.out $RESULTS_DIR/benchmark-4/q6.out
cp $RESULTS_DIR/benchmark-1/q6.time $RESULTS_DIR/benchmark-4/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-4/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-4/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-4/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-4/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-4/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-4/q9.time
cp $RESULTS_DIR/benchmark-0/q10.out $RESULTS_DIR/benchmark-4/q10.out
cp $RESULTS_DIR/benchmark-0/q10.time $RESULTS_DIR/benchmark-4/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-4/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-4/q11.time
cp $RESULTS_DIR/benchmark-1/q12.out $RESULTS_DIR/benchmark-4/q12.out
cp $RESULTS_DIR/benchmark-1/q12.time $RESULTS_DIR/benchmark-4/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-4/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-4/q13.time
cp $RESULTS_DIR/benchmark-0/q14.out $RESULTS_DIR/benchmark-4/q14.out
cp $RESULTS_DIR/benchmark-0/q14.time $RESULTS_DIR/benchmark-4/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-4/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-4/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-4/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-4/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-4/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-4/q17.time
cp $RESULTS_DIR/benchmark-0/q18.out $RESULTS_DIR/benchmark-4/q18.out
cp $RESULTS_DIR/benchmark-0/q18.time $RESULTS_DIR/benchmark-4/q18.time

# benchmark 5
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-5/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-5/q0.time
cp $RESULTS_DIR/benchmark-0/q1.out $RESULTS_DIR/benchmark-5/q1.out
cp $RESULTS_DIR/benchmark-0/q1.time $RESULTS_DIR/benchmark-5/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-5/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-5/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-5/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-5/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-5/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-5/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-5/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-5/q5.time
cp $RESULTS_DIR/benchmark-1/q6.out $RESULTS_DIR/benchmark-5/q6.out
cp $RESULTS_DIR/benchmark-1/q6.time $RESULTS_DIR/benchmark-5/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-5/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-5/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-5/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-5/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-5/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-5/q9.time
cp $RESULTS_DIR/benchmark-0/q10.out $RESULTS_DIR/benchmark-5/q10.out
cp $RESULTS_DIR/benchmark-0/q10.time $RESULTS_DIR/benchmark-5/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-5/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-5/q11.time
cp $RESULTS_DIR/benchmark-1/q12.out $RESULTS_DIR/benchmark-5/q12.out
cp $RESULTS_DIR/benchmark-1/q12.time $RESULTS_DIR/benchmark-5/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-5/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-5/q13.time
cp $RESULTS_DIR/benchmark-0/q14.out $RESULTS_DIR/benchmark-5/q14.out
cp $RESULTS_DIR/benchmark-0/q14.time $RESULTS_DIR/benchmark-5/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-5/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-5/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-5/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-5/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-5/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-5/q17.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-5/q18.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-5/program $PROG_ARGS 18 2>&1 | tee $RESULTS_DIR/benchmark-5/q18.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-5/q18.out
fi
reset_exp $REMOTE_HOST

# benchmark 6
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-6/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-6/q0.time
cp $RESULTS_DIR/benchmark-0/q1.out $RESULTS_DIR/benchmark-6/q1.out
cp $RESULTS_DIR/benchmark-0/q1.time $RESULTS_DIR/benchmark-6/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-6/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-6/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-6/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-6/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-6/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-6/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-6/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-6/q5.time
cp $RESULTS_DIR/benchmark-1/q6.out $RESULTS_DIR/benchmark-6/q6.out
cp $RESULTS_DIR/benchmark-1/q6.time $RESULTS_DIR/benchmark-6/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-6/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-6/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-6/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-6/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-6/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-6/q9.time
cp $RESULTS_DIR/benchmark-0/q10.out $RESULTS_DIR/benchmark-6/q10.out
cp $RESULTS_DIR/benchmark-0/q10.time $RESULTS_DIR/benchmark-6/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-6/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-6/q11.time
cp $RESULTS_DIR/benchmark-1/q12.out $RESULTS_DIR/benchmark-6/q12.out
cp $RESULTS_DIR/benchmark-1/q12.time $RESULTS_DIR/benchmark-6/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-6/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-6/q13.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-6/q14.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-6/program $PROG_ARGS 14 2>&1 | tee $RESULTS_DIR/benchmark-6/q14.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-6/q14.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-6/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-6/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-6/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-6/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-6/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-6/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-6/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-6/q18.time

# benchmark 7
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-7/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-7/q0.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-7/q1.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-7/program $PROG_ARGS 1 2>&1 | tee $RESULTS_DIR/benchmark-7/q1.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-7/q1.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-7/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-7/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-7/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-7/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-7/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-7/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-7/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-7/q5.time
cp $RESULTS_DIR/benchmark-1/q6.out $RESULTS_DIR/benchmark-7/q6.out
cp $RESULTS_DIR/benchmark-1/q6.time $RESULTS_DIR/benchmark-7/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-7/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-7/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-7/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-7/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-7/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-7/q9.time
cp $RESULTS_DIR/benchmark-0/q10.out $RESULTS_DIR/benchmark-7/q10.out
cp $RESULTS_DIR/benchmark-0/q10.time $RESULTS_DIR/benchmark-7/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-7/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-7/q11.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-7/q12.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-7/program $PROG_ARGS 12 2>&1 | tee $RESULTS_DIR/benchmark-7/q12.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-7/q12.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-7/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-7/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-7/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-7/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-7/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-7/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-7/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-7/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-7/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-7/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-7/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-7/q18.time

# benchmark 8
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-8/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-8/q0.time
cp $RESULTS_DIR/benchmark-7/q1.out $RESULTS_DIR/benchmark-8/q1.out
cp $RESULTS_DIR/benchmark-7/q1.time $RESULTS_DIR/benchmark-8/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-8/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-8/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-8/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-8/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-8/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-8/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-8/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-8/q5.time
cp $RESULTS_DIR/benchmark-1/q6.out $RESULTS_DIR/benchmark-8/q6.out
cp $RESULTS_DIR/benchmark-1/q6.time $RESULTS_DIR/benchmark-8/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-8/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-8/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-8/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-8/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-8/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-8/q9.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-8/q10.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-8/program $PROG_ARGS 10 2>&1 | tee $RESULTS_DIR/benchmark-8/q10.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-8/q10.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-8/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-8/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-8/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-8/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-8/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-8/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-8/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-8/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-8/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-8/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-8/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-8/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-8/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-8/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-8/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-8/q18.time

# benchmark 9
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-9/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-9/q0.time
cp $RESULTS_DIR/benchmark-7/q1.out $RESULTS_DIR/benchmark-9/q1.out
cp $RESULTS_DIR/benchmark-7/q1.time $RESULTS_DIR/benchmark-9/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-9/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-9/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-9/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-9/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-9/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-9/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-9/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-9/q5.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-9/q6.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-9/program $PROG_ARGS 6 2>&1 | tee $RESULTS_DIR/benchmark-9/q6.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-9/q6.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-9/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-9/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-9/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-9/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-9/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-9/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-9/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-9/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-9/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-9/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-9/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-9/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-9/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-9/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-9/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-9/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-9/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-9/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-9/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-9/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-9/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-9/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-9/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-9/q18.time

# benchmark 10
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-10/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-10/q0.time
/usr/bin/time -f '%e,%U,%S' -o $RESULTS_DIR/benchmark-10/q1.time -a timeout 10m /home/stephentu/cryptdb/obj/benchmark-10/program $PROG_ARGS 1 2>&1 | tee $RESULTS_DIR/benchmark-10/q1.out
if [ ${PIPESTATUS[0]} -eq 124 ]; then
  echo 'killed by timeout' >> $RESULTS_DIR/benchmark-10/q1.out
fi
reset_exp $REMOTE_HOST
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-10/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-10/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-10/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-10/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-10/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-10/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-10/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-10/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-10/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-10/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-10/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-10/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-10/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-10/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-10/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-10/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-10/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-10/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-10/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-10/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-10/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-10/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-10/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-10/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-10/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-10/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-10/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-10/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-10/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-10/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-10/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-10/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-10/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-10/q18.time

# benchmark 11
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-11/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-11/q0.time
cp $RESULTS_DIR/benchmark-10/q1.out $RESULTS_DIR/benchmark-11/q1.out
cp $RESULTS_DIR/benchmark-10/q1.time $RESULTS_DIR/benchmark-11/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-11/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-11/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-11/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-11/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-11/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-11/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-11/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-11/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-11/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-11/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-11/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-11/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-11/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-11/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-11/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-11/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-11/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-11/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-11/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-11/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-11/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-11/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-11/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-11/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-11/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-11/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-11/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-11/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-11/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-11/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-11/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-11/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-11/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-11/q18.time

# benchmark 12
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-12/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-12/q0.time
cp $RESULTS_DIR/benchmark-10/q1.out $RESULTS_DIR/benchmark-12/q1.out
cp $RESULTS_DIR/benchmark-10/q1.time $RESULTS_DIR/benchmark-12/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-12/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-12/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-12/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-12/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-12/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-12/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-12/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-12/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-12/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-12/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-12/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-12/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-12/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-12/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-12/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-12/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-12/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-12/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-12/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-12/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-12/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-12/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-12/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-12/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-12/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-12/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-12/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-12/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-12/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-12/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-12/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-12/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-12/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-12/q18.time

# benchmark 13
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-13/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-13/q0.time
cp $RESULTS_DIR/benchmark-10/q1.out $RESULTS_DIR/benchmark-13/q1.out
cp $RESULTS_DIR/benchmark-10/q1.time $RESULTS_DIR/benchmark-13/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-13/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-13/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-13/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-13/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-13/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-13/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-13/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-13/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-13/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-13/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-13/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-13/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-13/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-13/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-13/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-13/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-13/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-13/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-13/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-13/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-13/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-13/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-13/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-13/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-13/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-13/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-13/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-13/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-13/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-13/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-13/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-13/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-13/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-13/q18.time

# benchmark 14
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-14/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-14/q0.time
cp $RESULTS_DIR/benchmark-10/q1.out $RESULTS_DIR/benchmark-14/q1.out
cp $RESULTS_DIR/benchmark-10/q1.time $RESULTS_DIR/benchmark-14/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-14/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-14/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-14/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-14/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-14/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-14/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-14/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-14/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-14/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-14/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-14/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-14/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-14/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-14/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-14/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-14/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-14/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-14/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-14/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-14/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-14/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-14/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-14/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-14/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-14/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-14/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-14/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-14/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-14/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-14/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-14/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-14/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-14/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-14/q18.time

# benchmark 15
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-15/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-15/q0.time
cp $RESULTS_DIR/benchmark-10/q1.out $RESULTS_DIR/benchmark-15/q1.out
cp $RESULTS_DIR/benchmark-10/q1.time $RESULTS_DIR/benchmark-15/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-15/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-15/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-15/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-15/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-15/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-15/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-15/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-15/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-15/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-15/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-15/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-15/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-15/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-15/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-15/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-15/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-15/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-15/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-15/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-15/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-15/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-15/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-15/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-15/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-15/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-15/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-15/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-15/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-15/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-15/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-15/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-15/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-15/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-15/q18.time

# benchmark 16
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-16/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-16/q0.time
cp $RESULTS_DIR/benchmark-10/q1.out $RESULTS_DIR/benchmark-16/q1.out
cp $RESULTS_DIR/benchmark-10/q1.time $RESULTS_DIR/benchmark-16/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-16/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-16/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-16/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-16/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-16/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-16/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-16/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-16/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-16/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-16/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-16/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-16/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-16/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-16/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-16/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-16/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-16/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-16/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-16/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-16/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-16/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-16/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-16/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-16/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-16/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-16/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-16/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-16/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-16/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-16/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-16/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-16/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-16/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-16/q18.time

# benchmark 17
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-17/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-17/q0.time
cp $RESULTS_DIR/benchmark-10/q1.out $RESULTS_DIR/benchmark-17/q1.out
cp $RESULTS_DIR/benchmark-10/q1.time $RESULTS_DIR/benchmark-17/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-17/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-17/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-17/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-17/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-17/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-17/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-17/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-17/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-17/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-17/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-17/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-17/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-17/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-17/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-17/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-17/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-17/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-17/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-17/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-17/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-17/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-17/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-17/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-17/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-17/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-17/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-17/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-17/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-17/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-17/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-17/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-17/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-17/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-17/q18.time

# benchmark 18
cp $RESULTS_DIR/benchmark-2/q0.out $RESULTS_DIR/benchmark-18/q0.out
cp $RESULTS_DIR/benchmark-2/q0.time $RESULTS_DIR/benchmark-18/q0.time
cp $RESULTS_DIR/benchmark-10/q1.out $RESULTS_DIR/benchmark-18/q1.out
cp $RESULTS_DIR/benchmark-10/q1.time $RESULTS_DIR/benchmark-18/q1.time
cp $RESULTS_DIR/benchmark-2/q2.out $RESULTS_DIR/benchmark-18/q2.out
cp $RESULTS_DIR/benchmark-2/q2.time $RESULTS_DIR/benchmark-18/q2.time
cp $RESULTS_DIR/benchmark-2/q3.out $RESULTS_DIR/benchmark-18/q3.out
cp $RESULTS_DIR/benchmark-2/q3.time $RESULTS_DIR/benchmark-18/q3.time
cp $RESULTS_DIR/benchmark-2/q4.out $RESULTS_DIR/benchmark-18/q4.out
cp $RESULTS_DIR/benchmark-2/q4.time $RESULTS_DIR/benchmark-18/q4.time
cp $RESULTS_DIR/benchmark-4/q5.out $RESULTS_DIR/benchmark-18/q5.out
cp $RESULTS_DIR/benchmark-4/q5.time $RESULTS_DIR/benchmark-18/q5.time
cp $RESULTS_DIR/benchmark-9/q6.out $RESULTS_DIR/benchmark-18/q6.out
cp $RESULTS_DIR/benchmark-9/q6.time $RESULTS_DIR/benchmark-18/q6.time
cp $RESULTS_DIR/benchmark-3/q7.out $RESULTS_DIR/benchmark-18/q7.out
cp $RESULTS_DIR/benchmark-3/q7.time $RESULTS_DIR/benchmark-18/q7.time
cp $RESULTS_DIR/benchmark-3/q8.out $RESULTS_DIR/benchmark-18/q8.out
cp $RESULTS_DIR/benchmark-3/q8.time $RESULTS_DIR/benchmark-18/q8.time
cp $RESULTS_DIR/benchmark-2/q9.out $RESULTS_DIR/benchmark-18/q9.out
cp $RESULTS_DIR/benchmark-2/q9.time $RESULTS_DIR/benchmark-18/q9.time
cp $RESULTS_DIR/benchmark-8/q10.out $RESULTS_DIR/benchmark-18/q10.out
cp $RESULTS_DIR/benchmark-8/q10.time $RESULTS_DIR/benchmark-18/q10.time
cp $RESULTS_DIR/benchmark-1/q11.out $RESULTS_DIR/benchmark-18/q11.out
cp $RESULTS_DIR/benchmark-1/q11.time $RESULTS_DIR/benchmark-18/q11.time
cp $RESULTS_DIR/benchmark-7/q12.out $RESULTS_DIR/benchmark-18/q12.out
cp $RESULTS_DIR/benchmark-7/q12.time $RESULTS_DIR/benchmark-18/q12.time
cp $RESULTS_DIR/benchmark-0/q13.out $RESULTS_DIR/benchmark-18/q13.out
cp $RESULTS_DIR/benchmark-0/q13.time $RESULTS_DIR/benchmark-18/q13.time
cp $RESULTS_DIR/benchmark-6/q14.out $RESULTS_DIR/benchmark-18/q14.out
cp $RESULTS_DIR/benchmark-6/q14.time $RESULTS_DIR/benchmark-18/q14.time
cp $RESULTS_DIR/benchmark-0/q15.out $RESULTS_DIR/benchmark-18/q15.out
cp $RESULTS_DIR/benchmark-0/q15.time $RESULTS_DIR/benchmark-18/q15.time
cp $RESULTS_DIR/benchmark-3/q16.out $RESULTS_DIR/benchmark-18/q16.out
cp $RESULTS_DIR/benchmark-3/q16.time $RESULTS_DIR/benchmark-18/q16.time
cp $RESULTS_DIR/benchmark-1/q17.out $RESULTS_DIR/benchmark-18/q17.out
cp $RESULTS_DIR/benchmark-1/q17.time $RESULTS_DIR/benchmark-18/q17.time
cp $RESULTS_DIR/benchmark-5/q18.out $RESULTS_DIR/benchmark-18/q18.out
cp $RESULTS_DIR/benchmark-5/q18.time $RESULTS_DIR/benchmark-18/q18.time

done
throttle_bw_end $REMOTE_HOST
end_compression_channel
