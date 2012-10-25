#pragma once

/*****************
 * B Tree
 ****************/

#include <iostream>
#include <string>
#include <vector>
#include <ope/merkle.hh>
#include <ope/tree.hh>

// benchmarking
uint Merklecost = 0;

/** Forwarding Data Structures **/

template<class payload> class Element;
typedef Element<std::string> Elem;

class RootTracker;
class Node;
struct ChangeInfo;

template<class EncT>
class BTree : public Tree<EncT> {
public:


    BTree(OPETable<EncT> * ot, Connect * db);
    
    TreeNode<EncT> * get_root();

Node* build_tree(std::vector< std::string> & key_list, RootTracker & root_tracker, int start, int end);
Node* build_tree_wrapper(std::vector<std::string> & key_list, RootTracker & root_tracker, int start, int end);


    void insert(EncT ciph, OPEType ope_path, uint64_t nbits, uint64_t index);

private:
    RootTracker * tracker;
    OPETable<EncT> * opetable;
    Connect * db;
};

template<class EncT>
class Node : TreeNode<EncT> {
  
public:

    Node (RootTracker& root_track);

    // to return a reference when a search fails.
    static Elem m_failure;

    // the merkle hash of this node
    std::string merkle_hash;
 
    // the root of the tree may change.  this attribute keeps it accessible.
    RootTracker& m_root;

    
    Node* get_root();
    Elem& search (Elem& desired, Node*& last_visited);
    bool tree_insert (Elem& element, UpdateMerkleProof &p);
    bool tree_delete(Elem & target, UpdateMerkleProof & m);

    //cleaning up
    int delete_all_subtrees();

    Elem& operator[] (int i) { return m_vector[i]; }
    // node cannot be instantiated without a root tracker

    void dump(bool recursive = true);
    void in_order_traverse(std::list<std::string> & res);

    std::string pretty() const;
    
    std::vector<Elem> m_vector;

    unsigned int m_count;
    
protected:

    // locality of reference, beneficial to effective cache utilization,
    // is provided by a "vector" container rather than a "list"
    // upon construction, m_vector is sized to max amount of elements
    // the first element is the "zero" element, empty key and points to zeroth subtree
    //std::vector<Elem> m_vector;

    // number of elements currently in m_vector, including the zeroth element
    // which has only a subtree, no key value or payload.
    //unsigned int m_count;
    Node* mp_parent;


    bool is_leaf();
    
    bool vector_insert (Elem& element);
    bool vector_insert_for_split (Elem& element);
    bool split_insert (Elem& element, ChangeInfo & ci);

    bool vector_delete (Elem& target);
    bool vector_delete (int target_pos);

    void insert_zeroth_subtree (Node* subtree);

    void set_debug();
    int key_count () { return m_count-1; }

    Elem& largest_key () { return m_vector[m_count-1]; }
    Elem& smallest_key ();
    Elem& smallest_key_in_subtree();
    int index_has_subtree ();

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
    uint max_height();   //height in no of edges 

    // Friends
    friend class Test;
    friend MerkleProof node_merkle_proof(Node * n);
    friend std::ostream &
    operator<<(std::ostream & out, const Node & n);
    friend void record_state(Node * node, State & state);
    friend Node* build_tree(std::vector<std::string> & key_list, RootTracker & root_tracker, int start, int end);
    friend Node* build_tree_wrapper(std::vector<std::string> & key_list, RootTracker & root_tracker, int start, int end); 
#ifdef _DEBUG

    Elem debug[8];

#endif
    
};



Node* invalid_ptr = reinterpret_cast<Node*> (-1);

Node* null_ptr = reinterpret_cast<Node*> (0);


std::ostream &
operator<<(std::ostream & out, const Node & n);

	
/*
 * contains a key value, a payload, and a pointer toward the subtree
 * containing key values greater than this->m_key but lower than the
 * key value of the next element to the right
 */
template<class payload> class Element {

public:

    bool operator>   (Element& other) const { return m_key.compare(other.m_key) > 0; }
    bool operator<   (Element& other) const { return m_key.compare(other.m_key) < 0; }
    bool operator>=  (Element& other) const { return m_key.compare(other.m_key) >= 0; }
    bool operator<=  (Element& other) const { return m_key.compare(other.m_key) <= 0; }
    bool operator==  (Element& other) const { return m_key.compare(other.m_key) == 0; }

    bool valid () const { return mp_subtree != invalid_ptr; }

 
    void invalidate () { mp_subtree = invalid_ptr; }

    Element& operator= (const Element& other) {

        m_key = other.m_key;
        m_payload = other.m_payload;
        mp_subtree = other.mp_subtree;

        return *this;
    }

    Element () { mp_subtree = null_ptr; }
    Element (std::string key);
    void dump();
    std::string pretty() const;
    
    std::string m_key;
    Node* mp_subtree;
    bool has_subtree() const {return valid() && (mp_subtree != null_ptr) && mp_subtree; }
private:
    friend class Node;
    friend class NodeMerkleInfo;
    friend class NodeInfo;
    friend class Test;
    template<class EncT> friend class tree;
    
    //std::string m_key;
  
    payload m_payload;
   
    
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

    RootTracker() { mp_root = null_ptr; }

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

};




/**  Merkle Proof and verification **/


void
record_state(Node * node, State & state);


// returns the information needed to check the validity of node n
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


bool
verify_ins_merkle_proof(const UpdateMerkleProof & p,
			std::string ins_target,
			const std::string & merkle_root,
			std::string & new_merkle_root);


/////////////////////////////////////////////////////////////////////////////////////


/* Contains information about
 * a split at a level in a tree.
 * The information is before the split.
 * The node was split at index into itself and right.
 * For a new root, only right is NULL and index negative.
 */
struct LevelChangeInfo {
    Node * node;
    Node * right;
    int index;
};


typedef list<LevelChangeInfo> ChangeInfo;

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
myprint(const DelInfo & v);
const char *
myprint(const Elem & v);
const char *
myprint(const Node & v);

