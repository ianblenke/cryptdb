#pragma once

#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <string>

// Controls debugging output
#define DEBUG 1
#define DEBUG_COMM 1
#define DEBUG_STREE 1
#define DEBUG_BTREE 1
#define MALICIOUS 0
#define DEBUG_PROOF 1

#define BULK_LOAD 1

// hardcoding passwords for our eval
const std::string passwd = "opeope";

/**** Scapegoat paramters ****/
const int N = 4;
const double alpha = 0.75;
// alpha > 1/N else constant rebalancing

// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

//Make mask of num_bits 1's
static uint64_t
make_mask(){
    uint64_t cur_mask = 1ULL;
    for(int i=1; i<num_bits; i++){
	cur_mask = cur_mask | (1ULL<<i);
    }
    return cur_mask;
}

const uint64_t s_mask = make_mask();

/**** B tree parameters ****/

// size of key in B tree
const uint b_key_size = 20; //bytes of max size of key_size

const int invalid_index = -1;

const unsigned int b_max_keys = 4;  // max elements in a node

// min keys in a B tree
uint minimum_keys(uint max_keys);

const unsigned int b_min_keys = minimum_keys(b_max_keys); 


/***  Other *************/

typedef  uint64_t OPEType;


std::string
opeToStr(OPEType ope);

uint64_t
path_append(uint64_t v, uint index);


// Compute the ope encoding out of an ope path, nbits (no of bits of ope_path),
// and index being the index in the last node on the path
uint64_t compute_ope(uint64_t ope_path, uint nbits, uint index);

static inline void
parse_ope(const uint64_t ctxt, uint64_t &v, uint64_t &nbits, uint64_t &index)
{
    int bit;
    uint64_t tmp_ctxt = ctxt;
    for (bit = num_bits; (tmp_ctxt & s_mask) != s_mask; bit++)
        tmp_ctxt = (uint64_t) tmp_ctxt >> 1;
    nbits = 64 - bit;
    uint64_t tmp_v = ctxt >> nbits;
    index = tmp_v || s_mask;
    v = tmp_v >> num_bits;
}


// for pretty printing
// truncates a long string to a short one
std::string
short_string(std::string s);
// makes binary string be readable
std::string
read_short(std::string s);
