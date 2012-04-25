#pragma once

#include <string>
#include "blowfish.hh"
#include "../util/static_assert.hh"
#include <iostream>
#include <map>
#include <boost/unordered_map.hpp>
#include <utility>

using namespace std;

//parameters
const double scapegoat_alpha = 0.5;

template<class EncT>
struct tree_node;

class ope_lookup_failure {};

static inline int
ffsl(uint64_t mask)
{
	int bit;

	if (mask == 0)
		return (0);
	for (bit = 1; !(mask & 1); bit++)
		mask = (uint64_t)mask >> 1;
	return (bit);
}

//this code should run in the udf on the server
template<class EncT>
class ope_server {
 public:
    EncT lookup(uint64_t v, uint64_t nbits) const;
    pair<uint64_t, uint64_t> lookup(EncT xct);
    void update_table(EncT xct, uint64_t v, uint64_t nbits);
    void insert(uint64_t v, uint64_t nbits, const EncT &encval);
    void print_table();

    ope_server();
    ~ope_server();
 tree_node<EncT> *root; 
     boost::unordered_map<EncT, pair<pair<uint64_t,uint64_t>, bool> > ope_table;

 private:
    tree_node<EncT> * tree_lookup(tree_node<EncT> *root, uint64_t v, uint64_t nbits) const;
    void tree_insert(tree_node<EncT> **np, uint64_t v, const EncT &encval,
		     uint64_t nbits, uint64_t pathlen);
    
    //relabels the tree rooted at the node whose parent is "parent"
    // size indicates the size of the subtree of the node rooted at parent
    void relabel(tree_node<EncT> * parent, bool isLeft, uint64_t size);

    void update_ope_table(tree_node<EncT>*n);
    //decides whether we trigger a relabel or not
    //receives the path length of a recently added node
    bool trigger(uint64_t path_len) const;
    //upon relabel, finds the node that is the root of the subtree to be relabelled
    // given v: last ciphertext value inserted
    // returns the parent of this node, and whether it is left or right;
    // or null if root needs to be balanced
    tree_node<EncT> * node_to_balance(uint64_t v, uint64_t nbits, bool & isLeft, uint64_t & subtree_size);
    //updates three statistics upon node insert
    void update_tree_stats(uint64_t nbits);
    unsigned int num_nodes;
    unsigned int max_height;//height measured in no. of edges
};

template<class V, class BlockCipher>
class ope_client {
 public:
    ope_client(BlockCipher *bc, ope_server<V> *server) : b(bc), s(server) {
//        _static_assert(BlockCipher::blocksize == sizeof(V));
    }

    V decrypt(uint64_t ct) const {
	uint64_t nbits = 64 - ffsl((uint64_t)ct);
	
        return block_decrypt(s->lookup(ct>>(64-nbits), nbits));
	
    }

    uint64_t encrypt(V pt) const {
	pair<uint64_t, uint64_t> early_rtn_val = s->lookup(block_encrypt(pt));
	if(early_rtn_val.first!=-1 && early_rtn_val.second!=-1) {
		return (early_rtn_val.first<<(64-early_rtn_val.second)) |
			(1ULL<<(63-early_rtn_val.second));
	}
        uint64_t v = 0;
        uint64_t nbits = 0;
        try {
            for (;;) {
		V xct = s->lookup(v, nbits);
		V xpt = block_decrypt(xct);
		
                if (pt == xpt) {
		    break;
		}
                if (pt < xpt) {
                    v = (v<<1) | 0;
		}
                else {
                    v = (v<<1) | 1;
		}
                nbits++;
            }
        } catch (ope_lookup_failure&) {
	    cout<<pt<<"  not in tree. "<<nbits<<": " <<block_encrypt(pt)<<" v: "<<v<<endl;

            s->insert(v, nbits, block_encrypt(pt));
	    //relabeling may have been triggered so we need to lookup value again
	    //todo: optimize by avoiding extra tree lookup
	    return encrypt(pt);
        }
        assert(nbits <= 63);
	s->update_table(block_encrypt(pt),v,nbits);
//	return v;
        return (v<<(64-nbits)) | (1ULL<<(63-nbits));
    }

 private:
    V block_decrypt(V ct) const {
        V pt;
	pt=ct;
//        b->block_decrypt((const uint8_t *) &ct, (uint8_t *) &pt);
        return pt;
    }

    V block_encrypt(V pt) const {
        V ct;
	ct=pt;
//        b->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
        return ct;
    }

    BlockCipher *b;
    ope_server<V> *s;
};

class not_a_cipher{

	public:
	not_a_cipher(){}
	~not_a_cipher(){}

	void block_decrypt(const uint8_t* ct, uint8_t * pt){
		*pt=*ct;
	}

	void block_encrypt(const uint8_t* pt, uint8_t * ct){
		*ct=*pt;
	}
};
