#include <vector>
#include <cmath>
#include <stdint.h>
#include <iostream>
#include <assert.h>
#include <boost/unordered_map.hpp>

//Whether to print debugging output or not
#define DEBUG 0

using namespace std;

const int N = 4;
const double alpha = 0.3;


// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

uint64_t mask;


//Make mask of num_bits 1's
uint64_t make_mask(){
	uint64_t cur_mask = 1ULL;
	for(int i=1; i<num_bits; i++){
		cur_mask = cur_mask | (1ULL<<i);
	}
	return cur_mask;
}

struct table_storage{
	uint64_t v;
	uint64_t pathlen;
	uint64_t index;
};

class ope_lookup_failure {};

template<class EncT>
struct tree_node;

template<class EncT>
class tree {

public:
	tree_node<EncT> *root; 
	unsigned int num_nodes;

	void insert(uint64_t v, uint64_t nbits, EncT encval);
	vector<tree_node<EncT>* > tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, EncT encval, uint64_t pathlen);

	tree_node<EncT>* findScapegoat( vector<tree_node<EncT>* > path );

	void print_tree();

	tree_node<EncT>* tree_lookup(tree_node<EncT> *root, uint64_t v, uint64_t nbits) const;
	vector<EncT> lookup(uint64_t v, uint64_t nbits) const;
	table_storage lookup(EncT xct);

	boost::unordered_map<EncT, table_storage > ope_table;
	void update_ope_table(tree_node<EncT>* node, table_storage base);

	vector<EncT> flatten(tree_node<EncT>* node);
	tree_node<EncT>* rebuild(vector<EncT> key_list);
	void rebalance(tree_node<EncT>* node);
	void delete_nodes(tree_node<EncT>* node);

	tree(){
		num_nodes=0;
		root = NULL;
	}

	~tree(){
		delete_nodes(root);
		delete root;
	}

};


