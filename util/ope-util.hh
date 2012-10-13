#pragma once

#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <string>

const int N = 4;
const double alpha = 0.3;

// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

// hardcoding passwords for our eval
const std::string passwd = "opeope";

//Whether to print debugging output or not
#define DEBUG 1
#define DEBUG_COMM 1
#define DEBUG_STREE 1
#define DEBUG_BTREE 1
#define MALICIOUS 0

typedef  uint64_t OPEType;

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

uint64_t
path_append(uint64_t v, uint index);


// Compute the ope encoding out of an ope path, nbits (no of bits of ope_path),
// and index being the index in the last node on the path
template<class EncT>
static
uint64_t compute_ope(uint64_t ope_path, uint nbits, uint index) {
    ope_path = (ope_path << num_bits) | index;
    nbits+=num_bits;

    return (ope_path << (64-nbits)) | (mask << (64-num_bits-nbits));
}
