#include <vector>
#include <cmath>
#include <stdint.h>
#include <iostream>
#include <boost/unordered_map.hpp>
#include <edb/Connect.hh>

//Whether to print debugging output or not
#define DEBUG 0
#define DEBUG_COMM 0

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::ceil;
using std::sort;
using std::vector;

const int N = 4;
const double alpha = 0.3;


// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

uint64_t mask;

uint64_t make_mask();

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
	int version;
};

class ope_lookup_failure {};

template<class EncT>
struct tree_node;

template<class EncT>
class tree {

public:
	tree_node<EncT> *root;
	unsigned int num_nodes;
	Connect * dbconnect;
	int global_version;

	void insert(uint64_t v, uint64_t nbits, EncT encval);
	vector<tree_node<EncT>* > tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, EncT encval, uint64_t pathlen);

	tree_node<EncT>* findScapegoat( vector<tree_node<EncT>* > path );

	void print_tree();

	tree_node<EncT>* tree_lookup(tree_node<EncT> *root, uint64_t v, uint64_t nbits) const;
	vector<EncT> lookup(uint64_t v, uint64_t nbits) const;
	table_storage lookup(EncT xct);

	boost::unordered_map<EncT, table_storage > ope_table;
	void update_ope_table(tree_node<EncT>* node, table_storage base);
	void update_db(table_storage old_entry, table_storage new_entry);

	vector<EncT> flatten(tree_node<EncT>* node);
	tree_node<EncT>* rebuild(vector<EncT> key_list);
	void rebalance(tree_node<EncT>* node);
	void delete_nodes(tree_node<EncT>* node);

	tree(){
		num_nodes=0;
		root = NULL;
		global_version=0;
		dbconnect =new Connect( "localhost", "frank", "passwd","cryptdb", 3306);
	}

	~tree(){
		delete_nodes(root);
		delete root;
	}


	bool test_tree(tree_node<EncT>* cur_node);

	bool test_node(tree_node<EncT>* cur_node);

	bool test_vals(tree_node<EncT>* cur_node, EncT low, EncT high);

};


