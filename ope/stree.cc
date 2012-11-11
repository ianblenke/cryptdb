#include <ope/stree.hh>

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
using std::vector;
using std::cerr;
using std::map;
using std::max;


/* Some notes on when rebalancings happen and what is complexity for b-ary trees:
 *
 * Rivest and Galperin'93 say that the tree gets rebalanced if
 * max height > floor(log_1/alpha(num_nodes)), where height is num edges
 *
 * When this happens, relabeling triggers and tries to find the first element
 * from leaf to root that is alpha-weight unbalanced. A node is alpha-weight
 * balanced if the size of each child (size being num nodes including itself)
 * is at least alpha * size of parent. They later also propose to balance the
 * tree when at the first node that is not alpha-height balanced. A node is
 * alpha height balanced if its max height is at most floor(log_1/alpha(node size)).
 *
 * What amortized complexity does this give us for a b-ary tree? (B trees have
 * of b log_b n).
 *
 * If we pick the alpha-height-balanced scapegoat finding strategy, we get an amortized cost of
 * log_b n * (1-alpha)/(alpha - 1/2) worst case
 *
 * If we pick the alpha-weight-balanced it is at least this.
 */

		      

template<class EncT>
Stree<EncT>::Stree(OPETable<EncT> * ot, Connect * db) : ope_table(ot), dbconnect(db){
    if (DEBUG_STREE) {cerr << "creating the tree \n";}
    
    root = new Stree_node<EncT>();

    max_size=0;
    num_rebalances=0;

    cerr << "ope table " << this->ope_table << "\n";

    
#if MALICIOUS
    
    Node::m_failure.invalidate();
    Node::m_failure.m_key = "";

    Node* root_ptr = new Node(tracker);
    tracker.set_root(null_ptr, root_ptr);

#endif

}


template<class EncT>
Stree<EncT>::~Stree(){
    cerr << "destroying the tree\n";
    delete_nodes(root);
    delete root;
}

template<class EncT>
Stree_node<EncT> *
Stree<EncT>::get_root(){
  return root;

}

/********** Tree rebalancing ******************/

template<class EncT>
vector<EncT> 
Stree<EncT>::flatten(Stree_node<EncT>* node){
    vector<EncT> rtn_keys;

    vector<EncT> tmp;
    if(node->has_subtree( (EncT) NULL)){
	tmp=flatten(node->right[(EncT) NULL]);
	rtn_keys.insert(rtn_keys.end(), tmp.begin(), tmp.end());
    }
    for(int i=0; i<(int) node->keys.size(); i++){
	EncT tmp_key = node->keys[i];
	rtn_keys.push_back(tmp_key);
	if(node->has_subtree(tmp_key)) {
	    tmp = flatten(node->right[tmp_key]);
	    rtn_keys.insert(rtn_keys.end(), tmp.begin(), tmp.end() );
	}
    }

    return rtn_keys;			

}

//TODO eff: both rebuild and flatten create a whole vector and keep copying it
// around
// -- could take advantage of the fact that we
// know the num of nodes;
template<class EncT>
Stree_node<EncT>* 
Stree<EncT>::rebuild(vector<EncT> key_list){
    //Avoid building a node without any values
    if (key_list.size()==0) {return NULL;}

    Stree_node<EncT>* rtn_node = new Stree_node<EncT>();

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
	    Stree_node<EncT>* tmp_child = rebuild(subvector);
	    if (tmp_child!=NULL){
		    rtn_node->right[key_list[floor(i*base)]]=tmp_child;
	    }
	}
	//First subvector rebuilding special case
	vector<EncT> subvector;
	for (int j=0; j<floor(base); j++){
	    subvector.push_back(key_list[j]);
	}
	Stree_node<EncT>* tmp_child = rebuild(subvector);
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

// depth is the depth of the scapegoat node (num edges from root to scapegoat)
template<class EncT>
void 
Stree<EncT>::rebalance(Stree_node<EncT>* node, uint64_t v, uint64_t nbits,
		      uint64_t depth){
    if (DEBUG) cout<<"Rebalance"<<endl;

    vector<EncT> key_list = flatten(node);
/*	if(DEBUG){
	vector<EncT> tmp_list = key_list;
	sort(key_list.begin(), key_list.end());
	if(key_list!=tmp_list) cout<<"Flatten isn't sorted!"<<endl;		
	}*/

    Stree_node<EncT>* tmp_node = rebuild(key_list);
    delete_nodes(node);
    node->keys = tmp_node->keys;
    node->right = tmp_node->right;
    delete tmp_node;

    num_rebalances++;

    uint64_t base_v = (v >> (nbits - depth * num_bits));
    uint64_t base_nbits = depth * num_bits;

    //Make sure OPE table is updated and correct
    update_opetable_db(node, base_v, base_nbits);
    clear_db_version();
}



//Used to delete all subtrees of node
template<class EncT>
void 
Stree<EncT>::delete_nodes(Stree_node<EncT>* node){
    typename map<EncT, Stree_node<EncT> *>::iterator it;

    for(it=node->right.begin(); it!=node->right.end(); it++){
	delete_nodes(it->second);
	delete it->second;
    }

}


template<class EncT>
void
Stree<EncT>::clear_db_version(){
    string query="UPDATE emp SET version=0 where version=1";
    assert_s(dbconnect->execute(query), "failed to execute query " + query);
}

template<class EncT>
void
Stree<EncT>::delete_db(table_entry del_entry){
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
    assert_s(dbconnect->execute(query), "query " + query + " failed");
}

//Update now-stale values in db
template<class EncT>
void 
Stree<EncT>::update_db(uint64_t old_ope, uint64_t new_ope){

    stringstream query;
    query << "UPDATE emp SET ope_enc="
	  << new_ope
	  << ", version=1 where ope_enc="
	  << old_ope
	  << " and version=0";

    if(DEBUG_COMM) cout << "Query: " << query << endl;

    assert_s(dbconnect->execute(query.str()), string("query ") + query.str() + " failed ");

}

//Ensure table is correct
template<class EncT>
void
Stree<EncT>::update_opetable_db(Stree_node<EncT> *node, uint64_t base_v, uint64_t base_nbits){

    update_shifted_paths(0, base_v, base_nbits, node);

    uint64_t next_v, next_nbits;
	
    next_nbits = base_nbits+num_bits;

    if(node->has_subtree( (EncT) NULL)){

	next_v = base_v<<num_bits;
	update_opetable_db(node->right[(EncT) NULL],  next_v, next_nbits);
    }
    
    for(int i = 0; i < (int) node->keys.size(); i++){

	if(node->has_subtree(node->keys[i])){

	    next_v = (base_v<<num_bits) | (i+1);
	    update_opetable_db(node->right[node->keys[i]], next_v, next_nbits);	
	}	
    }

}

template<class EncT>
void 
Stree<EncT>::print_tree(){
    vector<Stree_node<EncT>* > queue;
    queue.push_back(root);
    while(queue.size()!=0){
	Stree_node<EncT>* cur_node = queue[0];
	if(cur_node==NULL){
	    queue.erase(queue.begin());
	    cout<<endl;
	    continue;
	}
	cout<<cur_node->height()-1<<": ";
	if(cur_node->has_subtree( (EncT)NULL)){
	    queue.push_back(cur_node->right[ (EncT)NULL]);
	}	
	for(int i=0; i< (int) cur_node->keys.size(); i++){
	    EncT key = cur_node->keys[i];
	    cout<<key<<", ";
	    if(cur_node->has_subtree(key)){
		queue.push_back(cur_node->right[key]);
	    }
	}	
	cout<<endl;
	queue.push_back((EncT) NULL);
	queue.erase(queue.begin());
    }

}

/*
 * Given a path to a newly inserted node (from leaf to root node), find a
 * scapegoat on this path.
 * Sets path_index to be the depth of the scapegoat (no of edges to scapegoat
 * from root)
 */
template<class EncT>
Stree_node<EncT>* 
Stree<EncT>::findScapegoat( vector<Stree_node<EncT>* > path , uint64_t & path_index){

    // Move along path from leaf to root
    // until find scapegoat
    for (int i=0; i < (int) path.size(); i++){
	Stree_node<EncT>* cur_node = path[i];

	for(auto it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
	    if (it->second->num_nodes > alpha * cur_node->num_nodes) {
		path_index = path.size() - 1 - i;
		return it->second;
	    }
	}		
    }
    path_index = 0;
    return root;
}

/*
template<class EncT>
struct successor {
    uint64_t succ_v;
    uint64_t succ_nbits;
    Stree_node<EncT>* succ_node;
};

template<class EncT>
successor<EncT>
Stree<EncT>::find_succ(Stree_node<EncT>* node, uint64_t v, uint64_t nbits){
    if(node->has_subtree((EncT) NULL)){
	return find_succ(node->right[(EncT) NULL], (v<<num_bits), nbits+num_bits);
    }else{
	successor<EncT> succ;
	succ.succ_v = v;
	succ.succ_nbits = nbits;
	succ.succ_node = node;
	return succ;
    }
}
*/
/*
template<class EncT>
struct predecessor{
    uint64_t pred_v;
    uint64_t pred_nbits;
    uint64_t pred_index;
    Stree_node<EncT>* pred_node;
};

template<class EncT>
predecessor<EncT>
Stree<EncT>::find_pred(Stree_node<EncT>* node, uint64_t v, uint64_t nbits){
    int max_index = node->keys.size()-1;

    EncT ciph = node->keys[max_index];
    
    if(node->has_subtree(ciph)){
	if(max_index!=N-2) {
	    cout<<"Error in pred, max_index not N-1"<<endl;
	    exit(1);
	}
	return find_pred(node->right[ciph], (v<<num_bits | (max_index+1)), nbits+num_bits);
    } else {
	predecessor<EncT> pred;
	pred.pred_v = v;
	pred.pred_nbits = nbits;
	pred.pred_index = max_index;
	pred.pred_node = node;
	return pred;
    }
}
*/
/*template<class EncT>
  string
  Stree<EncT>::delete_index(uint64_t v, uint64_t nbits, uint64_t index){
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
  Stree<EncT>::tree_delete(Stree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, uint64_t pathlen, bool swap){
  if (DEBUG) cout<<"Calling Stree_delete with "<<v<< " "<<nbits<<" "<<index<<endl;
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

  }else if(node->has_subtree(del_val)){
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
  if(node->has_subtree(node->keys[i])) {has_succ=true; break;}
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
  if(!node->has_subtree((EncT) NULL)){cout<<"Error in delete, child is truly leaf"<<endl; exit(1);}
  right_parent = (EncT) NULL;
  }else{
  right_parent = node->keys[index-1];
  }

  if(node->has_subtree(right_parent)){
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

  int key_index = (int) (v&(s_mask<<(nbits-num_bits)))>>(nbits-num_bits);

  EncT key;

  //Protocol set: index 0 is NULL (branch to node with lesser elements)
  //Else, index is 1+(key's index in node's key-vector)
  if(key_index==0) key= (EncT) NULL;
  else if(key_index<N) key = node->keys[key_index-1];
  else{cout<<"Delete fail, key_index not legal "<<key_index<<endl; exit(1);}

  //cout<<"Key: "<<key<<endl;

  //See if pointer to next node to be checked 
  if(!node->has_subtree(key)){
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

static bool
trigger(uint height, uint num_nodes, double alpha) {
    return height > (1.0 * log(num_nodes*1.0)/log(((double)1.0)/alpha) + 1);
}

template<class EncT>
static EncT TFromString(string s) {
    istringstream ss(s);
    EncT e;
    ss >> e;
    return e;
    
}

// v is path to node where the insertion should happen; nbits is len of v
// index is position in node where insertion should happen
template<class EncT>
void
Stree<EncT>::insert(string encval, TreeNode * tnode,
		    uint64_t v, uint64_t nbits, uint64_t index,
		    UpdateMerkleProof & p){
    bool node_inserted;

    EncT eval = TFromString<EncT>(encval);
    
    //todo: use tnode directly rather than searching for the node with v
    vector<Stree_node<EncT> * > path = tree_insert(root, v, nbits, index,
						   eval, nbits, node_inserted);

#if MALICIOUS
    
    //Merkle tree insert
    stringstream s;
    s << encval;
    string encval_str = s.str();

    Elem elem;
    elem.m_key = encval_str;
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


    double height = (double) path.size();

    if (height==0) {
	return;
    }
    
    //if(DEBUG) print_tree();

    if (trigger(height, root->num_nodes, alpha)) {
    	uint64_t path_index;

	Stree_node<EncT>* scapegoat = findScapegoat(path, path_index);
	rebalance(scapegoat, v, nbits, path_index);
	
	if (DEBUG) {cout<< " Insert rebalance "<< height <<": "<< root->num_nodes << endl;}

	if (scapegoat == root && max_size < root->num_nodes) {
	    max_size = root->num_nodes;
	}

        //if(DEBUG) print_tree();
    }
    
}

// updates ope encodings for the keys at node starting from index+1
// in the ope table and the DB
// does not proceed recursively down the tree because it assumes this is a leaf
template<class EncT>
void
Stree<EncT>::update_shifted_paths(uint index, uint64_t v, int pathlen,
				      Stree_node<EncT> * node) {

    for(int i=(int)index; i< (int) node->keys.size(); i++) {
	EncT ckey = node->keys[i];
	
	OPEType newope = compute_ope(v, pathlen, i);
	OPEType oldope = this->ope_table->get(ckey).ope;
	
	this->ope_table->update(ckey, newope, node);
	update_db(oldope, newope);
    }
    
}

/* Extracts the first index from the last nbits of v */
static int
extract_index(OPEType v, uint nbits) {
    return (int) (v&(s_mask<<(nbits-num_bits)))>>(nbits-num_bits);    
}


// v is path to node where to insert (if node is full, insertion will create a
// new node)
// the vector returned is the path from where encval is inserted to root
// pathlen is the length of v
// node_inserted is set to true if a node (tree node, not key) was inserted
template<class EncT>
vector<Stree_node<EncT>* >
Stree<EncT>::tree_insert(Stree_node<EncT>* node,
			uint64_t v, uint64_t nbits, uint64_t index,
			EncT encval, uint64_t pathlen,
			bool & node_inserted){

    vector<Stree_node<EncT>* > rtn_path;

    // End of the insert line, check if you can insert
    // Assumes v and nbits were from a lookup of an insertable node
    if (nbits==0) {
 	assert_s(node->keys.size() <= N-1," too many values at node");
	
	assert_s(index == node->keys.size() ||
		 node->keys[index] != encval,
		 "attempting to insert same value into tree");

	if (node->keys.size() == N-1) {// node is full

	    cerr << "tree_insert: node is full, create subtree \n";
	    
	    Stree_node<EncT> * subtree = node->new_subtree(index);
	    node->num_nodes++;
	    max_size = max(node->num_nodes, max_size);
	    
	    rtn_path = tree_insert(subtree, path_append(v, index),
				   0, 0, encval, pathlen+num_bits, node_inserted);

	    node_inserted = true;
	    rtn_path.push_back(node);
 
	    return rtn_path;
	}

	//INSERT HERE
	
	if(DEBUG) cout<< "tree_insert: inserting; ope path " << v << " index " << index << endl;

	typename vector<EncT>::iterator it;
	it = node->keys.begin();
	node->keys.insert(it+index, encval);

	// update ope table and db
	assert_s(this->ope_table->insert(encval, compute_ope(v, pathlen, index), node),
		 "did not insert new entry in ope_table");
	update_shifted_paths(index+1, v, pathlen, node);
	clear_db_version();

	// prepare return values
	node_inserted = false;
	rtn_path.push_back(node);
	return rtn_path;
    }

    // nbits > 0

    cerr << "tree_insert: one step down the tree\n";
    
    Stree_node<EncT> * subtree = node->get_subtree(extract_index(v, nbits));
    assert_s(subtree, "subtree must exist, but it doesn't");
    
    rtn_path = tree_insert(subtree, v, nbits-num_bits, index, encval, pathlen, node_inserted);

    //prepare results
    if (node_inserted) {
	node->num_nodes++;
    }
    rtn_path.push_back(node);
    return rtn_path;
}

template<class EncT>
bool
Stree<EncT>::test_tree(Stree_node<EncT>* cur_node){
    if(test_node(cur_node)!=true) return false;

    typename map<EncT, Stree_node<EncT> *>::iterator it;

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
Stree<EncT>::test_node(Stree_node<EncT>* cur_node){
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
	if(cur_node->has_subtree( (EncT) NULL)) return false;
	for(int i=0; i< (int) sorted_keys.size(); i++){
	    if(cur_node->has_subtree(sorted_keys[i])) return false;
	}
    }

    return true;

}

//Input: Node, and low and high end of range to test key values at
template<class EncT>
bool
Stree<EncT>::test_vals(Stree_node<EncT>* cur_node, EncT low, EncT high){
    //Make sure all keys at node are in proper range
    for(int i=0; i< (int) cur_node->keys.size(); i++){
	EncT this_key = cur_node->keys[i];
	if ( this_key <= low || this_key >= high) {
	    cout << "Values off "<< this_key << ": " << low << ", " << high << endl;
	    return false;
	}
    }

    typename map<EncT, Stree_node<EncT> *>::iterator it;

    for(it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
	if(test_vals(it->second, low, high)!=true) return false;
    }
    return true;
}

/*
 * Explicitly instantiate the tree template for various ciphertext types.
 */
template class Stree<uint64_t>;
template class Stree<uint32_t>;
template class Stree<uint16_t>;

template <class EncT>
int
Stree_node<EncT>::height() {
    typename std::map<EncT, Stree_node *>::iterator it;
    int max_child_height=0;
    int tmp_height=0;
    
    for(it=right.begin(); it!=right.end(); it++){
	tmp_height=it->second->height();
	if(tmp_height>max_child_height) {
	    max_child_height=tmp_height;
	}
    }		
    
    return max_child_height+1;
    
}

template<class EncT>
static string
TToString(EncT et) {
    ostringstream ss;
    ss << et;

    return ss.str();
}


template<class EncT>
vector<string>
Stree_node<EncT>::get_keys() {
    vector<string> res;
    for (auto k : keys) {
	res.push_back(TToString<EncT>(k));
    }
    return res;
}

template<class EncT>
int
Stree_node<EncT>::size(){
    int totalsize = keys.size();
    typename std::map<EncT, Stree_node *>::iterator it;
    
    for( it = right.begin(); it!=right.end(); it++){
	totalsize +=it->second->size();
    }		
    
    return totalsize;		    
}

//Returns true if node's right map contains key (only at non-leaf nodes)
template<class EncT>
Stree_node<EncT> *
Stree_node<EncT>::has_subtree(EncT key){ 
    auto it = right.find(key);

    // either key is not in right or it is in which case the subtree should we non-NULL
    assert_s((it == right.end()) || (it->second != NULL), "inconsistency in right");

    if (it == right.end()) {
	return NULL;
    } else {	
	return it->second;
    }
}

template<class EncT>
static EncT
get_key_for_index(uint index, Stree_node<EncT> * tnode) {
    assert_s(index <= tnode->keys.size(), "invalid index, larger than size of keys");

    if (index == 0) {
	return (EncT) NULL;
    } else {
	return tnode->keys[index - 1];
    }
}

template<class EncT>
Stree_node<EncT> *
Stree_node<EncT>::get_subtree(uint index) {

    EncT key = get_key_for_index(index, this);

    return has_subtree(key);
}

template<class EncT>
Stree_node<EncT> *
Stree_node<EncT>::new_subtree(uint index) {
    EncT key = get_key_for_index(index, this);

    assert_s(!has_subtree(key), "subtree should not have existed");
    Stree_node * subtree = new Stree_node();
    right[key] = subtree;
    return subtree;
}

template<class EncT>
MerkleProof
Stree<EncT>::merkle_proof(TreeNode * n) {
    assert_s(false, "Stree node_merkle_proof UNIMPLEMENTED");
    return MerkleProof();
}

template class Stree_node<uint64_t>;

