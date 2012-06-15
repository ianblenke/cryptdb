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

	pair<vector<EncT>, vector<EncT> > split(vector<EncT> key_list){
		int mid = key_list.size()/2;
		vector<EncT> lower, higher;
		for(int i=0; i<mid; i++){
			lower.push_back(key_list[i]);
		}
		for(int i=mid+1; i<key_list.size(); i++){
			higher.push_back(key_list[i]);
		}
		return make_pair(lower, higher);
	}

	EncT getMid(vector<EncT> key_list){
		int mid = key_list.size()/2;
		return key_list[mid];
	}

	tree_node* rebuild(vector<EncT> key_list){
		tree_node* rtn_node = new tree_node();

		if(key_list.size()<N){
			rtn_node->keys=key_list;
		}else{
			//Assuming N is 4
			EncT lm, m, rm;
			vector<EncT> l, ll, rl, r, lr, rr;

			m = getMid(key_list);
			pair<vector<EncT>, vector<EncT> > split_pair = split(key_list);
			l = split_pair.first;
			r = split_pair.second;
			lm = getMid(l);
			rm = getMid(r);

			pair<vector<EncT>, vector<EncT> > left_pair = split(l);
			pair<vector<EncT>, vector<EncT> > right_pair = split(r);

			ll = left_pair.first;
			lr = left_pair.second;
			rl = right_pair.first;
			rr = right_pair.second;

			rtn_node->keys.push_back(lm);
			rtn_node->keys.push_back(m);
			rtn_node->keys.push_back(rm);

			if(ll.size()>0)	rtn_node->right[NULL]= rebuild(ll);
			if(lr.size()>0)	rtn_node->right[lm]=rebuild(lr);
			if(rl.size()>0)	rtn_node->right[m]=rebuild(rl);
			if(rr.size()>0)	rtn_node->right[rm]=rebuild(rr);
		}
		return rtn_node;

	}

	void rebalance(){
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
tree<EncT>::insert(EncT key){

	vector<tree_node<EncT> * > path =root->insert(key);

	double height= (double) path.size();

	if (height==0) return;

	size+=1;
	if(height> log(size)/log(((double)1.0)/alpha)+1 ){
		tree_node<EncT>* scapegoat = findScapegoat(path);
		scapegoat->rebalance();

	}
	//cout<<(root->height()<=log(size)/log(((double)1.0)/alpha)+1)<<endl;
}

template<class EncT>
bool
tree<EncT>::test_tree(tree_node<EncT>* cur_node){

	if(test_node(cur_node)!=true) return false;

	typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

	for(it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
		if(test_tree(it->second)!=true) return false;
	}
	return true;


}

template<class EncT>
bool
tree<EncT>::test_node(tree_node<EncT>* cur_node){
	vector<EncT> sorted_keys = cur_node->keys;
	if(sorted_keys.size()==0) return false;
	if(sorted_keys.size()>3) return false;
	sort(sorted_keys.begin(), sorted_keys.end());

	int low = 0;
	int high = sorted_keys[0];
	int i=0;
	if(cur_node->key_in_map(NULL)){
		if(test_vals(cur_node->right[NULL], low, high)!=true) return false;
	}

	if(sorted_keys.size()>1){
		i++;
		low = high;
		high = sorted_keys[1];

		if(cur_node->key_in_map(sorted_keys[0])){
			if(test_vals(cur_node->right[sorted_keys[0]], low, high)!=true) return false;
		}

	}
	if(sorted_keys.size()>2){
		i++;
		low = high;
		high = sorted_keys[2];

		if(cur_node->key_in_map(sorted_keys[1])){
			if(test_vals(cur_node->right[sorted_keys[1]], low, high)!=true) return false;
		}		
	}
	
	low = high;
	high = RAND_MAX;

	if(cur_node->key_in_map(sorted_keys[i])){
		if(test_vals(cur_node->right[sorted_keys[i]], low, high)!=true) return false;
	}
	return true;

}

template<class EncT>
bool
tree<EncT>::test_vals(tree_node<EncT>* cur_node, EncT low, EncT high){
	for(int i=0; i<cur_node->keys.size(); i++){
		EncT this_key = cur_node->keys[i];
		if(this_key<=low || this_key>=high){
			return false;
		}
	}
	typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

	for(it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
		if(test_vals(it->second, low, high)!=true) return false;
	}
	return true;
}

int main(){
	tree<uint64_t>* my_tree = new tree<uint64_t>();

	srand( time(NULL));

	for(int i=0; i<1000000; i++){
		my_tree->insert((uint64_t) rand());
	}
	cout<<"Done inserting"<<endl;
	cout<<"Testing tree: "<<my_tree->test_tree(my_tree->root)<<endl;
	cout<<"Size: "<<my_tree->size<<" Height: "<<my_tree->root->height()<<endl;
	cout<<"Alpha-height balanced: "<<(my_tree->root->height()< log(my_tree->size)/log(((double)1.0)/alpha)+1)<<endl;
	
}