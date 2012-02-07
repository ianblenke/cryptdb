#!/bin/bash

# RTT latency
ping -q -c 5 vise4.csail.mit.edu

# Through-put
BWPING=$HOME/bwping-1.4/bwping
sudo $BWPING -b 100000000 -s 1024 -v $((1024 * 1000000)) vise4.csail.mit.edu 
