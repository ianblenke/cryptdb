#pragma once

#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include <string>
#include <vector>

#include <util/util.hh>

//whether we run with a database or not
#define WITH_DB 0

// if ope mode is true then trees can no longer use the order operation
// among keys because the real keys are encrypted
#define OPE_MODE 1


// Controls debugging output
#define DEBUG_BARE 0
#define DEBUG 0
#define DEBUG_COMM 0
#define DEBUG_STREE 0
#define DEBUG_BTREE 0
#define DEBUG_PROOF 0
#define DEBUG_EXP 0

// Controls tree type
#define STREE 0

#define BULK_LOAD 1

// hardcoding passwords for our eval
const std::string passwd = "opeopeopeopeopeo";

/**** Scapegoat paramters ****/
const int N = 4;
const double alpha = 0.75;
// alpha > 1/N else constant rebalancing


/**** B tree parameters ****/

// size of key in B tree
const uint b_key_size = 50; //bytes of max size of key_size

const int invalid_index = -1;

const unsigned int b_max_keys = 4;  // max elements in a node

/****** common between the trees ***/
#if STREE
// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));
#else  /*Btree*/
const unsigned int num_bits = (int)ceil(log2(b_max_keys+1.0));
#endif

// min keys in a B tree
uint minimum_keys(uint max_keys);

// the number of keys at a node that does not include the ""key
// therefore, there are b_min_keys+1 subtrees from this node
const unsigned int b_min_keys = minimum_keys(b_max_keys); 


//Make mask of num_bits 1's
static uint64_t
make_mask(){
    uint64_t cur_mask = 1ULL;
    for(int i=1; i< (int)num_bits; i++){
	cur_mask = cur_mask | (1ULL<<i);
    }
    return cur_mask;
}

const uint64_t s_mask = make_mask();


/***  OPE ciphertexts manipulation *************/

typedef  uint64_t OPEType;


const uint  UNIT_VAL = (1 << num_bits);
const uint MAX_INDEX = (1 << num_bits) -1 ;


// transforms an ope encoding into
// a vector of uint each being an edge on the path
std::vector<uint>
enc_to_vec(OPEType val);

// the opposite of above
OPEType
vec_to_enc(const std::vector<uint> & path);


//takes an ope path and transforms it into
// a vector 
std::vector<uint>
path_to_vec(OPEType val, int num);

//takes a vector and transforms it into a ope path value
OPEType
vec_to_path(const std::vector<uint> &path);

bool
match(const std::vector<uint> & ope_path, const std::vector<uint> & path);

std::string
pretty_path(std::vector<uint> v);

std::string
opeToStr(OPEType ope);

uint64_t
path_append(uint64_t v, uint index);

static inline
uint64_t compute_ope(uint64_t ope_path, uint nbits) {
    assert_s(nbits < 64, " too many values considering ope size");
    return (ope_path << (64-nbits)) | (s_mask << (64-num_bits-nbits)); 
}
// Compute the ope encoding out of an ope path, nbits (no of bits of ope_path),
// and index being the index in the last node on the path. Note that
// index is 1 less than index into Node's m_vector! 
// E.g If index = 0, accessing m_vector[1]
static inline
uint64_t compute_ope(uint64_t ope_path, uint nbits, uint index) {
    ope_path = (ope_path << num_bits) | index;
    nbits+=num_bits;

    return compute_ope(ope_path, nbits);
}

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
