#include <vector>
#include <cmath>
#include <stdint.h>

using namespace std;

const int N = 3;
const double alpha = 0.35;

const int num_bits = (int) ceil(log(N)/log(2));

uint64_t mask;

uint64_t make_mask(){
	uint64_t cur_mask = 1ULL;
	for(int i=1; i<num_bits; i++){
		cur_mask = cur_mask | (1ULL<<i);
	}
	return cur_mask;
}

class ope_lookup_failure {};

template<class EncT>
struct tree_node;

template<class EncT>
class tree {

public:

	tree_node<EncT> *root; 
	unsigned int num_nodes;

	void insert_recursive(EncT key);
	void insert(uint64_t v, uint64_t nbits, EncT encval);
	vector<tree_node<EncT>* > tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, EncT encval, uint64_t pathlen);

	tree_node<EncT>* findScapegoat( vector<tree_node<EncT>* > path );

	void print_tree();

	tree_node<EncT>* tree_lookup(tree_node<EncT> *root, uint64_t v, uint64_t nbits) const;
	vector<EncT> lookup(uint64_t v, uint64_t nbits) const;


	tree(){
		num_nodes=0;
		root = new tree_node<EncT>();
	}

	bool test_tree(tree_node<EncT>* cur_node);

	bool test_node(tree_node<EncT>* cur_node);

	bool test_vals(tree_node<EncT>* cur_node, EncT low, EncT high);
};
