#include <iostream>
#include <string>
#include <vector>
#include <util/util.hh>
#include <crypto/sha.hh>

template<class payload> class Element;
typedef Element<std::string> Elem;

class RootTracker;

class Node {

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

    Node* rotate_from_left(int parent_index_this);
    Node* rotate_from_right(int parent_index_this);

    Node* merge_right (int parent_index_this);
    Node* merge_left (int parent_index_this);

    bool merge_into_root ();

    int minimum_keys ();

    std::string Merkle_hash();
    
    void max_height_help(uint height, uint & max_height);
      
#ifdef _DEBUG

    Elem debug[8];

#endif

public:

    Elem& search (Elem& desired, Node*& last_visited);
    
    bool tree_insert (Elem& element);

    bool delete_element (Elem& target);

    //cleaning up
    int delete_all_subtrees();

    Node* find_root();

    // to return a reference when a search fails.
    static Elem m_failure;

    std::string merkle_hash;

    //recomputes Merkle hash of the current node
    void update_Merkle();

    //recomputes Merkle hash of all the nodes from this up to the root
    void update_Merkle_upward();
  
    // for testing
    void check_Merkle_tree();
    void recompute_Merkle_subtree();
    uint max_height();
    
    // the root of the tree may change.  this attribute keeps it accessible.
    RootTracker& m_root;

    Elem& operator[] (int i) { return m_vector[i]; }
    // node cannot be instantiated without a root tracker

    Node (RootTracker& root_track);

    void dump(bool recursive = true);
    void in_order_traverse(std::list<std::string> & res);

}; 

Node* invalid_ptr = reinterpret_cast<Node*> (-1);

Node* null_ptr = reinterpret_cast<Node*> (0);

const int invalid_index = -1;

const unsigned int max_elements = 200;  // max elements in a node

// size limit for the array in a vector object.  best performance was
// at 800 bytes.
const unsigned int max_array_bytes = 800; 
static const int nodekeysize = 16;
	
/*
 * contains a key value, a payload, and a pointer toward the subtree
 * containing key values greater than this->m_key but lower than the
 * key value of the next element to the right
 */
template<class payload> class Element {

public:

    std::string m_key;
    payload m_payload;
    Node* mp_subtree;

public:
    bool operator>   (Element& other) const { return m_key.compare(other.m_key) > 0; }
    bool operator<   (Element& other) const { return m_key.compare(other.m_key) < 0; }
    bool operator>=  (Element& other) const { return m_key.compare(other.m_key) >= 0; }
    bool operator<=  (Element& other) const { return m_key.compare(other.m_key) <= 0; }
    bool operator==  (Element& other) const { return m_key.compare(other.m_key) == 0; }

    bool valid () const { return mp_subtree != invalid_ptr; }

    bool has_subtree() const {return valid() && (mp_subtree != null_ptr) && mp_subtree; }

    void invalidate () { mp_subtree = invalid_ptr; }

    Element& operator= (const Element& other) {

        m_key = other.m_key;
        m_payload = other.m_payload;
        mp_subtree = other.mp_subtree;

        return *this;
    }

    Element () { mp_subtree = null_ptr; }

    void dump(); 

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

