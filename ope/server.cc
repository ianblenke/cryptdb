#include "server.hh"
#include <exception>
#include <utility>
#include <stdlib.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>
#include <sys/types.h>

template<class EncT>
struct tree_node
{
	vector<EncT> keys;
	map<EncT, tree_node *> right;

	tree_node(){}

	~tree_node(){
		keys.clear();
		right.clear();
	}

	//Returns true if node's right map contains key (only at non-leaf nodes)
	bool key_in_map(EncT key){
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
	if(key_list.size()==0) return NULL;

	tree_node<EncT>* rtn_node = new tree_node<EncT>();

	if((int) key_list.size()<N){
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
			tree_node<EncT>* tmp_child = rebuild(subvector);
			if(tmp_child!=NULL){
				rtn_node->right[key_list[floor(i*base)]]=tmp_child;
			}
		}
		//First subvector rebuilding special case
		vector<EncT> subvector;
		for(int j=0; j<floor(base); j++){
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
tree<EncT>::rebalance(tree_node<EncT>* node){
	if(DEBUG) cout<<"Rebalance"<<endl;

	table_storage base = ope_table[node->keys[0]];

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
	global_version++;


	//Make sure OPE table is updated and correct
	update_ope_table(node, base);
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
tree<EncT>::delete_db(table_storage del_entry){
	uint64_t del_v, del_nbits, del_version;
	del_v=(del_entry.v << num_bits) | del_entry.index;
	del_nbits=del_entry.pathlen+num_bits;
	del_version=del_entry.version;	

	uint64_t del_ope = (del_v<<(64-del_nbits)) | (mask<<(64-num_bits-del_nbits));

	ostringstream o;
	o.str("");
	o.clear();
	o<<del_ope;
	string del_ope_str = o.str();
	o.str("");
	o.clear();
	o<<del_version;
	string del_version_str = o.str();

	string query="DELETE FROM emp WHERE ope_enc="+
					del_ope_str+
					" and version="+del_version_str;
	if(DEBUG_COMM) cout<<"Query: "<<query<<endl;
	dbconnect->execute(query);
}

//Update now-stale values in db
template<class EncT>
void 
tree<EncT>::update_db(table_storage old_entry, table_storage new_entry){
	//Takes old ope_table entry and new ope_table entry and calculates
	//corresponding db values, and updates db old vals to new vals
	uint64_t old_v, old_nbits, old_version;
	uint64_t new_v, new_nbits, new_version;

	old_v=(old_entry.v << num_bits) | old_entry.index;
	old_nbits=old_entry.pathlen+num_bits;
	old_version=old_entry.version;

	new_v=(new_entry.v << num_bits) | new_entry.index;
	new_nbits=new_entry.pathlen+num_bits;
	new_version=new_entry.version;	

	uint64_t old_ope = (old_v<<(64-old_nbits)) | (mask<<(64-num_bits-old_nbits));
	uint64_t new_ope = (new_v<<(64-new_nbits)) | (mask<<(64-num_bits-new_nbits));

	ostringstream o;
	o.str("");
	o.clear();
	o<<old_ope;
	string old_ope_str = o.str();
	o.str("");
	o.clear();
	o<<old_version;
	string old_version_str = o.str();

	o.str("");
	o.clear();
	o<<new_ope;
	string new_ope_str = o.str();
	o.str("");
	o.clear();
	o<<new_version;
	string new_version_str = o.str();	

	string query="UPDATE emp SET ope_enc="+
					new_ope_str+
					", version="+new_version_str+" where ope_enc="+
					old_ope_str+
					" and version="+old_version_str;
	if(DEBUG_COMM) cout<<"Query: "<<query<<endl;
	dbconnect->execute(query);

}

//Ensure table is correct
template<class EncT>
void
tree<EncT>::update_ope_table(tree_node<EncT> *node, table_storage base){

	table_storage old_table_entry;
	for(int i=0; i< (int) node->keys.size(); i++){
		table_storage tmp = base;
		tmp.index=i;
		tmp.version=global_version;

		old_table_entry=ope_table[node->keys[i]];

		ope_table[node->keys[i]] = tmp;
		update_db(old_table_entry, tmp);
	}
	if(node->key_in_map( (EncT) NULL)){
		table_storage tmp = base;
		tmp.v = tmp.v<<num_bits;
		tmp.pathlen+=num_bits;
		tmp.index=0;
		update_ope_table(node->right[(EncT) NULL], tmp);
	}
	for(int i=0; i< (int) node->keys.size(); i++){
		if(node->key_in_map(node->keys[i])){
			table_storage tmp = base;
			tmp.v = tmp.v<<num_bits | (i+1);
			tmp.pathlen+=num_bits;
			tmp.index=0;
			update_ope_table(node->right[node->keys[i]], tmp);	
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
tree<EncT>::findScapegoat( vector<tree_node<EncT>* > path ){
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
			if(childSizes[i] > alpha*tmp_size) return cur_node;
		}

		childSizes.clear();

	}

	return root;

}

template<class EncT>
struct successor{
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
		if(max_index!=N-1) {
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

template<class EncT>
string
tree<EncT>::delete_index(uint64_t v, uint64_t nbits, uint64_t index){
	if(DEBUG) cout<<"Deleting index at v="<<v<<" nbits="<<nbits<<" index="<<index<<endl;
	EncT deleted_val = tree_delete(root, v, nbits, index, nbits, false);
	num_nodes--;

	//Merkle tree delete
	stringstream s;
	s << deleted_val;
	string delval_str = s.str();

	Elem desired;
	desired.m_key = delval_str;
	DelMerkleProof dmp;
	/*bool deleted = */tracker.get_root()->tree_delete(desired, dmp);

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
    s<<merkle_root+" hash_end ";
    s<<dmp;

    if(DEBUG_BTREE) cout<<"Delete dmp="<<dmp<<endl;
	if(root==NULL) return s.str();
	
	if(num_nodes<alpha*max_size){
		if(DEBUG) cout<<"Delete rebalance"<<endl;
		rebalance(root);
		max_size=num_nodes;
	}
	if(DEBUG_BTREE) cout<<"Delete msg="<<s.str()<<endl;
	return s.str();
}

//v should just be path to node, not including index. swap indicates whether val being deleted was swapped, should be false for top-lvl call
template<class EncT>
EncT
tree<EncT>::tree_delete(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, uint64_t pathlen, bool swap){
	if(DEBUG) cout<<"Calling tree_delete with "<<v<< " "<<nbits<<" "<<index<<endl;
	if(nbits==0){
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
			global_version++;

			for(int i=(int)index; i< (int) node->keys.size(); i++){
				table_storage old_entry = ope_table[node->keys[i]];
				ope_table[node->keys[i]].index = i;
				ope_table[node->keys[i]].version=global_version;
				table_storage update_entry = ope_table[node->keys[i]];
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
			global_version++;
			if(!swap) {
				delete_db(ope_table[del_val]);
				ope_table.erase(del_val);
			}
			table_storage old_entry = ope_table[node->keys[index]];
			ope_table[node->keys[index]].v = v;
			ope_table[node->keys[index]].pathlen=pathlen;
			ope_table[node->keys[index]].index=index;
			ope_table[node->keys[index]].version=global_version;
			table_storage update_entry = ope_table[node->keys[index]];
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
				global_version++;
				if(!swap) {
					delete_db(ope_table[del_val]);
					ope_table.erase(del_val);
				}
				//Updating db entry for newly moved value
				table_storage old_entry = ope_table[node->keys[index]];
				ope_table[node->keys[index]].index=index;
				ope_table[node->keys[index]].version=global_version;
				table_storage update_entry = ope_table[node->keys[index]];
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
					global_version++;
					if(!swap){
						delete_db(ope_table[del_val]);
						ope_table.erase(del_val);
					}
					table_storage old_entry = ope_table[node->keys[index]];
					ope_table[node->keys[index]].v = v;
					ope_table[node->keys[index]].pathlen=pathlen;
					ope_table[node->keys[index]].index=index;
					ope_table[node->keys[index]].version=global_version;
					table_storage update_entry = ope_table[node->keys[index]];
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
					global_version++;
					if(!swap){
						delete_db(ope_table[del_val]);
						ope_table.erase(del_val);
					}
					table_storage old_entry = ope_table[node->keys[index]];
					ope_table[node->keys[index]].index=index;
					ope_table[node->keys[index]].version=global_version;
					table_storage update_entry = ope_table[node->keys[index]];
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
}

template<class EncT>
string
tree<EncT>::insert(uint64_t v, uint64_t nbits, uint64_t index, EncT encval){
	//If root doesn't exist yet, create it, then continue insertion
	if(!root){
		if(DEBUG) cout<<"Inserting root"<<endl;
		root=new tree_node<EncT>();
	}
	vector<tree_node<EncT> * > path=tree_insert(root, v, nbits, index, encval, nbits);

	//Merkle tree insert
	stringstream s;
	s << encval;
	string encval_str = s.str();

	Elem elem;
	elem.m_key = encval_str;
	elem.m_payload = encval_str+" hi you";
	InsMerkleProof imp;
	tracker.get_root()->tree_insert(elem, imp);

	//Test merkle tree
    Elem desired;
    desired.m_key = encval_str;
    Node * last;
    Elem& result = tracker.get_root()->search(desired, last);
    assert_s(desired.m_key == result.m_key, "could not find val that should be there");	
	
	string merkle_root = tracker.get_root()->merkle_hash;
	MerkleProof proof = get_merkle_proof(last);
	assert_s(verify_merkle_proof(proof, merkle_root), "failed to verify merkle proof");   
	////
	////
	s.str("");
	s.clear();
	s<<merkle_root+" hash_end ";
	s<<imp;

	if(DEBUG_BTREE) cout<<"Insert merkle hash="<<merkle_root<<endl;
	if(DEBUG_BTREE) cout<<"Insert imp="<<imp<<endl;
	if(DEBUG_BTREE) cout<<"Insert msg="<<s.str()<<endl;

	double height= (double) path.size();

	if (height==0) return s.str();

	//if(DEBUG) print_tree();

	if(height> log(num_nodes)/log(((double)1.0)/alpha)+1 ){
		tree_node<EncT>* scapegoat = findScapegoat(path);
		rebalance(scapegoat);
		if(DEBUG) cout<<"Insert rebalance "<<height<<": "<<num_nodes<<endl;
		if(scapegoat==root) max_size=num_nodes;
		//if(DEBUG) print_tree();
	} 

	return s.str();

}

template<class EncT>
vector<tree_node<EncT>* >
tree<EncT>::tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, EncT encval, uint64_t pathlen){

	vector<tree_node<EncT>* > rtn_path;

	//End of the insert line, check if you can insert
	//Assumes v and nbits were from a lookup of an insertable node
	if(nbits==0){
		if(node->keys.size()>N-2) {
			cout<<"Insert fail, found full node"<<endl;
			exit(1);
		}else{
			if(DEBUG) cout<<"Found node, inserting into index "<<index<<endl;
			typename vector<EncT>::iterator it;
			it=node->keys.begin();

			node->keys.insert(it+index, encval);
			//sort(node->keys.begin(), node->keys.end());
			num_nodes++;
			max_size=max(num_nodes, max_size);
			rtn_path.push_back(node);

			table_storage new_entry;
			new_entry.v = v;
			new_entry.pathlen = pathlen;
			new_entry.index = index;
			new_entry.version = global_version;
			ope_table[encval] = new_entry;

			//Incr puts new entry in ope table at unique version #.
			global_version++;

			for(int i=(int)index+1; i< (int) node->keys.size(); i++){
				table_storage old_entry = ope_table[node->keys[i]];
				ope_table[node->keys[i]].index = i;
				ope_table[node->keys[i]].version=global_version;
				table_storage update_entry = ope_table[node->keys[i]];
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
    if(key_index==0) key= (EncT) NULL;
	else if(key_index<N) key = node->keys[key_index-1];
	else{ cout<<"Insert fail, key_index not legal"<<endl;exit(1);}

	//cout<<"Key: "<<key<<endl;

	//See if pointer to next node to be checked 
    if(!node->key_in_map(key)){
	    if (nbits == (uint64_t) num_bits && ((int) node->keys.size())==N-1) {
	    	if(DEBUG) cout<<"Creating new node"<<endl;
	    	node->right[key] = new tree_node<EncT>();
	    }else{cout<<"Insert fail, wrong condition to create new node child"<<endl; exit(1);}
    }
    rtn_path = tree_insert(node->right[key], v, nbits-num_bits, index, encval, pathlen);
    rtn_path.push_back(node);
    return rtn_path;
}

template<class EncT>
tree_node<EncT> *
tree<EncT>::tree_lookup(tree_node<EncT> *node, uint64_t v, uint64_t nbits) const
{

    if (nbits == 0) {
        return node;
    }

    int key_index = (int) (v&(mask<<(nbits-num_bits)))>>(nbits-num_bits);

    EncT key;

    if(key_index==0) key=(EncT) NULL;
	else if(key_index<N) key = node->keys[key_index-1];
	else return 0;

    if(!node->key_in_map(key)){
    	if(DEBUG) cout<<"Key not in map"<<endl;
    	return 0;
    }
    return tree_lookup(node->right[key], v, nbits-num_bits);
}

template<class EncT>
vector<EncT>
tree<EncT>::lookup(uint64_t v, uint64_t nbits) const
{
	tree_node<EncT>* n;
	//Handle if root doesn't exist yet
	if(!root){
		if(DEBUG) cout<<"First lookup no root"<<endl;
		n=0;
	}else{
		n  = tree_lookup(root, v, nbits);
	}

    if(n==0){
    	if(DEBUG) cout<<"NOPE! "<<v<<": "<<nbits<<endl;    	
    	throw ope_lookup_failure();
    }
    return n->keys;

}

template<class EncT>
table_storage
tree<EncT>::lookup(EncT xct){
    if(ope_table.find(xct)!=ope_table.end()){
        if(DEBUG) cout <<"Found "<<xct<<" in table with v="<<ope_table[xct].v<<" nbits="<<ope_table[xct].pathlen<<" index="<<ope_table[xct].index<<endl;
        return ope_table[xct];
    }
    table_storage negative;
    negative.v=-1;
    negative.pathlen=-1;
    negative.index=-1;
    negative.version=global_version;
    return negative;
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

	//Test that all subtree nodes have keys with values in the correct range
	//No longer true with DET enc
	/*int low = 0;
	int high = sorted_keys[0];
	int i=0;
	if(cur_node->key_in_map((EncT) NULL)){
		if(test_vals(cur_node->right[(EncT) NULL], low, high)!=true) return false;
	}

	for(int j=1; j<N-1; j++){
		if((int) sorted_keys.size()>j){
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

	for(int x=0; x< (int) sorted_keys.size()-1; x++){
		if(sorted_keys[x]==sorted_keys[x+1]) return false;
	}*/

	return true;

}

//Input: Node, and low and high end of range to test key values at
template<class EncT>
bool
tree<EncT>::test_vals(tree_node<EncT>* cur_node, EncT low, EncT high){
	//Make sure all keys at node are in proper range
	for(int i=0; i< (int) cur_node->keys.size(); i++){
		EncT this_key = cur_node->keys[i];
		if(this_key<=low || this_key>=high){
			cout<<"Values off "<<this_key<<": "<<low<<", "<<high<<endl;
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
void handle_client(void* lp, tree<EncT>* s){
        int *csock = (int*) lp;

        cout<<"Call to function!"<<endl;
        //Buffer to handle all messages received
        char buffer[1024];
        while(true){
        	//Clear buffer
            memset(buffer, 0, 1024);

            //Receive message to process
            recv(*csock, buffer, 1024, 0);
            if(DEBUG_COMM) cout<<"Received msg "<<buffer<<endl;
            //Find protocol code:
		    /*Server protocol code:
			 * lookup(encrypted_plaintext) = 1
			 * lookup(v, nbits) = 2
			 * insert(v, nbits, index, encrypted_laintext) = 3
			*/            
            int func_d;
            istringstream iss(buffer);
            iss>>func_d;
            if(DEBUG_COMM) cout<<"See value func_d: "<<func_d<<endl;

            ostringstream o;
            string rtn_str;
            if(func_d==0){
            	//Kill server
                break;
            }else if(func_d==1) {
            	//Lookup using OPE table
                uint64_t blk_encrypt_pt;
                iss>>blk_encrypt_pt;
                if(DEBUG_COMM) cout<<"Blk_encrypt_pt: "<<blk_encrypt_pt<<endl;
                //Do tree lookup
                table_storage table_rslt = s->lookup((uint64_t) blk_encrypt_pt);
                if(DEBUG_COMM) cout<<"Rtn b/f ostringstream: "<<table_rslt.v<<" : "<<table_rslt.pathlen<<" : "<<table_rslt.index<<" : "<<table_rslt.version<<endl;
                //Construct response
                o<<table_rslt.v<<" "<<table_rslt.pathlen<<" "<<table_rslt.index<<" "<<table_rslt.version;
                rtn_str=o.str();
                if(DEBUG_COMM) cout<<"Rtn_str : "<<rtn_str<<endl;
                send(*csock, rtn_str.c_str(), rtn_str.size(),0);
            }else if(func_d==2){
            	//Lookup w/o table, using v and nbits
                uint64_t v, nbits;
                iss>>v;
                iss>>nbits;
                if(DEBUG_COMM) cout<<"Trying lookup("<<v<<", "<<nbits<<")"<<endl;
                vector<EncT> xct_vec;
                //Do tree lookup
                try{
                        xct_vec = s->lookup(v, nbits);
                }catch(ope_lookup_failure&){
                        if(DEBUG_COMM) cout<<"Lookup fail, need to insert!"<<endl;
                        const char* ope_fail = "ope_fail";
                        send(*csock, ope_fail, strlen(ope_fail),0);
                        continue;
                }
                //Construct response out of vector xct_vec
                for(int i=0; i< (int) xct_vec.size(); i++){
                	o<<xct_vec[i];
                	o<<" ";
                	if(DEBUG_BTREE) cout<<"xct_vec "<<i<<"="<<xct_vec[i]<<endl;
                }
                o<<"; ";
                for(int i=0; i<(int) xct_vec.size(); i++){
                	stringstream pss;
                	pss<<xct_vec[i];

                	Elem desired;
                	desired.m_key = pss.str();
                	Node* last;
                	Elem& result = s->tracker.get_root()->search(desired, last);
	    			assert_s(desired.m_key == result.m_key, "could not find val that should be there");
	    			MerkleProof proof = get_merkle_proof(last);
	    			o<<proof<<" ";
	    			if(DEBUG_BTREE) cout<<"2 MP "<<i<<"="<<proof<<endl;
	    			pss.str("");
	    			pss.clear();
                }

                rtn_str=o.str();
                if(rtn_str.size()>1024){
                	cout<<"Message too long!"<<endl;
                	exit(-1);
                }
                if(DEBUG_COMM || DEBUG_BTREE) cout<<"Rtn_str: "<<rtn_str<<endl;
                send(*csock, rtn_str.c_str(), rtn_str.size(),0);
            }else if(func_d==3){
            	//Insert using v and nbits
                uint64_t v, nbits, index, blk_encrypt_pt;
                iss>>v;
                iss>>nbits;
                iss>>index;
                iss>>blk_encrypt_pt;
                //Insert...need not send response
                if(DEBUG_COMM) cout<<"Trying insert("<<v<<", "<<nbits<<", "<<index<<", "<<blk_encrypt_pt<<")"<<endl;
                string proof = s->insert(v, nbits, index, blk_encrypt_pt);
                if(proof.size()>1024){
                	cout<<"Insert proof too large"<<endl;
                	exit(-1);
                }
                if(DEBUG_COMM) cout<<proof<<endl;
                send(*csock, proof.c_str(), proof.size(),0);
            }else if(func_d==4){
            	uint64_t v, nbits, index;
            	iss>>v;
            	iss>>nbits;
            	iss>>index;
            	string proof = s->delete_index(v, nbits, index);
            	if(proof.size()>1024){
            		cout<<"Delete proof too large"<<endl;
            		exit(-1);
            	}
				send(*csock, proof.c_str(), proof.size(),0);
            }else{
            	//Uh oh!
                cout<<"Something's wrong!: "<<buffer<<endl;
                break;
            }
            if(DEBUG) cout<<"Message "<<func_d<<" processed"<<endl;
            //TEST CODE (comment out in release version)
			if(s->root!=NULL){
				if(DEBUG) cout<<"Testing tree"<<endl;
				//After every message, check tree is still really a tree,
				//and that it maintains approx. alpha height balance
				if(DEBUG) s->print_tree();			
				if(s->test_tree(s->root)!=1){
					cout<<"No test_tree pass"<<endl;
					return;
				} 
				if(s->num_nodes>1){
					if(((s->root->height()-1)<= log(s->num_nodes)/log(((double)1.0)/alpha)+1)!=1) {
						cout<<func_d<<" height wrong "<<s->root->height()-1<<" : "<<log(s->num_nodes)/log(((double)1.0)/alpha)+1<<endl;
						cout<<"Max size: "<<s->max_size<<" num_nodes: "<<s->num_nodes<<endl;
						return;
					}
				}	
			}

        }
        free(csock);
}


int main(int argc, char **argv){
	//Build mask based on N
	mask = make_mask();

	cerr<<"Starting tree server \n";

	//Construct new server with new tree
	tree<uint64_t>* server = new tree<uint64_t>();

	cerr<<"Made a server \n";
	//Socket connection
	int host_port = 1111;
    int hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock ==-1){
            cout<<"Error initializing socket"<<endl;
    }
    cerr<<"Init socket \n";
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(host_port);
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    cerr<<"Sockaddr \n";
    //Bind to socket
    int bind_rtn = bind(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr));
    if(bind_rtn<0){
            cerr<<"Error binding to socket"<<endl;
    }
    cerr<<"Bind \n";
    //Start listening
    int listen_rtn = listen(hsock, 10);
    if(listen_rtn<0){
            cerr<<"Error listening to socket"<<endl;
    }
    cerr<<"Listening \n";

    socklen_t addr_size=sizeof(sockaddr_in);
    int* csock;
    struct sockaddr_in sadr;
    int i=1;
    //Handle 1 client b/f quiting (can remove later)
    while(i>0){
            cerr<<"Listening..."<<endl;
            csock = (int*) malloc(sizeof(int));
            if((*csock = accept(hsock, (struct sockaddr*) &sadr, &addr_size))!=-1){
            	//Pass connection and messages received to handle_client
                handle_client((void*)csock,server);
            }
            else{
                cout<<"Error accepting!"<<endl;
            }
            i--;
    }
    cerr<<"Done with server, closing now\n";
    close(hsock);
}
