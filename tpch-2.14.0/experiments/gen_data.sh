#!/bin/bash

## run this script (from the enc-data folder)
## to generate data

PWD=`pwd`
if [ `basename $PWD` != 'enc-data' ]; then
  echo "run from enc-data folder"
  exit 1;
fi

set -x

DBGEN=../../tpch-2.14.0/dbgen/dbgen

#for scale in 0.05 0.25 0.50 0.75 1.00 2.00; do
for scale in 2.00; do
  mkdir -p scale-$scale
  cd scale-$scale
  cp ../../tpch-2.14.0/dbgen/dists.dss .  #hack
  yes | $DBGEN -s $scale
  rm dists.dss
  cd ..
done
