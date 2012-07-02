#include "nwaytree.hh"
#include <boost/unordered_map.hpp>
#include <vector>
#include <algorithm>
#include <exception>
#include <cmath>
#include <utility>
#include <time.h>
#include <stdlib.h>

using namespace std;

template<class EncT>
struct tree_node
{
	vector<EncT> keys;
	boost::unordered_map<EncT, tree_node *> right;

	tree_node(){}

	~tree_node(){
		keys.clear();
		right.clear();
	}

	bool key_in_map(EncT key){
	    // FL: EncT is being used as a primitive, so this is fine.
		typename boost::unordered_map<EncT, tree_node *>::iterator it;
		for(it=right.begin(); it!=right.end(); it++){
			if(it->first==key) return true;
		}
		return false;
	}

	int height(){
		typename boost::unordered_map<EncT, tree_node *>::iterator it;
		int max_child_height=0;
		int tmp_height=0;

		for(it=right.begin(); it!=right.end(); it++){
			tmp_height=it->second->height();
			if(tmp_height>max_child_height) max_child_height=tmp_height;
		}		

		return max_child_height+1;

	}

	int len(){
		int len = keys.size();
		typename boost::unordered_map<EncT, tree_node *>::iterator it;

		for(it=right.begin(); it!=right.end(); it++){
			len+=it->second->len();
		}		

		return len;		

	}

	vector<EncT> flatten(){
		vector<EncT> rtn_keys = keys;

		typename boost::unordered_map<EncT, tree_node *>::iterator it;

		vector<EncT> child_keys;

		for(it=right.begin(); it!=right.end(); it++){
			child_keys = it->second->flatten();
			for(int i=0; i<child_keys.size(); i++){
				rtn_keys.push_back(child_keys[i]);
			}
		}	

		return rtn_keys;			

	}


	tree_node* rebuild(vector<EncT> key_list){
		//Avoid building a node without any values
		if(key_list.size()==0) return NULL;

		tree_node* rtn_node = new tree_node();

		if(key_list.size()<N){
			//Remaining keys fit in one node, return that node
			rtn_node->keys=key_list;
		}else{
			//Borders to divide key_list by
			double base = ((double) key_list.size())/ ((double) N);

			//Fill keys with every base-th key;
			for(int i=1; i<N; i++){
				rtn_node->keys.push_back(key_list[floor(i*base)]);
			}
			//Handle the subvector rebuilding except for the first and last
			for(int i=1; i<N-1; i++){
				vector<EncT> subvector;
				for(int j=floor(i*base)+1; j<floor((i+1)*base); j++){
					subvector.push_back(key_list[j]);
				}
				tree_node* tmp_child = rebuild(subvector);
				if(tmp_child!=NULL){
					rtn_node->right[key_list[floor(i*base)]]=tmp_child;
				}
			}
			//First subvector rebuilding special case
			vector<EncT> subvector;
			for(int j=0; j<floor(base); j++){
				subvector.push_back(key_list[j]);
			}
			tree_node* tmp_child = rebuild(subvector);
			if(tmp_child!=NULL){
				rtn_node->right[NULL]=tmp_child;
			}	
			subvector.clear();
			//Last subvector rebuilding special case
			for(int j=floor((N-1)*base)+1; j<key_list.size(); j++){
				subvector.push_back(key_list[j]);
			}
			tmp_child = rebuild(subvector);
			if(tmp_child!=NULL){
				rtn_node->right[key_list[floor((N-1)*base)]]=tmp_child;
			}	

		}
		return rtn_node;

	}

	void rebalance(){
		if(DEBUG) cout<<"Rebalance"<<endl;
		vector<EncT> key_list = flatten();
		sort(key_list.begin(), key_list.end());
		tree_node<EncT>* tmp_node = rebuild(key_list);
		delete_nodes();
		keys = tmp_node->keys;
		right = tmp_node->right;
		delete tmp_node;
	}

	void delete_nodes(){
		typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

		for(it=right.begin(); it!=right.end(); it++){
			it->second->delete_nodes();
			delete it->second;
		}

	}

};

template<class EncT>
void 
tree<EncT>::print_tree(){
	bool last_was_null = false;
	vector<tree_node<EncT>* > queue;
	queue.push_back(root);
	while(queue.size()!=0){
		tree_node<EncT>* cur_node = queue[0];
		if(cur_node==NULL){
			queue.erase(queue.begin());
			cout<<endl;
			continue;
		}
/*		if(cur_node==NULL && !last_was_null){
			last_was_null = true;
			cout<<endl;
			queue.erase(queue.begin());
			continue;
		}else if(cur_node==NULL && last_was_null){
			queue.erase(queue.begin());
			continue;
		}else if(cur_node!=NULL && last_was_null){
			last_was_null = false;
		}else{

		}*/
		cout<<cur_node->height()<<": ";
		if(cur_node->key_in_map(NULL)){
			queue.push_back(cur_node->right[NULL]);
		}	
		for(int i=0; i<cur_node->keys.size(); i++){
			EncT key = cur_node->keys[i];
			cout<<key<<", ";
			if(cur_node->key_in_map(key)){
				queue.push_back(cur_node->right[key]);
			}
		}	
		cout<<endl;
		queue.push_back(NULL);
		queue.erase(queue.begin());
	}

}

template<class EncT>
tree_node<EncT>* 
tree<EncT>::findScapegoat( vector<tree_node<EncT>* > path ){
	int tmp_size = 0;
	tree_node<EncT>* last = NULL;
	typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

	int childSize=0;
	vector<int> childSizes;

	//Move along path until you find the scapegoat node
	for(int i=0; i<path.size(); i++){
		tree_node<EncT>* cur_node = path[i];

		int tmp_sum=0;
		for(it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
			if(it->second!=last){
				childSize = it->second->len();
				tmp_sum+=childSize;
				childSizes.push_back(childSize);
			}else{
				tmp_sum+=tmp_size;
				childSizes.push_back(tmp_size);
			}

		}		
		tmp_size = tmp_sum + cur_node->keys.size();
		last = cur_node;

		for(int i=0; i<childSizes.size(); i++){
			if(childSizes[i] > alpha*tmp_size) return cur_node;
		}

		childSizes.clear();

	}

}

template<class EncT>
void
tree<EncT>::insert(uint64_t v, uint64_t nbits, EncT encval){
	//If root doesn't exsit yet, create it, then continue insertion
	if(!root){
		if(DEBUG) cout<<"Inserting root"<<endl;
		root=new tree_node<EncT>();
	}
	vector<tree_node<EncT> * > path=tree_insert(root, v, nbits, encval, nbits);
	double height= (double) path.size();

	if (height==0) return;

	if(DEBUG) print_tree();

	if(height> log(num_nodes)/log(((double)1.0)/alpha)+1 ){
		tree_node<EncT>* scapegoat = findScapegoat(path);
		scapegoat->rebalance();
		if(DEBUG) print_tree();
	}
}

template<class EncT>
vector<tree_node<EncT>* >
tree<EncT>::tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, EncT encval, uint64_t pathlen){

	vector<tree_node<EncT>* > rtn_path;

	//End of the insert line, check if you can insert
	//Assumes v and nbits were from a lookup of an insertable node
	if(nbits==0){
		if(node->keys.size()>N-2) cout<<"Insert fail, found full node"<<endl;
		else{
			if(DEBUG) cout<<"Found node, inserting"<<endl;
			node->keys.push_back(encval);
			sort(node->keys.begin(), node->keys.end());
			num_nodes++;
			rtn_path.push_back(node);
		}
		return rtn_path;
	}
    //Just a check, can remove for performance
    int key_index = (int) (v&(mask<<(nbits-num_bits)))>>(nbits-num_bits);

    vector<EncT> sorted = node->keys;
    sort(sorted.begin(), sorted.end());

    EncT key;

	//cout<<"Index: "<<key_index<<endl;

    //Protocol set: index 0 is NULL (branch to node with lesser elements)
    //Else, index is 1+(key's index in node's key-vector)
    if(key_index==0) key=NULL;
	else if(key_index<N) key = sorted[key_index-1];
	else cout<<"Insert fail, key_index not legal"<<endl;

	//cout<<"Key: "<<key<<endl;

	//See if pointer to next node to be checked 
    if(!node->key_in_map(key)){
	    if (nbits == num_bits && node->keys.size()==N-1) {
	    	if(DEBUG) cout<<"Creating new node"<<endl;
	    	node->right[key] = new tree_node<EncT>();
	    }else cout<<"Insert fail, wrong condition to create new node child"<<endl;
    }
    rtn_path = tree_insert(node->right[key], v, nbits-num_bits, encval, pathlen);
    rtn_path.push_back(node);
    return rtn_path;
}

template<class EncT>
tree_node<EncT> *
tree<EncT>::tree_lookup(tree_node<EncT> *root, uint64_t v, uint64_t nbits) const
{
	//Handle if root doesn't exist yet
	if(!root){
		if(DEBUG) cout<<"First lookup no root"<<endl;
		return 0;
	}

    if (nbits == 0) {
        return root;
    }
    //Just a check, can remove for performance
    int key_index = (int) (v&(mask<<(nbits-num_bits)))>>(nbits-num_bits);

    vector<EncT> sorted = root->keys;
    sort(sorted.begin(), sorted.end());

    EncT key;

	//cout<<"Index: "<<key_index<<endl;

    if(key_index==0) key=NULL;
	else if(key_index<N) key = sorted[key_index-1];
	else return 0;


	//cout<<"Key: "<<key<<endl;

    if(!root->key_in_map(key)){
    	if(DEBUG) cout<<"Key not in map"<<endl;
    	return 0;
    }
    return tree_lookup(root->right[key], v, nbits-num_bits);
}

template<class EncT>
vector<EncT>
tree<EncT>::lookup(uint64_t v, uint64_t nbits) const
{
    tree_node<EncT>* n = tree_lookup(root, v, nbits);
    if(n==0){
    	if(DEBUG) cout<<"NOPE! "<<v<<": "<<nbits<<endl;    	
    	throw ope_lookup_failure();
    }
    return n->keys;

}

template<class EncT>
bool
tree<EncT>::test_tree(tree_node<EncT>* cur_node){
	if(test_node(cur_node)!=true) return false;

	typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

	for(it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
	    if(!test_tree(it->second)) {
		return false;
	    }
	}
	return true;


}

template<class EncT>
bool
tree<EncT>::test_node(tree_node<EncT>* cur_node){
	vector<EncT> sorted_keys = cur_node->keys;
	if(sorted_keys.size()==0) {
		cout<<"Node with no keys"<<endl;
		return false;
	}
	if(sorted_keys.size()>N-1) {
		cout<<"Node with too many keys"<<endl;
		return false;
	}
	sort(sorted_keys.begin(), sorted_keys.end());

	int low = 0;
	int high = sorted_keys[0];
	int i=0;
	if(cur_node->key_in_map(NULL)){
		if(test_vals(cur_node->right[NULL], low, high)!=true) return false;
	}

	for(int j=1; j<N-1; j++){
		if(sorted_keys.size()>j){
			i++;
			low = high;
			high = sorted_keys[j];
			if(cur_node->key_in_map(sorted_keys[j-1])){
				if(test_vals(cur_node->right[sorted_keys[j-1]], low, high)!=true) return false;
			}

		}

	}
	
	low = high;
	high = RAND_MAX; //Note this is 2147483647

	if(cur_node->key_in_map(sorted_keys[i])){
		if(test_vals(cur_node->right[sorted_keys[i]], low, high)!=true) return false;
	}

	for(int x=0; x<sorted_keys.size()-1; x++){
		if(sorted_keys[x]==sorted_keys[x+1]) return false;
	}

	return true;

}

template<class EncT>
bool
tree<EncT>::test_vals(tree_node<EncT>* cur_node, EncT low, EncT high){
	for(int i=0; i<cur_node->keys.size(); i++){
		EncT this_key = cur_node->keys[i];
		if(this_key<=low || this_key>=high){
			cout<<"Values off "<<this_key<<": "<<low<<", "<<high<<endl;
			return false;
		}
	}

	typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

	for(it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
		if(test_vals(it->second, low, high)!=true) return false;
	}
	return true;
}

//RP: We would need the following two tests (automatic) to really be confident
// that it works
// 1. test if the height of the tree is not larger than the scapegoat limit: alpha log(weight) 
// 2. check that an in-order traversal of the tree (with decryptions of the DET)
// indeed yields sorted order on the values
// These tests should be automatic -- keeps inserting elements in the tree and
//  periodically checks the conditions above
// - would be good to test in three cases: values inserted are random, values
// inserted are in increasing order, values inserted are in decreasing order
// I would also count the number of relabelings that happen because we might be
// triggering relabeling too frequently if the bound is too tight

//0 == random
//1 == sorted incr
//2 == sorted decr
bool test_order(int num_vals, int sorted){

	tree<uint64_t>* my_tree = new tree<uint64_t>();

	ope_client<uint64_t>* my_client = new ope_client<uint64_t>(my_tree);

	vector<uint64_t> inserted_vals;
	//srand( time(NULL));
	srand(0);

	for(int i=0; i<num_vals; i++){
		inserted_vals.push_back((uint64_t) rand());
	}
	if(sorted>0){
		sort(inserted_vals.begin(), inserted_vals.end());
	}
	if(sorted>1){
		reverse(inserted_vals.begin(), inserted_vals.end());
	}

	vector<uint64_t> tmp_vals;

	for(int i=0; i<num_vals; i++){
		uint64_t val = inserted_vals[i];
		tmp_vals.push_back(val);
		uint64_t enc_val = my_client->encrypt(val);
		if(DEBUG) cout<<"Enc complete for "<<val<<" with enc="<<enc_val<<endl;

		if(my_tree->test_tree(my_tree->root)!=1){
			cout<<"No test_tree pass"<<i<<endl;
			return false;
		} 
		if(my_tree->num_nodes>1){
			if((my_tree->root->height()< log(my_tree->num_nodes)/log(((double)1.0)/alpha)+1)!=1) {
				cout<<"Height wrong "<<i<<endl;
				return false;
			}
		}

		sort(tmp_vals.begin(), tmp_vals.end());
		uint64_t last_val = 0;
		uint64_t last_enc = 0;
		uint64_t cur_val = 0;
		uint64_t cur_enc = 0;
		for(int j=0; j<tmp_vals.size(); j++){
			cur_val = tmp_vals[j];
			cur_enc = my_client->encrypt(cur_val);
			if(cur_val>=last_val && cur_enc>=last_enc){
				last_val = cur_val;
				last_enc = cur_enc;
			}else{
				cout<<"j: "<<j<<" cur:"<<cur_val<<": "<<cur_enc<<" last: "<<last_val<<": "<<last_enc<<endl;
				return false;
			}
			if(DEBUG) cout<<"Decrypting "<<cur_enc<<endl;
			if(cur_val!=my_client->decrypt(cur_enc)) {
				cout<<"No match with "<<j<<endl;
				return false;
			}
		}		
	}

	cout<<"Sorted order "<<sorted<<" passes test"<<endl;

	delete my_tree;
	delete my_client;

	return true;

}

bool test(int num_vals){
	return test_order(num_vals,0) && test_order(num_vals,1) && test_order(num_vals,2);

}

int main(){

	mask = make_mask();
	if(test(513)) cout<<"Tests passed!"<<endl;
	else cout<<"FAIL"<<endl;

}
