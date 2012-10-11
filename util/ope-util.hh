#pragma once

#include <algorithm>
#include <cmath>

const int N = 4;
const double alpha = 0.3;

// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

typedef  uint64_t OPEType;

//Whether to print debugging output or not
#define DEBUG 1
#define DEBUG_COMM 1
#define DEBUG_BTREE 0
#define MALICIOUS 0

//Make mask of num_bits 1's
static uint64_t
make_mask(){
    uint64_t cur_mask = 1ULL;
    for(int i=1; i<num_bits; i++){
	cur_mask = cur_mask | (1ULL<<i);
    }
    return cur_mask;
}

uint64_t mask = make_mask();

std::string
opeToStr(OPEType ope);
