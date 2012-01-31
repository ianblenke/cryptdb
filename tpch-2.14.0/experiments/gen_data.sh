#!/bin/bash

## run this script (from the enc-data folder)
## to generate data

PWD=`pwd`
if [ `basename $PWD` != 'enc-data' ]; then
  echo "run from enc-data folder"
  exit 1;
fi

set -x

source ../tpch-2.14.0/experiments/scales.sh

for scale in $SCALES; do
    mkdir -p scale-$scale
    cd scale-$scale
    DBGEN=../../tpch-2.14.0/dbgen/dbgen
    cp ../../tpch-2.14.0/dbgen/dists.dss .  #hack
    yes | $DBGEN -s $scale
    rm dists.dss
    cd ..
done
