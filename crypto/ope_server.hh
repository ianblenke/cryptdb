#pragma once

#include <string>
#include "../util/static_assert.hh"
#include <iostream>
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
    void insert(uint64_t v, uint64_t nbits, const EncT &encval);
    void print_table();
    void print_tree();

    ope_server();
    ~ope_server();

 private:

    tree_node<EncT> *root; 
    boost::unordered_map<EncT, pair<uint64_t,uint64_t> > ope_table;


    void print_tree(tree_node<EncT> * n);

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


