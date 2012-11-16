#pragma once

/*****************
 * B Tree
 ****************/

#include <iostream>
#include <string>
#include <vector>
#include <ope/merkle.hh>
#include <ope/tree.hh>
#include <util/ope-util.hh>
#include <ope/opetable.hh>

// uint Merklecost = 0;

/** Forwarding Data Structures **/

class Elem;

class RootTracker;

class Node;

Node* build_tree(std::vector< std::string> & key_list, RootTracker * root_tracker, int start, int end);
Node* build_tree_wrapper(std::vector<std::string> & key_list, RootTracker * root_tracker, int start, int end);


#define invalid_ptr  reinterpret_cast<Node*>(-1)
#define null_ptr  reinterpret_cast<Node*>(0)


/**  Merkle Proof and verification **/

// records state of tree starting from node up to root
void
record_state(Node * node, State & state);


// returns the information needed to check the validity of node 
MerkleProof
node_merkle_proof(Node * n);


// verifies that the merkle information corresponding to a node matches the
// overall root merkle hash
bool
verify_merkle_proof(const MerkleProof & proof, const std::string & merkle_root);

//verifies the merkle proof for deletion and returns true if it holds, and it
//also sets new_merkle_root accordingly
bool
verify_del_merkle_proof(const UpdateMerkleProof & p,
            std::string del_target,
            const std::string & merkle_root,
            std::string & new_merkle_root);

// Verifies insertion Merkle proof
// bc is the block cipher to use for decryption 
template<class V, class BC>
bool
verify_ins_merkle_proof(const UpdateMerkleProof & p,
			std::string ins_target,
			const std::string & merkle_root,
			std::string & new_merkle_root,
			BC * bc = NULL);

/////////////////////////////////////////////////////////////////////////////
///// Helper data structures //////

/* Contains information about
 * a split at a level in a tree.
 * The node was split at split_point into itself and right.
 * split_point is negative if there was no split
 * index is index where element was inserted
 */

struct LevelChangeInfo {
    Node * node;
    Node * right;
    int index;
    int split_point;
    bool is_new_root;
    OPEType opepath;
    uint nbits;
      
    LevelChangeInfo() {
	node = right = NULL;
	index = -1;
	is_new_root = false;
    }

    LevelChangeInfo(Node * _node, Node * _right,
		    int _index, int _split_point, bool new_root = false):
	node(_node), right(_right),
	index(_index), split_point(_split_point),
	is_new_root(new_root){}
};


// list of LevelChangeInfo where first is a leaf and last is the last
// node affected by an insert
typedef std::list<LevelChangeInfo > ChangeInfo;


//////////////////////////////////////

class BTree : public Tree {
public:

    BTree(OPETable<std::string> * ot, Connect * db, bool malicious,
	  string table_name = "", string field_name = "", bool db_updates = true);
    ~BTree();
    
    TreeNode * get_root();

//    Node* build_tree(std::vector< std::string> & key_list, RootTracker & root_tracker, int start, int end);
//    Node* build_tree_wrapper(std::vector<std::string> & key_list, RootTracker & root_tracker, int start, int end);

    void insert(std::string ciph, TreeNode * tnode,
		OPEType ope_path, uint64_t nbits, uint64_t index,
		UpdateMerkleProof & p);

     MerkleProof merkle_proof(TreeNode * n);
    
    void update_db(OPEType ciph, ChangeInfo  c);

    uint num_rewrites() {return nrewrites;}
    
    //returns the size of the subtree root at this Node
    // in terms of keys (does not count the empty key)
    static uint size(Node * n);

    uint nrewrites;
    RootTracker * tracker;
    OPETable<std::string> * opetable;
    Connect * db;
    string table_name, field_name;
    bool db_updates;

    uint glb_counter;
};


class Node : public TreeNode {
  
public:

    Node (RootTracker * root_track);

    // to return a reference when a search fails.
    static Elem m_failure;

    // the merkle hash of this node
    std::string merkle_hash;
 
    // the root of the tree may change.  this attribute keeps it accessible.
    RootTracker * m_root;
    
    Node* get_root();
    Elem& search (Elem& desired, Node*& last_visited);
 
    Elem& operator[] (int i) { return m_vector[i]; }
    // node cannot be instantiated without a root tracker

    // return all keys at the node -- do not return the "" key
    std::vector<std::string> get_keys();
    TreeNode * get_subtree(uint index);

  
    std::string pretty() const;
    
    std::vector<Elem > m_vector;
    // number of elements currently in m_vector, including the zeroth element
    // which has only a subtree, no key value or payload.
    unsigned int m_count;
    Node* mp_parent;

    /***** HELPER FUNCTIONS ***************/

    //Dump to file so we can measure size

    void dump_to_file();

    // locality of reference, beneficial to effective cache utilization,
    // is provided by a "vector" container rather than a "list"
    // upon construction, m_vector is sized to max amount of elements
    // the first element is the "zero" element, empty key and points to zeroth subtree
    //std::vector<Elem> m_vector;

    
    bool tree_insert (Elem element, UpdateMerkleProof &p);
    bool tree_delete(Elem & target, UpdateMerkleProof & m);

    void dump(bool recursive = true);
    void in_order_traverse(std::list<std::string> & res);

    
    bool do_insert(Elem element, UpdateMerkleProof & p, ChangeInfo & c, int index = -1);

    //cleaning up
    int delete_all_subtrees();


    bool is_leaf();
    
    bool vector_insert (Elem & element, int index = -1);
    bool vector_insert_for_split (Elem& element, int index = -1);
    bool split_insert (Elem& element, ChangeInfo & ci, int index = -1);

    bool vector_delete (Elem& target);
    bool vector_delete (int target_pos);

    void insert_zeroth_subtree (Node* subtree);

    void set_debug();
    int key_count () const { return m_count-1; }

    Elem& largest_key () { return m_vector[m_count-1]; }
    Elem& smallest_key ();
    Elem& smallest_key_in_subtree();
    int index_has_subtree();

    Node* right_sibling (int& parent_index_this);
    Node* left_sibling (int& parent_index_this);

    void rotate_from_left(int parent_index_this);
			  
    void rotate_from_right(int parent_index_this);

    Node* merge_right (int parent_index_this);
    Node* merge_left (int parent_index_this);

    bool merge_into_root ();

    bool
    tree_delete_help (Elem& target, UpdateMerkleProof & proof, Node* & start_node, Node * node);

   

    // outputs the index that this node has in his parent element list
    int index_in_parent();
    
    // outputs the index that child has in this parent list
    // or a negative value if it does not exist  
    int index_of_child(Node * child);


    /***************************
     * Merkle related functions */

    // computes the hash of the current node
    std::string hash_node();
    
    //recomputes Merkle hash of the current node
    void update_merkle();

    //recomputes Merkle hash of all the nodes from this up to the root
    void update_merkle_upward();

    NodeInfo extract_NodeInfo(int notextract);

    void finish_del(UpdateMerkleProof & proof);

    // returns the number of nodes to be included in a Merkle proof of insertion
    uint Merkle_cost();

    // Functions for testing
    void check_merkle_tree(); //checks merkle tree was computed correctly
    void recompute_merkle_subtree();
    void compute_merkle_subtree();
    uint max_height();   //height in no of edges 

#ifdef _DEBUG

    Elem debug[8];

#endif
    
};



std::ostream &
operator<<(std::ostream & out, const Node & n);

	
/*
 * contains a key value, a payload, and a pointer toward the subtree
 * containing key values greater than this->m_key but lower than the
 * key value of the next element to the right
 */
class Elem {

public:

    bool operator>   (Elem& other) const { return m_key.compare(other.m_key) > 0; }
    bool operator<   (Elem& other) const { return m_key.compare(other.m_key) < 0; }
    bool operator>=  (Elem& other) const { return m_key.compare(other.m_key) >= 0; }
    bool operator<=  (Elem& other) const { return m_key.compare(other.m_key) <= 0; }
    bool operator==  (Elem& other) const { return m_key.compare(other.m_key) == 0; }

    bool valid () const { return mp_subtree != invalid_ptr; }

 
    void invalidate () { mp_subtree = invalid_ptr; }

    Elem& operator= (const Elem& other) {

        m_key = other.m_key;
        mp_subtree = other.mp_subtree;

        return *this;
    }

    Elem () { mp_subtree = null_ptr; }
    Elem (std::string key);
    void dump();
    std::string pretty() const;

    std::string m_key;
    Node* mp_subtree;
    bool has_subtree() const {return valid() && (mp_subtree != null_ptr) && mp_subtree; }

    void dump_to_file();
private:
    friend class Node;
    friend class NodeMerkleInfo;
    friend class NodeInfo;
    friend class Test;

    //std::string m_key;
    
    /***********************
     **** Merkle-related ***/

    // returns a hash of the subtree of this element,
    // even if the subtree is empty
    std::string get_subtree_merkle();
    
    std::string repr(); // converts to string the relevant information for Merkle
		      // hash of this node

    /*********************/

}; 

std::ostream&
operator<<(std::ostream & out, const Elem & e);


class RootTracker {

// all the node instances that belong to a given tree have a reference to one
// instance of RootTracker.  As the Node instance that is the root may change
// or the original root may be deleted, Node instances must access the root
// of the tree through this tracker, and update the root pointer when they
// perform insertions or deletions. Using a static attribute in the Node
// class to hold the root pointer would limit a program to just one B-tree.

protected:

    Node* mp_root;
    
public:

    RootTracker(bool malicious) { mp_root = null_ptr; MALICIOUS = malicious; }

    void set_root (Node* old_root, Node* new_root) {

        // ensure that the root is only being changed by a node belonging to the
        // same tree as the current root

        if (old_root != mp_root)
            throw "wrong old_root in RootTracker::set_root";
        else {
            mp_root = new_root;
	}
    }

    Node* get_root () { return mp_root; }

    ~RootTracker () {

        // safety measure
       if (mp_root) {

            mp_root->delete_all_subtrees();

            delete mp_root;

        }

    }

    bool MALICIOUS;

};

////////////////////////////////////////////////////////////////////////////////////
/* Function definitions for templated functions */

template<class V, class BC>
bool verify_ins_merkle_proof(const UpdateMerkleProof & proof,
			     std::string ins_target,
			     const std::string & old_merkle_root,
			     std::string & new_merkle_root,
			     BC * bc = NULL) {

    if (DEBUG_PROOF) { std::cerr << "\n\n start verify\n";}

    if (old_merkle_root != "") {//TODO: remove this check; means client needs
	// to be initialized with merkle root for empty tree
	if (old_merkle_root != proof.old_hash()) {
	    std::cerr << "merkle hash of old state does not verify \n";
	    std::cerr << "hash of old state is " << proof.old_hash() << "\n";
	    return false;
	}
    }

    bool r = proof.check_ins_change<V, BC>(ins_target, bc);
    
    if (!r) {
	std::cerr << "incorrect update information";
	return false;
    }

    new_merkle_root = proof.new_hash();
    
    return true;
}


template<class V, class BC>
bool
verify_del_merkle_proof(const UpdateMerkleProof & proof,
			std::string del_target,
			const std::string & old_merkle_root,
			std::string & new_merkle_root,
			BC * bc = NULL) {

    if (DEBUG_PROOF) { std::cerr << "\n\n start verify\n";}

    if (old_merkle_root != proof.old_hash()) {
	std::cerr << "merkle hash of old state does not verify \n";
	std::cerr << "hash of old state is " << proof.old_hash() << "\n";
	return false;
    }

    bool r = proof.check_del_change<V, BC>(del_target, bc);
    
    if (!r) {
	std::cerr << "incorrect update information";
	return false;
    }

    new_merkle_root = proof.new_hash();
    
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////



// TODO: clean these myprint vs << functions for gdb
// functions for my gdb macro,
// could not figure out how to call template funcs from gdb
const char *
myprint(const MerkleProof & v);
const char *
myprint(const UpdateMerkleProof & v);
const char *
myprint(const NodeInfo & v);
const char *
myprint(const ElInfo & v);
const char *
myprint(const Node * v);
const char *
myprint(const Elem v);
const char *
myprint(const UpInfo & v);
const char *
myprint(const Elem & v);
const char *
myprint(const Node & v);
const char *
myprint(const State & st);


