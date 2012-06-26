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

	//Strict predecessor. aka 7 is pred of 8, but 8 is not pred of 8.
	EncT pred(EncT key){
		EncT tmp_pred_key = 0;
		bool pred_exists = false;
		for (int i = 0; i<keys.size(); i++)
		{
			if (keys[i] < key && keys[i] > tmp_pred_key)
			{
				pred_exists=true;
				tmp_pred_key = keys[i];
			}
		}
		if(pred_exists){
			return tmp_pred_key;
		}else{
			return NULL;
		}
	}

	bool key_in_map(EncT key){
	    // RP: in order for it->first==key to work you need to define
	    // an operator == for the EncT class - otherwise, it may not
	    // compare the way you want; have you defined this and maybe I did
	    // not see it? 
		typename boost::unordered_map<EncT, tree_node *>::iterator it;
		for(it=right.begin(); it!=right.end(); it++){
			if(it->first==key) return true;
		}
		return false;
	}

	vector<tree_node* > insert(EncT key){

		vector<tree_node*> rtn_path;

		//Check if key is in node (maybe later can rid this code)
		for(int i = 0; i<keys.size(); i++){
			if(keys[i]==key) return rtn_path;
		}

		

		if( keys.size() < N-1){
			keys.push_back(key);
			rtn_path.push_back(this);
			return rtn_path;
		}else{
			EncT pred_key = pred(key);
			if(!key_in_map(pred_key)){
				right[pred_key] = new tree_node();
			}
			tree_node* pred_node = right[pred_key];
			rtn_path = pred_node->insert(key);
			
			if(rtn_path.size()>0){
				rtn_path.push_back(this);
			}

			return rtn_path;

		}

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
		cout<<"Rebalance"<<endl;
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
	vector<tree_node<EncT>* > queue;
	queue.push_back(root);
	while(queue.size()!=0){
		tree_node<EncT>* cur_node = queue[0];
		//if(cur_node==NULL) {cout<<endl; queue.erase(queue.begin()); continue;}
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
		queue.erase(queue.begin());
		//queue.push_back(NULL);
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
tree<EncT>::insert_recursive(EncT key){

	vector<tree_node<EncT> * > path =root->insert(key);

	double height= (double) path.size();

	if (height==0) return;

	num_nodes+=1;
	if(height> log(num_nodes)/log(((double)1.0)/alpha)+1 ){
		tree_node<EncT>* scapegoat = findScapegoat(path);
		scapegoat->rebalance();

	}
	//cout<<(root->height()<=log(size)/log(((double)1.0)/alpha)+1)<<endl;
}

template<class EncT>
void
tree<EncT>::insert(uint64_t v, uint64_t nbits, EncT encval){
	vector<tree_node<EncT> * > path=tree_insert(root, v, nbits, encval, nbits);
	double height= (double) path.size();

	if (height==0) return;

	if(height> log(num_nodes)/log(((double)1.0)/alpha)+1 ){
		tree_node<EncT>* scapegoat = findScapegoat(path);
		print_tree();
		scapegoat->rebalance();

	}
}

template<class EncT>
vector<tree_node<EncT>* >
tree<EncT>::tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, EncT encval, uint64_t pathlen){

	vector<tree_node<EncT>* > rtn_path;

	if(nbits==0){
		if(node->keys.size()>N-2) cout<<"Insert fail, found full node"<<endl;
		else{
			node->keys.push_back(encval);
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

    if(key_index==0) key=NULL;
	else if(key_index<N) key = sorted[key_index-1];
	else cout<<"Insert fail, key_index not legal"<<endl;

	//cout<<"Key: "<<key<<endl;

    if(!node->key_in_map(key)){
	    if (nbits == num_bits && node->keys.size()==N-1) {
	    	cout<<"Creating new node"<<endl;
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
    	cout<<"Key not in map"<<endl;
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
    	//throw ope_lookup_failure();
    	cout<<"NOPE!"<<endl;
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

int main(){

	mask = make_mask();

	tree<uint64_t>* my_tree = new tree<uint64_t>();

	//srand( time(NULL));
	srand(0);
	for(int i=0; i<15; i++){
		my_tree->insert_recursive((uint64_t) rand());
	}
	my_tree->print_tree();
	my_tree->insert(10ULL, 4, 2144897763);
	my_tree->insert(42ULL, 6, 2147483646);	
	my_tree->print_tree();	
	vector<uint64_t> root_keys = my_tree->lookup(10ULL,4);
	cout<<"Search node: ";
	for(int i=0; i<root_keys.size(); i++){
		cout<<root_keys[i]<<", ";
	}
	cout<<endl;

	cout<<"Done inserting"<<endl;
	cout<<"Testing tree: "<<my_tree->test_tree(my_tree->root)<<endl;
	cout<<"Size: "<<my_tree->num_nodes<<" Height: "<<my_tree->root->height()<<" Len: "<<my_tree->root->len()<<endl;
	cout<<"Alpha-height balanced: "<<(my_tree->root->height()< log(my_tree->num_nodes)/log(((double)1.0)/alpha)+1)<<endl;
}
