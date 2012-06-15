#include <vector>

using namespace std;

const int N = 4;
const double alpha = 0.3;

template<class EncT>
struct tree_node;

template<class EncT>
class tree {

public:

	tree_node<EncT> *root; 
	double size;

	void insert(EncT key);
	tree_node<EncT>* findScapegoat( vector<tree_node<EncT>* > path );
	void print_tree();


	tree(){
		size=0;
		root = new tree_node<EncT>();
	}

	bool test_tree(tree_node<EncT>* cur_node);

	bool test_node(tree_node<EncT>* cur_node);

	bool test_vals(tree_node<EncT>* cur_node, EncT low, EncT high);
};
