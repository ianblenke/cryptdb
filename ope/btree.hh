#include <iostream>
#include <string>
#include <vector>
#include <util/util.hh>
#include <crypto/sha.hh>

template<class payload> class Element;
typedef Element<std::string> Elem;

class RootTracker;


// The Merkle information at a node N
// needed for Merkle checks
typedef struct NodeMerkleInfo {

    // The vector of all children of node N
    // for each child, key and merkle_hash
    std::vector<std::pair<std::string, std::string>> childr;

    // The position in the vector of children of the node
    // to check by recomputing the hash;
    // the node from which the Merkle computation starts
    // upwards will have a pos_of_child_to_check of -1
    int pos_of_child_to_check;

    std::string hash() const;


} NodeMerkleInfo;

std::ostream& operator<<(std::ostream &out, const NodeMerkleInfo & mi);
std::istream& operator>>(std::istream &is, NodeMerkleInfo & mi);



// Contains Merkle information for each node on the path from a specific node
// to the root 
// Each element of the list is the NodeMerkleInfo for one level
// starting with the originating node and ending with the root
typedef std::list<NodeMerkleInfo>  MerklePath;

typedef struct MerkleProof {
    MerklePath path;
} MerkleProof;

std::ostream& operator<<(std::ostream &out, const MerkleProof & mp);
std::istream& operator>>(std::istream &is, MerkleProof & mp);

/*
 *  Deletion Merkle Proof
 *
 * -  Delete changes at most the nodes on a path from a node (``deletion path'')
 *    up to the root and one additional node, sibling of one node on this path
 *
 * - There are a few transformations deletion may perform on the tree and we
 * - need to convey these in the proofs
 */
      

// Information needed about a delete on a level
// old: state before deletion of deleted, new: state after deletion of deleted
typedef struct DelInfo {
    std::string deleted;
    NodeMerkleInfo old_node, new_node;
    
    bool is_sib;
    int pos_of_sib_in_parent;
    NodeMerkleInfo old_sib, new_sib;
    bool sib_was_deleted;
    bool this_was_deleted;
    
    //checks that the new state is a correct state
    // after deleting ``deleted'' from old state
    bool local_check();

    DelInfo(std::string deleted_, NodeMerkleInfo nodeinfo): deleted(deleted_), old_node(nodeinfo) {	
	DelInfo();
    }

    DelInfo() {
	is_sib = sib_was_deleted = this_was_deleted = false;
    }
    
} DelLevelInfo;

// Merkle proof that a certain item was deleted
typedef struct DelMerkleProof {
    std::list<DelInfo> levels;

    bool has_non_leaf;
    DelInfo non_leaf;
    
    std::string old_hash() const;
    bool check_change() const;
    std::string new_hash() const;

    DelMerkleProof() {
	has_non_leaf = false;
    }
    
} DelMerkleProof;

std::ostream& operator<<(std::ostream &out, const DelMerkleProof & dmp);
std::istream& operator>>(std::istream &is, DelMerkleProof & dmp);

class Node;

// Metadata helping to prepare deletion proof: DelMerkleProof
typedef struct DelProofMeta {
    
    Node * start_node; // lowest node from which deletion starts
    MerklePath oldpath;

    // Indicates if a non_leaf node was the first to delete from
    // Null if deletion started from leaf node
    Node * non_leaf;
    NodeMerkleInfo non_leaf_old;
    
    bool sib_was_deleted;
    // Points to a node sibling of a node on the deletion path
    // or is null if there is no such node
    Node * sibl;
    NodeMerkleInfo old_sib;
    // Node on deletion path whose sibling is sibl
    // or null if N/A
    Node * sibl_of;
    uint pos_of_sib_in_parent;

    bool new_tree_empty;

    DelProofMeta() {
	sib_was_deleted = false;
	start_node = non_leaf = sibl = sibl_of = 0;
    }
    
} DelProofMeta;

typedef struct InsMerkleProof {
    MerklePath oldproof, newproof;
} InsMerkleProof;

std::ostream& operator<<(std::ostream &out, const InsMerkleProof & imp);
std::istream& operator>>(std::istream &is, InsMerkleProof & imp);

class Node {

    
public:

    Elem& search (Elem& desired, Node*& last_visited);
    
    bool tree_insert (Elem& element, InsMerkleProof & p);

    bool tree_delete(Elem & target, DelMerkleProof & m);

    //cleaning up
    int delete_all_subtrees();

    Node* find_root();

    // to return a reference when a search fails.
    static Elem m_failure;

    // the merkle hash of this node
    std::string merkle_hash;
 
    /********************/
    
    // the root of the tree may change.  this attribute keeps it accessible.
    RootTracker& m_root;

    Elem& operator[] (int i) { return m_vector[i]; }
    // node cannot be instantiated without a root tracker

    Node (RootTracker& root_track);

    void dump(bool recursive = true);
    void in_order_traverse(std::list<std::string> & res);


protected:

    // locality of reference, beneficial to effective cache utilization,
    // is provided by a "vector" container rather than a "list"
    // upon construction, m_vector is sized to max amount of elements
    // the first element is the "zero" element, empty key and points to zeroth subtree
    std::vector<Elem> m_vector;

    // number of elements currently in m_vector, including the zeroth element
    // which has only a subtree, no key value or payload.
    unsigned int m_count;
    Node* mp_parent;


    bool is_leaf();
    
    bool vector_insert (Elem& element);
    bool vector_insert_for_split (Elem& element);
    bool split_insert (Elem& element);

    bool vector_delete (Elem& target);
    bool vector_delete (int target_pos);

    void insert_zeroth_subtree (Node* subtree);

    void set_debug();
    int key_count () { return m_count-1; }

    Elem& largest_key () { return m_vector[m_count-1]; }
    Elem& smallest_key () { return m_vector[1]; }
    Elem& smallest_key_in_subtree();
    int index_has_subtree ();

    Node* right_sibling (int& parent_index_this);
    Node* left_sibling (int& parent_index_this);

    Node* rotate_from_left(int parent_index_this, DelInfo & di, DelInfo & di_parent);
    Node* rotate_from_right(int parent_index_this, DelInfo & di, DelInfo & di_parent);

    Node* merge_right (int parent_index_this, DelInfo & di, DelInfo & di_parent);
    Node* merge_left (int parent_index_this, DelInfo & di, DelInfo & di_parent);

    bool merge_into_root ();

    int minimum_keys ();

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

    NodeMerkleInfo extract_NodeMerkleInfo(int pos);

    // Functions for testing
    void check_merkle_tree(); //checks merkle tree was computed correctly
    void recompute_merkle_subtree();
    uint max_height();    
    void max_height_help(uint height, uint & max_height);

	
    

    /****************************/

    // Friends
    friend class Test;
    friend MerklePath get_merkle_path(Node * n);
    friend MerkleProof get_search_merkle_proof(Node * n);
 
#ifdef _DEBUG

    Elem debug[8];

#endif
    
};

// returns the information needed to check the validity of node n
MerkleProof
get_search_merkle_proof(Node * n);

//returns the merkle information for a path from n to root
MerklePath
get_merkle_path(Node * n);

// verifies that the merkle information corresponding to a node matches the
// overall root merkle hash
bool
verify_merkle_proof(const MerkleProof & proof, const std::string & merkle_root);

//verifies the merkle proof for deletion and returns true if it holds, and it
//also sets new_merkle_root accordingly
bool
verify_del_merkle_proof(const DelMerkleProof & p,
			const std::string & merkle_root,
			std::string & new_merkle_root);


bool
verify_ins_merkle_proof(const InsMerkleProof & p,
			const std::string & merkle_root,
			std::string & new_merkle_root);

Node* invalid_ptr = reinterpret_cast<Node*> (-1);

Node* null_ptr = reinterpret_cast<Node*> (0);

const int invalid_index = -1;

const unsigned int max_elements = 200;  // max elements in a node

// size limit for the array in a vector object.  best performance was
// at 800 bytes.
const unsigned int max_array_bytes = 800; 

	
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

    void dump(); 

    std::string m_key;

private:
    friend class Node;
    friend class NodeMerkleInfo;
    friend class Test;
    template<class EncT> friend class tree;
    
    //std::string m_key;
    static const uint key_size = 20; //bytes of max size of key_size
    payload m_payload;
    Node* mp_subtree;
    bool has_subtree() const {return valid() && (mp_subtree != null_ptr) && mp_subtree; }

    /***********************
     **** Merkle-related ***/

    // returns a hash of the subtree of this element,
    // even if the subtree is empty
    std::string get_subtree_merkle();
    
    std::string repr(); // converts to string the relevant information for Merkle
		      // hash of this node
    static const uint repr_size = sha256::hashsize + key_size; //bytes

    /*********************/

}; 


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

