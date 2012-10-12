#include "server.hh"
#include <exception>
#include <utility>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>
#include <sys/types.h>

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::ceil;
using std::sort;
using std::vector;
using std::cerr;
using std::map;
using std::max;

// Compute the ope encoding out of an ope path, nbits (no of bits of ope_path),
// and index being the index in the last node on the path
template<class EncT>
static
uint64_t compute_ope(uint64_t ope_path, uint nbits, uint index) {
    ope_path = (ope_path << num_bits) | index;
    nbits+=num_bits;

    return (ope_path << (64-nbits)) | (mask << (64-num_bits-nbits));
}

template<class EncT>
struct tree_node
{
    vector<EncT> keys;
    map<EncT, tree_node *> right; // right has one more element than keys, the 0
				  // element represents the leftmost subtree

    tree_node(){}

    ~tree_node(){
	keys.clear();
	right.clear();
    }

    //Returns true if node's right map contains key (only at non-leaf nodes)
    bool key_in_map(EncT key){ // R: why this???
	// FL: EncT is being used as a primitive, so this is fine.
	typename map<EncT, tree_node *>::iterator it;
	for(it=right.begin(); it!=right.end(); it++){
	    if(it->first==key) return true;
	}
	return false;
    }

    //Recursively calculate height of node in tree
    int height(){
	typename map<EncT, tree_node *>::iterator it;
	int max_child_height=0;
	int tmp_height=0;

	for(it=right.begin(); it!=right.end(); it++){
	    tmp_height=it->second->height();
	    if(tmp_height>max_child_height) max_child_height=tmp_height;
	}		

	return max_child_height+1;

    }

    //Recursively calculate number of keys at node and subtree nodes
    int len(){
	int len = keys.size();
	typename map<EncT, tree_node *>::iterator it;

	for(it=right.begin(); it!=right.end(); it++){
	    len+=it->second->len();
	}		

	return len;		

    }

};

template<class EncT>
tree<EncT>::tree(){
    num_nodes=0;
    max_size=0;
    root = NULL;
    //global_version=0;
    num_rebalances=0;

    root = new tree_node<EncT>();
    
#if MALICIOUS
    
    Node::m_failure.invalidate();

    Node::m_failure.m_key = "";

    //tracker=RootTracker();

    Node* root_ptr = new Node(tracker);
    tracker.set_root(null_ptr, root_ptr);

#endif
    
    dbconnect = new Connect( "localhost", "root", "letmein","cryptdb", 3306);	
}


template<class EncT>
tree<EncT>::~tree(){
    delete_nodes(root);
    delete root;
}

/****************************/
//Tree rebalancing

template<class EncT>
vector<EncT> 
tree<EncT>::flatten(tree_node<EncT>* node){
    vector<EncT> rtn_keys;

    vector<EncT> tmp;
    if(node->key_in_map( (EncT) NULL)){
	tmp=flatten(node->right[(EncT) NULL]);
	rtn_keys.insert(rtn_keys.end(), tmp.begin(), tmp.end());
    }
    for(int i=0; i<(int) node->keys.size(); i++){
	EncT tmp_key = node->keys[i];
	rtn_keys.push_back(tmp_key);
	if(node->key_in_map(tmp_key)){
	    tmp = flatten(node->right[tmp_key]);
	    rtn_keys.insert(rtn_keys.end(), tmp.begin(), tmp.end() );
	}
    }

    return rtn_keys;			

}

template<class EncT>
tree_node<EncT>* 
tree<EncT>::rebuild(vector<EncT> key_list){
    //Avoid building a node without any values
    if (key_list.size()==0) {return NULL;}

    tree_node<EncT>* rtn_node = new tree_node<EncT>();

    if ((int) key_list.size()<N){
	//Remaining keys fit in one node, return that node
	rtn_node->keys=key_list;
    } else {
	//Borders to divide key_list by
	double base = ((double) key_list.size())/ ((double) N);

	//Fill keys with every base-th key;
	for(int i=1; i<N; i++){
	    rtn_node->keys.push_back(key_list[floor(i*base)]);
	}
	//Handle the subvector rebuilding except for the first and last
	for (int i=1; i<N-1; i++){
	    vector<EncT> subvector;
	    for(int j=floor(i*base)+1; j<floor((i+1)*base); j++){
		subvector.push_back(key_list[j]);
	    }
	    tree_node<EncT>* tmp_child = rebuild(subvector);
	    if (tmp_child!=NULL){
		rtn_node->right[key_list[floor(i*base)]]=tmp_child;
	    }
	}
	//First subvector rebuilding special case
	vector<EncT> subvector;
	for (int j=0; j<floor(base); j++){
	    subvector.push_back(key_list[j]);
	}
	tree_node<EncT>* tmp_child = rebuild(subvector);
	if(tmp_child!=NULL){
	    rtn_node->right[(EncT) NULL]=tmp_child;
	}	
	subvector.clear();
	//Last subvector rebuilding special case
	for(int j=floor((N-1)*base)+1; j< (int) key_list.size(); j++){
	    subvector.push_back(key_list[j]);
	}
	tmp_child = rebuild(subvector);
	if(tmp_child!=NULL){
	    rtn_node->right[key_list[floor((N-1)*base)]]=tmp_child;
	}	

    }
    return rtn_node;

}

template<class EncT>
void 
tree<EncT>::rebalance(tree_node<EncT>* node, uint64_t v, uint64_t nbits,
					 uint64_t path_index, map<EncT, table_entry >  & ope_table){
    if(DEBUG) cout<<"Rebalance"<<endl;

    //table_entry base = ope_table[node->keys[0]];

    vector<EncT> key_list = flatten(node);
/*	if(DEBUG){
	vector<EncT> tmp_list = key_list;
	sort(key_list.begin(), key_list.end());
	if(key_list!=tmp_list) cout<<"Flatten isn't sorted!"<<endl;		
	}*/

    tree_node<EncT>* tmp_node = rebuild(key_list);
    delete_nodes(node);
    node->keys = tmp_node->keys;
    node->right = tmp_node->right;
    delete tmp_node;

    num_rebalances++;

    //Make sure new fresh global_version number to use as versions for
    //newly updated (no longer stale) db values
    //global_version++;

    uint64_t base_v = (v >> (path_index * num_bits));
    uint64_t base_nbits = nbits - path_index * num_bits;

    //Make sure OPE table is updated and correct
    update_ope_table(node, base_v, base_nbits, ope_table);
    clear_db_version();
}

//Used to delete all subtrees of node
template<class EncT>
void 
tree<EncT>::delete_nodes(tree_node<EncT>* node){
    typename map<EncT, tree_node<EncT> *>::iterator it;

    for(it=node->right.begin(); it!=node->right.end(); it++){
	delete_nodes(it->second);
	delete it->second;
    }

}

/****************************/
template<class EncT>
void
tree<EncT>::clear_db_version(){
    string query="UPDATE emp SET version=0 where version=1";
    dbconnect->execute(query);
}

template<class EncT>
void
tree<EncT>::delete_db(table_entry del_entry){
    uint64_t del_ope = del_entry.ope;

    ostringstream o;
    o.str("");
    o.clear();
    o<<del_ope;
    string del_ope_str = o.str();
    /*o.str("");
      o.clear();
      o<<del_version;
      string del_version_str = o.str();
    */
    string query="DELETE FROM emp WHERE ope_enc="+
	del_ope_str;/*+
		      " and version="+del_version_str;*/
    if(DEBUG_COMM) cout<<"Query: "<<query<<endl;
    dbconnect->execute(query);
}

//Update now-stale values in db
template<class EncT>
void 
tree<EncT>::update_db(table_entry old_entry, table_entry new_entry){
    //Takes old ope_table entry and new ope_table entry and calculates
    //corresponding db values, and updates db old vals to new vals

    uint64_t old_ope = old_entry.ope;
    uint64_t new_ope = new_entry.ope;

    ostringstream o;
    o.str("");
    o.clear();
    o<<old_ope;
    string old_ope_str = o.str();
    /*o.str("");
      o.clear();
      o<<old_version;
      string old_version_str = o.str();
    */
    o.str("");
    o.clear();
    o<<new_ope;
    string new_ope_str = o.str();
    /*o.str("");
      o.clear();
      o<<new_version;
      string new_version_str = o.str();	*/

    string query="UPDATE emp SET ope_enc="+
	new_ope_str+
	", version=1 where ope_enc="+
	old_ope_str+
	" and version=0";
    if(DEBUG_COMM) cout<<"Query: "<<query<<endl;
    dbconnect->execute(query);

}

//Ensure table is correct
template<class EncT>
void
tree<EncT>::update_ope_table(tree_node<EncT> *node, uint64_t base_v, uint64_t base_nbits, 
							map<EncT, table_entry >  & ope_table){

    table_entry old_entry;
    for(int i = 0; i < (int) node->keys.size(); i++){
/*		table_entry tmp = base;
		tmp.index = i;*/
	//tmp.version=global_version;
    	old_entry = ope_table[node->keys[i]];
	table_entry new_entry = old_entry ;

	new_entry.ope = compute_ope<EncT>(base_v, base_nbits, i);

	ope_table[node->keys[i]] = new_entry;
	update_db(old_entry , new_entry);
    }

    uint64_t next_v, next_nbits;
	
	next_nbits = base_nbits+num_bits;

    if(node->key_in_map( (EncT) NULL)){
/*		table_entry tmp = base;
		tmp.v = tmp.v<<num_bits;
		tmp.pathlen+=num_bits;
		tmp.index=0;*/

		next_v = base_v<<num_bits;


		update_ope_table(node->right[(EncT) NULL],  next_v, next_nbits, ope_table);
    }
    for(int i = 0; i < (int) node->keys.size(); i++){

		if(node->key_in_map(node->keys[i])){
	/*	    table_entry tmp = base;
		    tmp.v = tmp.v<<num_bits | (i+1);
		    tmp.pathlen+=num_bits;
		    tmp.index=0;*/

		    next_v = (base_v<<num_bits) | (i+1);

		    update_ope_table(node->right[node->keys[i]], next_v, next_nbits, ope_table);	
		}	
    }

}

template<class EncT>
void 
tree<EncT>::print_tree(){
    vector<tree_node<EncT>* > queue;
    queue.push_back(root);
    while(queue.size()!=0){
	tree_node<EncT>* cur_node = queue[0];
	if(cur_node==NULL){
	    queue.erase(queue.begin());
	    cout<<endl;
	    continue;
	}
	cout<<cur_node->height()-1<<": ";
	if(cur_node->key_in_map( (EncT)NULL)){
	    queue.push_back(cur_node->right[ (EncT)NULL]);
	}	
	for(int i=0; i< (int) cur_node->keys.size(); i++){
	    EncT key = cur_node->keys[i];
	    cout<<key<<", ";
	    if(cur_node->key_in_map(key)){
		queue.push_back(cur_node->right[key]);
	    }
	}	
	cout<<endl;
	queue.push_back((EncT) NULL);
	queue.erase(queue.begin());
    }

}

template<class EncT>
tree_node<EncT>* 
tree<EncT>::findScapegoat( vector<tree_node<EncT>* > path , uint64_t & path_index){
    int tmp_size = 0;
    tree_node<EncT>* last = NULL;
    typename map<EncT, tree_node<EncT> *>::iterator it;

    int childSize=0;
    vector<int> childSizes;

    //Move along path until you find the scapegoat node
    for(int i=0; i< (int) path.size(); i++){
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

		for(int i=0; i< (int) childSizes.size(); i++){
		    if(childSizes[i] > alpha*tmp_size) {
		    	path_index = i;
		    	return cur_node;
		    }
		}

		childSizes.clear();

    }
    path_index = path.size()-1;
    return root;

}

template<class EncT>
struct successor {
    uint64_t succ_v;
    uint64_t succ_nbits;
    tree_node<EncT>* succ_node;
};

template<class EncT>
successor<EncT>
tree<EncT>::find_succ(tree_node<EncT>* node, uint64_t v, uint64_t nbits){
    if(node->key_in_map((EncT) NULL)){
	return find_succ(node->right[(EncT) NULL], (v<<num_bits), nbits+num_bits);
    }else{
	successor<EncT> succ;
	succ.succ_v = v;
	succ.succ_nbits = nbits;
	succ.succ_node = node;
	return succ;
    }
}

template<class EncT>
struct predecessor{
    uint64_t pred_v;
    uint64_t pred_nbits;
    uint64_t pred_index;
    tree_node<EncT>* pred_node;
};

template<class EncT>
predecessor<EncT>
tree<EncT>::find_pred(tree_node<EncT>* node, uint64_t v, uint64_t nbits){
    int max_index = node->keys.size()-1;
    if(node->key_in_map(node->keys[max_index])){
	if(max_index!=N-2) {
	    cout<<"Error in pred, max_index not N-1"<<endl;
	    exit(1);
	}
	return find_pred(node->right[node->keys[max_index]], (v<<num_bits | (max_index+1)), nbits+num_bits);
    }else{
	predecessor<EncT> pred;
	pred.pred_v = v;
	pred.pred_nbits = nbits;
	pred.pred_index = max_index;
	pred.pred_node = node;
	return pred;
    }
}

/*template<class EncT>
string
tree<EncT>::delete_index(uint64_t v, uint64_t nbits, uint64_t index){
    if(DEBUG) cout<<"Deleting index at v="<<v<<" nbits="<<nbits<<" index="<<index<<endl;
    EncT deleted_val = tree_delete(root, v, nbits, index, nbits, false);
    clear_db_version();
    num_nodes--;

    //Merkle tree delete
    stringstream s;
    s << deleted_val;
    string delval_str = s.str();

#if MALICIOUS
    Elem desired;
    desired.m_key = delval_str;
    UpdateMerkleProof dmp;
    tracker.get_root()->tree_delete(desired, dmp);

    //Test merkle delete

    Node * last;
    Elem result = tracker.get_root()->search(desired, last);
    assert_s(result == Node::m_failure, "found element that should have been deleted");		
    /////
    ////

    s.str("");
    s.clear();

    string merkle_root = tracker.get_root()->merkle_hash;

    if(DEBUG_BTREE) cout<<"Delete new merkle hash="<<merkle_root<<endl;
    s<<merkle_root<<" hash_end ";
    s<<dmp;

    if(DEBUG_BTREE) cout<<"Delete dmp="<<dmp<<endl;

    if(DEBUG_BTREE) cout<<"Delete msg="<<s.str()<<endl;
#endif

    if(root==NULL) return s.str();
	
    if(num_nodes<alpha*max_size){
	if(DEBUG) cout<<"Delete rebalance"<<endl;
	rebalance(root, (uint64_t) 0, (uint64_t) 0, (uint64_t) 0, ope_table);
	max_size=num_nodes;
    }

    return s.str();

}*/

//v should just be path to node, not including index. swap indicates whether val being deleted was swapped, should be false for top-lvl call
/*template<class EncT>
EncT
tree<EncT>::tree_delete(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, uint64_t pathlen, bool swap){
    if (DEBUG) cout<<"Calling tree_delete with "<<v<< " "<<nbits<<" "<<index<<endl;
    if (nbits == 0){
	EncT del_val = node->keys[index];
	if(node->right.empty()){
	    //Value is being deleted from a leaf node
	    if(DEBUG) cout<<"Deleting leaf node val"<<endl;
	    typename vector<EncT>::iterator it;
	    it=node->keys.begin();
	    node->keys.erase(it+index);
	    if(!swap) {
		delete_db(ope_table[del_val]);
		ope_table.erase(del_val);
	    }
	    if(node->keys.size()==0){
		if(node==root) {
		    if(DEBUG) cout<<"Deleting root!"<<endl;
		    root=NULL;
		    if(DEBUG) cout<<"Root reset to null"<<endl;
		}
		delete node;
		return del_val;

	    }
	    //NEED TO UPDATE OPE TABLE/DATABASE

	    //Incr puts new entry in ope table at unique version #.
	    //global_version++;

	    for(int i=(int)index; i< (int) node->keys.size(); i++){
		table_entry old_entry = ope_table[node->keys[i]];
		ope_table[node->keys[i]].index = i;
		//ope_table[node->keys[i]].version=global_version;
		table_entry update_entry = ope_table[node->keys[i]];
		//Only keys after deleted one need updating
		update_db(old_entry, update_entry);
	    }

	}else if(node->key_in_map(del_val)){
	    if(DEBUG) cout<<"Deleting val by swapping with succ"<<endl;
	    //Find node w/ succ, swap succ_val with curr_val, then delete succ_val
	    successor<EncT> succ = find_succ(node->right[del_val], (v<<num_bits | (index+1)), pathlen+num_bits);
	    node->keys[index]=succ.succ_node->keys[0]; //swap
	    //Fixing right map
	    node->right[node->keys[index]]=node->right[del_val];
	    node->right.erase(del_val);

	    //NEED TO UPDATE OPE TABLE/DATABASE
	    //Only need to update the new value being swapped in.
	    //global_version++;
	    if(!swap) {
		delete_db(ope_table[del_val]);
		ope_table.erase(del_val);
	    }
	    table_entry old_entry = ope_table[node->keys[index]];
	    ope_table[node->keys[index]].v = v;
	    ope_table[node->keys[index]].pathlen=pathlen;
	    ope_table[node->keys[index]].index=index;
	    //ope_table[node->keys[index]].version=global_version;
	    table_entry update_entry = ope_table[node->keys[index]];
	    update_db(old_entry, update_entry);

	    tree_delete(node, succ.succ_v, succ.succ_nbits-pathlen, 0, succ.succ_nbits, true); //TODO: Could optimize by just calling delete on next right node?



	}else{
	    //This case means node->right has no entry for del_val
	    bool has_succ = false;
	    for(int i=(int) index+1; i<N-1; i++){
		if(node->key_in_map(node->keys[i])) {has_succ=true; break;}
	    }
	    if(has_succ){
		if(DEBUG) cout<<"Deleting val by shifting towards succ"<<endl;
		node->keys[index]= node->keys[index+1];
		//NEED TO UPDATE OPE TABLE/DATABASE
		//global_version++;
		if(!swap) {
		    delete_db(ope_table[del_val]);
		    ope_table.erase(del_val);
		}
		//Updating db entry for newly moved value
		table_entry old_entry = ope_table[node->keys[index]];
		ope_table[node->keys[index]].index=index;
		//ope_table[node->keys[index]].version=global_version;
		table_entry update_entry = ope_table[node->keys[index]];
		update_db(old_entry, update_entry);


		tree_delete(node, v, nbits, index+1, pathlen, true);
	    }else{
		//Must find pred
		EncT right_parent;
		if(index==0){
		    if(!node->key_in_map((EncT) NULL)){cout<<"Error in delete, child is truly leaf"<<endl; exit(1);}
		    right_parent = (EncT) NULL;
		}else{
		    right_parent = node->keys[index-1];
		}

		if(node->key_in_map(right_parent)){
		    if(DEBUG) cout<<"Deleting val by swapping with pred"<<endl;
		    predecessor<EncT> pred = find_pred(node->right[right_parent], (v<<num_bits | index), pathlen+num_bits);
		    node->keys[index]=pred.pred_node->keys[pred.pred_index];

		    //NEED TO UPDATE OPE TABLE/DATABASE	
		    //Only need to update the new value being swapped in.
		    //No need to change right. del_val had no mapping in right anyways.
		    //global_version++;
		    if(!swap){
			delete_db(ope_table[del_val]);
			ope_table.erase(del_val);
		    }
		    table_entry old_entry = ope_table[node->keys[index]];
		    ope_table[node->keys[index]].v = v;
		    ope_table[node->keys[index]].pathlen=pathlen;
		    ope_table[node->keys[index]].index=index;
		    //ope_table[node->keys[index]].version=global_version;
		    table_entry update_entry = ope_table[node->keys[index]];
		    update_db(old_entry, update_entry);


		    tree_delete(node, pred.pred_v, pred.pred_nbits-pathlen, pred.pred_index, pred.pred_nbits, true); //TODO: Could optimize by just calling delete on next right node?
				
		}else{
		    if(DEBUG) cout<<"Deleting val by shifting towards pred"<<endl;
		    if(index==0) {
			cout<<"Error in delete, child should be leaf!"<<endl;
			exit(1);
		    }
		    node->keys[index]=node->keys[index-1];

		    //NEED TO UPDATE OPE TABLE/DATABASE
		    //global_version++;
		    if(!swap){
			delete_db(ope_table[del_val]);
			ope_table.erase(del_val);
		    }
		    table_entry old_entry = ope_table[node->keys[index]];
		    ope_table[node->keys[index]].index=index;
		    //ope_table[node->keys[index]].version=global_version;
		    table_entry update_entry = ope_table[node->keys[index]];
		    update_db(old_entry, update_entry);		
								
		    tree_delete(node, v, nbits, index-1, pathlen, true);
			
		}

				
	    }
	}
	if(DEBUG) print_tree();
	return del_val;
    }

    int key_index = (int) (v&(mask<<(nbits-num_bits)))>>(nbits-num_bits);

    EncT key;

    //Protocol set: index 0 is NULL (branch to node with lesser elements)
    //Else, index is 1+(key's index in node's key-vector)
    if(key_index==0) key= (EncT) NULL;
    else if(key_index<N) key = node->keys[key_index-1];
    else{cout<<"Delete fail, key_index not legal "<<key_index<<endl; exit(1);}

    //cout<<"Key: "<<key<<endl;

    //See if pointer to next node to be checked 
    if(!node->key_in_map(key)){
	cout<<"Delete fail, wrong condition to traverse to new node"<<endl;
	cout<<"v: "<<v<<" nbits:"<<nbits<<" index:"<<index<<" pathlen:"<<pathlen<<endl;
	exit(1);
    }
    tree_node<EncT>* next_node = node->right[key];
    if(nbits== (uint64_t) num_bits && next_node->keys.size()==(uint64_t) 1){
	if(index!=0) {
	    cout<<"Deleting last val at node but index!=0??"<<index<<endl;
	    cout<<v<< " "<<nbits<<" "<<index<<endl;
	    exit(1);
	}
	node->right.erase(key);    			
    }    
    return tree_delete(next_node, v, nbits-num_bits, index, pathlen, swap);
}*/

template<class EncT>
string
tree<EncT>::insert(uint64_t v, uint64_t nbits, uint64_t index, EncT encval, map<EncT, table_entry >  & ope_table){
    //If root doesn't exist yet, create it, then continue insertion
    if(!root){
	if(DEBUG) cout<<"Inserting root"<<endl;
	root=new tree_node<EncT>();
    }
    vector<tree_node<EncT> * > path=tree_insert(root, v, nbits, index, encval, nbits, ope_table);
    clear_db_version();

    //Merkle tree insert
    stringstream s;
    s << encval;
    string encval_str = s.str();


#if MALICIOUS
    Elem elem;
    elem.m_key = encval_str;
    elem.m_payload = encval_str+" hi you";
    UpdateMerkleProof imp;
    tracker.get_root()->tree_insert(elem, imp);


    //Test merkle tree
    Elem desired;
    desired.m_key = encval_str;
    Node * last;
    Elem& result = tracker.get_root()->search(desired, last);
    assert_s(desired.m_key == result.m_key, "could not find val that should be there");	
	
    string merkle_root = tracker.get_root()->merkle_hash;
    MerkleProof proof = get_search_merkle_proof(last);
    assert_s(verify_merkle_proof(proof, merkle_root), "failed to verify merkle proof");   
    ////
    ////
    s.str("");
    s.clear();
    s<<merkle_root<<" hash_end ";
    s<<imp;


    if(DEBUG_BTREE) cout<<"Insert merkle hash="<<merkle_root<<endl;
    if(DEBUG_BTREE) cout<<"Insert imp="<<imp<<endl;
    if(DEBUG_BTREE) cout<<"Insert msg="<<s.str()<<endl;
#endif


    double height= (double) path.size();

    if (height==0) return s.str();

    //if(DEBUG) print_tree();

    if(height> log(num_nodes)/log(((double)1.0)/alpha)+1 ){
    	uint64_t path_index;
		tree_node<EncT>* scapegoat = findScapegoat(path, path_index);
		rebalance(scapegoat, v, nbits, path_index, ope_table);
		if(DEBUG) cout<<"Insert rebalance "<<height<<": "<<num_nodes<<endl;
		if(scapegoat==root) max_size=num_nodes;
		//if(DEBUG) print_tree();
    } 

    return s.str();

}

template<class EncT>
vector<tree_node<EncT>* >
tree<EncT>::tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, EncT encval, uint64_t pathlen, map<EncT, table_entry >  & ope_table){

    vector<tree_node<EncT>* > rtn_path;

    //End of the insert line, check if you can insert
    //Assumes v and nbits were from a lookup of an insertable node
    if(nbits==0){
	if(node->keys.size()> N-2) {
	    cout<<"Insert fail, found full node"<<endl;
	    exit(1);
	}else{
	    if(DEBUG) cout<<"Found node, inserting into index "<<index<<endl;

        assert_s(node->keys[index] != encval, "attempting to insert same value into tree");

	    typename vector<EncT>::iterator it;
	    it=node->keys.begin();

	    node->keys.insert(it+index, encval);
	    //sort(node->keys.begin(), node->keys.end());
	    num_nodes++;
	    max_size=max(num_nodes, max_size);
	    rtn_path.push_back(node);

	    table_entry new_entry;
	    /*new_entry.v = v;
	    new_entry.pathlen = pathlen;
	    new_entry.index = index;*/
	    new_entry.ope = compute_ope<EncT>(v, pathlen, index);
	    new_entry.refcount = 1;
	    //new_entry.version = global_version;
	    ope_table[encval] = new_entry;

	    //Incr puts new entry in ope table at unique version #.
	    //global_version++;

	    for(int i=(int)index+1; i< (int) node->keys.size(); i++){
			table_entry old_entry = ope_table[node->keys[i]];
			//ope_table[node->keys[i]].index = i;
			ope_table[node->keys[i]].ope = compute_ope<EncT>(v, pathlen, i);
			//ope_table[node->keys[i]].version=global_version;
			table_entry update_entry = ope_table[node->keys[i]];
			//Only keys after newly inserted one need updating
			update_db(old_entry, update_entry);

	    }
			
	}
	return rtn_path;
    }

    int key_index = (int) (v&(mask<<(nbits-num_bits)))>>(nbits-num_bits);

    EncT key;

    //Protocol set: index 0 is NULL (branch to node with lesser elements)
    //Else, index is 1+(key's index in node's key-vector)
    if (key_index==0) {
	key= (EncT) NULL;
    }
    else if (key_index < N) {
	key = node->keys[key_index-1];
    }
    else {
	cout<<"Insert fail, key_index not legal"<<endl; exit(1);
    }

    //cout<<"Key: "<<key<<endl;

    //See if pointer to next node to be checked 
    if(!node->key_in_map(key)){
	if (nbits == (uint64_t) num_bits && ((int) node->keys.size())==N-1) {
	    if(DEBUG) cout << "Creating new node" << endl;
	    node->right[key] = new tree_node<EncT>();
	} else{
	    cout<<"Insert fail, wrong condition to create new node child"<< endl; exit(1);}
    }
    rtn_path = tree_insert(node->right[key], v, nbits-num_bits, index, encval, pathlen, ope_table);
    rtn_path.push_back(node);
    return rtn_path;
}

template<class EncT>
bool
tree<EncT>::test_tree(tree_node<EncT>* cur_node){
    if(test_node(cur_node)!=true) return false;

    typename map<EncT, tree_node<EncT> *>::iterator it;

    //Recursively check tree at this node and all subtrees
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
    //Make sure num of keys is b/w 0 and N exclusive 
    //(no node should have no keys)
    if(sorted_keys.size()==0) {
	cout<<"Node with no keys"<<endl;
	return false;
    }
    if(sorted_keys.size()>N-1) {
	cout<<"Node with too many keys"<<endl;
	return false;
    }

    //Check that node's keys are sorted <- No longer true with DET enc
/*	sort(sorted_keys.begin(), sorted_keys.end());
	if(sorted_keys!=cur_node->keys){
	cout<<"Node keys in wrong order"<<endl;
	}*/

    //If num of keys < N-1, then node is a leaf node. 
    //Right map should have no entries.
    if(sorted_keys.size()<N-1){
	if(cur_node->key_in_map( (EncT) NULL)) return false;
	for(int i=0; i< (int) sorted_keys.size(); i++){
	    if(cur_node->key_in_map(sorted_keys[i])) return false;
	}
    }

    return true;

}

//Input: Node, and low and high end of range to test key values at
template<class EncT>
bool
tree<EncT>::test_vals(tree_node<EncT>* cur_node, EncT low, EncT high){
    //Make sure all keys at node are in proper range
    for(int i=0; i< (int) cur_node->keys.size(); i++){
	EncT this_key = cur_node->keys[i];
	if ( this_key <= low || this_key >= high) {
	    cout << "Values off "<< this_key << ": " << low << ", " << high << endl;
	    return false;
	}
    }

    typename map<EncT, tree_node<EncT> *>::iterator it;

    for(it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
	if(test_vals(it->second, low, high)!=true) return false;
    }
    return true;
}

/*
 * Explicitly instantiate the tree template for various ciphertext types.
 */
template class tree<uint64_t>;
template class tree<uint32_t>;
template class tree<uint16_t>;

template<class EncT>
void
Server<EncT>::interaction(EncT ciph,
			  uint & rindex, uint & nbits,
			  uint64_t & ope_path,
			  bool & requals) {

    tree_node<EncT> * curr = ope_tree.root;

    ope_path = 0;
    nbits = 0;

     while (true) {	
    	// create message and send it to client
    	stringstream msg;
    	msg.clear();
    	msg << MsgType::INTERACT_FOR_LOOKUP << " ";
    	msg << ciph << " ";

    	uint len = curr->keys.size();
    	msg << len << " ";

    	for (uint i = 0; i < len; i++) {
    	    msg << curr->keys[i] << " ";
    	}

    	string _reply = send_receive(sock_cl, msg.str());
    	istringstream reply(_reply);

    	uint index;
    	bool equals = false;
    	reply >> index >> equals;

    	assert_s(len > index, "index returned by client is incorrect");

    	tree_node<EncT> * node  =curr->right[index];
    	if (equals || (!node && len<N-1) ) {
    	    // done: found node match or empty spot
    	    requals = equals;
            rindex = index;
    	    return;
    	}
        else if ( !node && len==N-1) {
            //done: curr_node is full though, need new node
            ope_path = (ope_path << num_bits) | index;
            nbits += num_bits;          
            rindex = 0;
            requals = equals;
            return;
        }
    	curr = node;
    	ope_path = (ope_path << num_bits) | index;
    	nbits += num_bits;
    }
    
}

template<class EncT>
void 
Server<EncT>::handle_enc(int csock, istringstream & iss, bool do_ins) {
   
    uint64_t ciph;
    iss >> ciph;
    if(DEBUG_COMM) cout<< "ciph: "<< ciph << endl;

    string ope;
    
    auto ts = ope_table.find(ciph);
    if (ts != ope_table.end()) { // found in OPE Table
    	if (DEBUG) {cerr << "found in ope table"; }
    	// if this is an insert, increase ref count
    	if (do_ins) {
    	    ts->second.refcount++;
    	}
	ope = opeToStr(ts->second.ope);
   	
    } else { // not found in OPE Table
    	if (DEBUG) {cerr << "not in ope table \n"; }

    	uint index = 0;
    	uint64_t ope_path = 0;
    	bool equals = false;
    	uint nbits = 0;

    	interaction(ciph, index, nbits, ope_path, equals);

        assert_s(!equals, "equals should always be false");

    	
    	if (DEBUG) {cerr << "new ope_enc has "<< " path " << ope_path
    			 << " index " << index << "\n";}
    	if (do_ins) {
    	    // insert in OPE Tree
            // ope_insert also updates ope_table
    	    ope_tree.insert(ope_path, nbits, index, ciph, ope_table);

            ts = ope_table.find( ciph);
            assert_s( ts != ope_table.end() , "after insert, value must be in ope_table");
            ope = opeToStr(ts->second.ope);            
        }else{

            uint64_t ope_enc = compute_ope<EncT>(ope_path, nbits, index);

            //Subtract 1 b/c if ope points to existing value in tree, want this ope value
            //to be less than 
            ope = opeToStr(ope_enc-1);            
        }
    }
    assert_s(send(csock, ope.c_str(), ope.size(), 0) != (int)ope.size(),
	     "problem with send");
    
}

template<class EncT>
void
Server<EncT>::dispatch( int csock, istringstream & iss) {
   
    MsgType msgtype;
    iss >> msgtype;

    if (DEBUG_COMM) {cerr << "message type is " << mtnames[(int) msgtype] << endl;}

    switch (msgtype) {
    case MsgType::ENC_INS: {
	return handle_enc(csock, iss, true);
    }
    case MsgType::QUERY: {
	return handle_enc(csock, iss, false);
    }
    default: {}
    }

    assert_s(false, "invalid message type in dispatch");
    
}

template<class EncT>
Server<EncT>::Server() {
    sock_cl = create_and_connect(OPE_CLIENT_HOST, OPE_CLIENT_PORT);
    sock_udf = create_and_bind(OPE_SERVER_PORT);
}

template<class EncT>
Server<EncT>::~Server() {
    close(sock_cl);
    close(sock_udf);
}



int main(int argc, char **argv){
    
    cerr<<"Starting tree server \n";
    Server<uint64_t> server;
    
    //Start listening
    int listen_rtn = listen(server.sock_udf, 10);
    if (listen_rtn < 0) {
	cerr<<"Error listening to socket"<<endl;
    }
    cerr<<"Listening \n";

    socklen_t addr_size = sizeof(sockaddr_in);
    struct sockaddr_in sadr;
    
    int csock = accept(server.sock_udf, (struct sockaddr*) &sadr, &addr_size);
    assert_s(csock >= 0, "Server accept failed!");
    cerr << "Server accepted connection with udf \n";

    uint buflen = 10240;
    char buffer[buflen];

    while (true) {
    	cerr<<"Listening..."<<endl;

        memset(buffer, 0, buflen);

	uint len;
        assert_s((len = recv(csock, buffer, buflen, 0)) > 0, "received 0 bytes");

        if (DEBUG_COMM) {cerr << "received msg " << buffer << endl;}
        istringstream iss(buffer);
	
	//Pass connection and messages received to handle_client
	server.dispatch(csock, iss);

    }

    close(csock);
    cerr << "Server exits!\n";
}
