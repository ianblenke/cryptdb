#!/bin/bash

COUNT=262144

# sequential write perf
dd bs=4K count=$COUNT if=/dev/zero of=/tmp/dummy_file conv=fdatasync

# flush disk cache
sync
sudo sh -c 'echo 1 > /proc/sys/vm/drop_caches'
sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
sleep 1

# sequential read perf
dd bs=4K count=$COUNT if=/tmp/dummy_file of=/dev/null
