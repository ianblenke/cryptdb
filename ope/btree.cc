/*

  B-tree + Merkle tree implementation,
  The initial B-tree was adapted from toucan@textelectric.net, 2003

*/

#include "btree.hh"


using namespace std;

template<class payload>
void Element<payload>::dump () {
    std::cout << " (key = " <<  m_key << "\n" << " mhash = ";
    if (has_subtree()) {
	cout << read_short(short_string(mp_subtree->merkle_hash)) << ") "; 
    } else {
	cout << "NO SUBTREE) ";
    }

}

uint
Node::Merkle_cost() {

    uint mycost = m_count; // no of keys

    //add no of hashes
    for (uint i = 0 ; i < m_count; i++) {
	if (m_vector[i].mp_subtree != NULL) {
	    mycost++;
	}
    }

    if (!mp_parent) {
	return mycost;
    } else {
	return mycost + mp_parent->Merkle_cost();
    }
}

 

inline void Node::set_debug() {

#ifdef _DEBUG

// the contents of an STL vector are not visible in the visual C++ debugger,
// so this function copies up to eight elements from the STL vector into
// a simple C++ array.

    for (int i=0; i<m_count && i<8; i++) {

        debug[i] = m_vector[i];

        if (m_vector[i].mp_subtree)

            m_vector[i].mp_subtree->set_debug();

    }

    for ( ; i<8; i++)

        debug[i] = m_failure;

#endif

} 


void Node::insert_zeroth_subtree (Node* subtree) {

    m_vector[0].mp_subtree = subtree;

    m_vector[0].m_key = "";

    m_count = 1;

    if (subtree)

        subtree->mp_parent = this;

}
 

void Node::dump(bool recursive){

// write out the keys in this node and all its subtrees, along with
// node adresses, for debugging purposes

    cout << "[";
    if (this == m_root.get_root()) {
            cout << "ROOT ";
    }

    //cout << "\nthis=" << this << endl;

    //cout << "parent=" << mp_parent << " count=" << m_count << endl;

    cout << "m_count " << m_count << " mhash = " << merkle_hash << " ";
    cout << "keys at node: \n";
    for (unsigned int i=0; i<m_count; i++) {
	m_vector[i].dump();
    }

    cout << "] \n";

    if (!recursive) {
	return;
    }
    
    for (unsigned int i=0; i<m_count; i++) {
	if (m_vector[i].has_subtree()) {	    
	    m_vector[i].mp_subtree->dump();
	}
    }
    cout << endl;

} 

Node::Node(RootTracker& root_track)  : m_root(root_track) {

    m_vector.resize(b_max_keys);
    
    m_count = 0;
    
    mp_parent = 0;

    merkle_hash = hash_empty_node; 
    
    insert_zeroth_subtree (0);
}

 

Node* Node::get_root () {

    return m_root.get_root();

} 

 

bool Node::is_leaf () {

    return m_vector[0].mp_subtree==0;

} 
 

int Node::delete_all_subtrees () {

// return the number of nodes deleted

    int count_deleted = 0;

    for (unsigned int i=0; i< m_count; i++) {

        if (!m_vector[i].mp_subtree)

            continue;

        else if (m_vector[i].mp_subtree->is_leaf()) {

            delete m_vector[i].mp_subtree;

            count_deleted++;

        }

        else

            count_deleted += m_vector[i].mp_subtree->delete_all_subtrees();

    }

    return count_deleted;

} 

 

bool Node::vector_insert (Elem& element) {

// this method merely tries to insert the argument into the current node.
// it does not concern itself with the entire tree.
// if the element can fit into m_vector, slide all the elements
// greater than the argument forward one position and copy the argument
// into the newly vacated slot, then return true.  otherwise return false.
// note: the tree_insert method will already have verified that the key
// value of the argument element is not present in the tree.


    if (m_count >= m_vector.size()-1) // leave an extra slot for split_insert
        return false;

    int i = m_count;

    while (i>0 && m_vector[i-1]>element) {
        m_vector[i] = m_vector[i-1];
        i--;
    }

    if (element.mp_subtree)
        element.mp_subtree->mp_parent = this;

    m_vector[i] = element;

    m_count++;

    return true;

} 

 

bool Node::vector_delete (Elem& target) {

// delete a single element in the vector belonging to *this node.
// if the target is not found, do not look in subtrees, just return false.

    int target_pos = -1;
    int first = 1;
    int last = m_count-1;

    // perform binary search
    while (last-first > 1) {
        int mid = first+(last-first)/2;

        if (target>=m_vector[mid])
            first = mid;
        else
            last = mid;
    }

    if (m_vector[first]==target) {
        target_pos = first;
    }

    else if (m_vector[last]==target) {
        target_pos = last;
    }

    else {
        return false;
    }
    
    // the element's subtree, if it exists, is to be deleted or re-attached
    // in a different function.  not a concern here.  just shift all the
    // elements in positions greater than target_pos.
    for (unsigned int i= target_pos; i < m_count; i++) {
        m_vector[i] = m_vector[i+1];
    }
   
    m_count--;

    return true;

}  

bool Node::vector_delete (int target_pos) {

// delete a single element in the vector belonging to *this node.
// the element is identified by position, not value.

    if (target_pos<0 || target_pos>=(int)m_count)

        return false;

  

    // the element's subtree, if it exists, is to be deleted or re-attached
    // in a different function.  not a concern here.  just shift all the
    // elements in positions greater than target_pos.
    for (unsigned int i= (unsigned int)target_pos; i<m_count; i++)

        m_vector[i] = m_vector[i+1];

   

    m_count--;

    return true;

} 

 
bool Node::vector_insert_for_split (Elem& element) {

// this method insert an element that is in excess of the nominal capacity of
// the node, using the extra slot that always remains unused during normal
// insertions.  this method should only be called from split_insert()

    if (m_count >= m_vector.size()) // error

        return false;

    int i = m_count;

   

    while (i>0 && m_vector[i-1]>element) {

        m_vector[i] = m_vector[i-1];

        i--;

    }

    if (element.mp_subtree)

        element.mp_subtree->mp_parent = this;

    m_vector[i] = element;

    m_count++;

    return true;

}



/* split_insert should only be called if node is full */
bool Node::split_insert (Elem& element) {

    if (m_count != m_vector.size()-1) {
        throw "bad m_count in split_insert";
    }
 
    vector_insert_for_split(element);

    unsigned int split_point = m_count/2;

    if (2*split_point < m_count) { // perform the "ceiling function"
        split_point++;
    }

    // new node receives the rightmost half of elements in *this node
    Node* new_node = new Node(m_root);

    Elem upward_element = m_vector[split_point];

    new_node->insert_zeroth_subtree (upward_element.mp_subtree);

    upward_element.mp_subtree = new_node;

    // element that gets added to the parent of this node
    for (int i=1; i< (int)(m_count-split_point); i++) {
        new_node->vector_insert(m_vector[split_point+i]);
    }
    
    new_node->m_count = m_count-split_point;

    m_count = split_point;

    new_node->mp_parent = mp_parent;

    // --- Merkle ----------------
    new_node->update_merkle();
    update_merkle_upward();
    //-----------------------------

    // now insert the upward_element into the parent, splitting it if necessary
    if (mp_parent && mp_parent->vector_insert(upward_element)) {
	return true;
    }

    else if (mp_parent && mp_parent->split_insert(upward_element))
        return true;

    else if (!mp_parent) { // this node was the root
        Node* new_root = new Node(m_root);

        new_root->insert_zeroth_subtree(this);

        this->mp_parent = new_root;

        new_node->mp_parent = new_root;

        new_root->vector_insert (upward_element);

        m_root.set_root (m_root.get_root(),  new_root);

        new_root->mp_parent = 0;

	//----Merkle----------
	update_merkle_upward();
	//--------------------

    }
    return true;
}

template<class payload>
string
Element<payload>::get_subtree_merkle() {
    if (has_subtree()) {
	return mp_subtree->merkle_hash;
    } else {
	return hash_empty_node;
    }
}

// notextract is a position for which we should
// not extract the hash because it may have changed from the
// time we want to snaposhot
NodeInfo
Node::extract_NodeInfo(int notextract = -1) {
    NodeInfo ni = NodeInfo();

    if (this == get_root()) {
	ni.key = "";
	ni.pos_in_parent = invalid_index;
    } else {
	ni.pos_in_parent = index_has_subtree();
	ni.key = mp_parent->m_vector[ni.pos_in_parent].m_key;
    }
	
    ni.childr.resize(m_count);

    for (int i = 0; i < (int)m_count; i++) {
	Elem e = m_vector[i];
	if (i == notextract) {
	    ni.childr[i] = ElInfo(e.m_key, hash_not_extracted);	    
	} else {
	    ni.childr[i] = ElInfo(e.m_key, e.get_subtree_merkle());
	}
    }

    return ni;
}

// returns the index of this node in its parent vector
// or negative if this does not have a parent (is root)
int
Node::index_in_parent() {
    if (this == get_root()) {
	    return -1;
    } else {
	return mp_parent->index_of_child(this);
    }
}

MerkleProof
node_merkle_proof(Node * start_node) {
    MerkleProof p;
    Node * node = start_node;
    
    while (node) {
	p.path.push_back(node->extract_NodeInfo());
	node = node->mp_parent;
    }

    return p;
}



static bool
Merkle_path_hash(const MerkleProof & proof, string & root_hash){

    string hash =  "";

    for (auto ni = proof.path.begin(); ni != proof.path.end(); ni++) {
	hash = ni->hash();
	if (ni->pos_in_parent >= 0) {
	    auto ni2 = ni;
	    auto parent = ++ni2;
	    if (parent->childr[ni->pos_in_parent].hash != hash) {
		cerr << "child hash does not verify in parent\n";
		return false;
	    }
	}
    }

    if (hash != root_hash) {
	cerr << "root hash does not verify\n";
	return false;
    }

    return true;
}


bool
verify_merkle_proof(const MerkleProof & proof, const string & merkle_root) {

  
    // check that the resulting hash matches the root Merkle hash
    string computed_hash;
    bool r = Merkle_path_hash(proof, computed_hash);
    if (!r) {
	return false;
    }
    if (computed_hash != merkle_root) {
	return false;
    }

    return true;
}


static bool
verify_update_merkle_proof(bool is_del,
			   const UpdateMerkleProof & proof,
			   string key_target,
			   const string & old_merkle_root,
			   string & new_merkle_root) {
    
    if (DEBUG_PROOF) { cerr << "\n\n start verify\n";}

    if (old_merkle_root != proof.old_hash()) {
	cerr << "merkle hash of old state does not verify \n";
	return false;
    }

    bool r;

    if (is_del) {
	r = proof.check_del_change(key_target);
    } else {
	r = proof.check_ins_change(key_target);
    }
    
    if (!r) {
	cerr << "incorrect update information";
	return false;
    }

    new_merkle_root = proof.new_hash();
    
    return true;
}

bool verify_ins_merkle_proof(const UpdateMerkleProof & proof,
			string ins_target,
			const string & old_merkle_root,
			string & new_merkle_root) {
    return verify_update_merkle_proof(false, proof, ins_target, old_merkle_root, new_merkle_root);
    
}

bool
verify_del_merkle_proof(const UpdateMerkleProof & proof,
			string del_target,
			const string & old_merkle_root,
			string & new_merkle_root) {

    return verify_update_merkle_proof(true, proof, del_target, old_merkle_root, new_merkle_root);
}


// for gdb -- could not figure out how to call template funcs from
// gdb, so the below has repetitions
const char *
myprint(const MerkleProof & v) {

    return v.pretty().c_str();
}

const char *
myprint(const UpdateMerkleProof & v) {

    return v.pretty().c_str();
}

const char *
myprint(const NodeInfo & v) {
    return v.pretty().c_str();
}

const char *
myprint(const Node * n) {
    return n->pretty().c_str();
}
const char *
myprint(const Elem & e) {
    return e.pretty().c_str();
}

const char *
myprint(const ElInfo & v) {
    return v.pretty().c_str();
}
const char *
myprint(const DelInfo & v) {
    return v.pretty().c_str();
}

const char *
myprint(const Node & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
}

string
Node::hash_node(){
    uint repr_size = ElInfo::repr_size;
 
    string hashes_concat = string(m_count * repr_size, 0);

    for (uint i = 0 ; i < m_count ; i++) {
	Elem e = m_vector[i];
	hashes_concat.replace(i*repr_size, repr_size, e.repr());
    }
    
    return sha256::hash(hashes_concat);
 
}


ostream &
operator<<(ostream & out, const Node & n) {
    out << "(Node m_ct " << n.m_count << " m_vec ";
    for (uint i = 0; i < n.m_count; i++) {
	out << n.m_vector[i] <<  " ";
    }
    out << ")";
    return out;
}

void Node::update_merkle() {
    merkle_hash = hash_node();
}

void Node::update_merkle_upward() {
    update_merkle();
    
    if (this != get_root()) {
	//update siblings
	int parent_index_this;
	Node * right = right_sibling(parent_index_this);
	if (right) {
	    right->update_merkle();
	}
	Node * left = left_sibling(parent_index_this);
	if (left) {
	    left->update_merkle();
	}
	mp_parent->update_merkle_upward();
    }
}

bool Node::tree_insert_help(Elem & element, UpdateMerkleProof & p) {
    // ---- Merkle -----
    // last_visited_ptr is the start point for the proof
    record_state(this, p.st_before);
    // -----------------

    // ---- bench ----
    // insert happens at last_visited_ptr
    Merklecost += Merkle_cost();
    //---------------
    
    // insert the element in last_visited_ptr if this node is not fulls
    if (vector_insert(element)) {
	cerr << "basic insert\n";
        // -----Merkle ----
	// done making the changes for insert so update merkle hash and record
	// new state
	update_merkle_upward();
	record_state(this, p.st_after);
	// ----------------
	
        return true;
    }

    //last_visited_ptr node is full so we will need to split
    bool r = split_insert(element);
    cerr << "split_insert\n";
    // -----Merkle ----
    // done making the changes for insert so update merkle hash and record
    // new state
    update_merkle_upward();
    record_state(this, p.st_after);
    // ----------------

    return r;  
}
bool Node::tree_insert(Elem& element, UpdateMerkleProof & p) {

    Node* last_visited_ptr = this;

    if (search(element, last_visited_ptr).valid()) { // element already in tree
	cerr << "element to insert exists already\n";
        return false;
    }

    return last_visited_ptr->tree_insert_help(element, p);

} 


void
record_state(Node * node, State & state) {

    bool is_root = false;
    if (node != node->get_root()) {
	record_state(node->mp_parent, state);
    } else {
	is_root = true;
    }

    DelInfo di;
    di.node = node->extract_NodeInfo();
    if (is_root) {
	di.has_left_sib = false;
	di.has_right_sib = false;
    } else {
	int pos = di.node.pos_in_parent;
	Node * right = node->right_sibling(pos);
	Node * left = node->left_sibling(pos);
	if (right) {
	    di.has_right_sib = true;
	    di.right_sib = right->extract_NodeInfo();
	}
	if (left) {
	    di.has_left_sib = true;
	    di.left_sib = left->extract_NodeInfo();
	}
    }
    state.push_back(di);
}

bool
Node::tree_delete(Elem & target, UpdateMerkleProof & proof) {

    if (DEBUG_PROOF) { cerr << "\n\n start delete\n";}
    // first find the node contain the Elem instance with the given key

    Node* node = 0;
    Node* start_node = 0;

    Elem& found = search(target, node);

    if (!found.valid()) {
        return false;
    }

    bool r;

    if (!node->is_leaf()) {

	if (DEBUG_PROOF) { cerr << "not leaf\n";}

	Elem& smallest_in_subtree = found.mp_subtree->smallest_key_in_subtree();

	Node * node2 = 0;
	Elem& found2 = found.mp_subtree->search(smallest_in_subtree, node2);

	assert_s(found2.valid(), "should have found it");

	// ---- Merkle ------
	// record old state
	start_node = node2;
	record_state(start_node, proof.st_before);
	// -----------------

        found.m_key = smallest_in_subtree.m_key;

        found.m_payload = smallest_in_subtree.m_payload;
	
        r = found.mp_subtree->tree_delete_help(smallest_in_subtree, proof, start_node, node2);
	
    } else {
	// ---- Merkle ----------------
	// This node is starting point for change
	start_node = node;
	record_state(node, proof.st_before);
	// ------------------------------

	r = tree_delete_help(target, proof, start_node, node);
	
    }

    // --- Merkle -------
    start_node->update_merkle_upward();

    record_state(start_node, proof.st_after);
    // --------------
    
    return r;
}

bool
Node::tree_delete_help (Elem& target, UpdateMerkleProof & proof, Node * & start_node, Node * node) {

// target is just a package for the key value.  the reference does not
// provide the address of the Elem instance to be deleted.

    int parent_index_this = invalid_index;

    bool at_leaf_level = true;
    
    assert_s(node->is_leaf(), "node should be leaf");

    if (node->key_count() > (int)b_min_keys) {	
	if (DEBUG_PROOF) { cerr << "basic delete enough elems; my key count " << node->key_count()
			  << " min " << b_min_keys << " \n"; }
	
	bool r = node->vector_delete(target);

	
	return r;
    } else {
	
	node->vector_delete(target);
	
	// loop invariant: if _node_ is not null_ptr, it points to a node
	// that has lost an element and needs to import one from a sibling
	// or merge with a sibling and import one from its parent.
	// after an iteration of the loop, _node_ may become null or
	// it may point to its parent if an element was imported from the
	// parent and this caused the parent to fall below the minimum
	// element count.
	
	
	DelInfo di_parent;
	
	while (node) {
	    
	    // NOTE: the "this" pointer may no longer be valid after the first
	    // iteration of this loop!!!
	    
	    if (node==node->get_root() && node->is_leaf()) {
		cerr << "delete root \n";
		//empty tree
		break;
	    }
	    
	    if (node==node->get_root() && !node->is_leaf()) { // sanity check
		throw "node should not be root in delete_element loop";
	    }
	    
	    Node* right = node->right_sibling(parent_index_this);
	    Node* left = node->left_sibling(parent_index_this);
	    
	    // is an extra element available from the right sibling (if any)         
	    if (right && right->key_count() > (int)b_min_keys) {
		// node will be null after this function, because delete
		// terminates, so the function will finalize the proof
		if (DEBUG_PROOF) { cerr << "rotate right \n";}
		node->rotate_from_right(parent_index_this);
		break;
	    }
	    
		else if (left && left->key_count() > (int)b_min_keys) {
		// check if an extra element is available from the left sibling (if any)
		// node will be null after this function, because delete
		// terminates, so the function will finalize the proof
		if (DEBUG_PROOF) { cerr << "rotate left \n";}
		node->rotate_from_left(parent_index_this);
		break;
	    }
	    else if (right) {
		if (DEBUG_PROOF) { cerr << "merge right \n";}
		node = node->merge_right(parent_index_this);
	    }
	    else if (left) {
		// current node will be deleted
		if (at_leaf_level) {
		    start_node = left;
		}
		if (DEBUG_PROOF) { cerr << "merge left \n";}
		node = node->merge_left(parent_index_this);
	    }
	    
	    at_leaf_level = false;
	}
	
    }
    

    if (DEBUG_PROOF) {
	cerr << "done delete\n";
    }
    return true;

}


void
Node::rotate_from_right(int parent_index_this){

    // new element to be added to this node
    Elem underflow_filler = (*mp_parent)[parent_index_this+1];

    // right sibling of this node

    Node* right_sib = (*mp_parent)[parent_index_this+1].mp_subtree;

    underflow_filler.mp_subtree = (*right_sib)[0].mp_subtree;

    // copy the entire element

    (*mp_parent)[parent_index_this+1] = (*right_sib)[1];

    // now restore correct pointer

    (*mp_parent)[parent_index_this+1].mp_subtree = right_sib;

    vector_insert(underflow_filler);

    right_sib->vector_delete(0);

    (*right_sib)[0].m_key = "";

    (*right_sib)[0].m_payload = "";

    // parent node still has same element count

 
}

 

void
Node::rotate_from_left(int parent_index_this){

    // new element to be added to this node

    Elem underflow_filler = (*mp_parent)[parent_index_this];

    // left sibling of this node

    Node* left_sib = (*mp_parent)[parent_index_this-1].mp_subtree;

    underflow_filler.mp_subtree = (*this)[0].mp_subtree;

    (*this)[0].mp_subtree = (*left_sib)[left_sib->m_count-1].mp_subtree;

    if ((*this)[0].mp_subtree)

        (*this)[0].mp_subtree->mp_parent = this;

    // copy the entire element

    (*mp_parent)[parent_index_this] = (*left_sib)[left_sib->m_count-1];

    // now restore correct pointer

    (*mp_parent)[parent_index_this].mp_subtree = this;

    vector_insert (underflow_filler);

    left_sib->vector_delete(left_sib->m_count-1);

    // parent node still has same element count


} 

 

Node*
Node::merge_right (int parent_index_this) {
// copy elements from the right sibling into this node, along with the
// element in the parent node vector that has the right sibling as it subtree.
// the right sibling and that parent element are then deleted


    Elem parent_elem = (*mp_parent)[parent_index_this+1];

    Node* right_sib = (*mp_parent)[parent_index_this+1].mp_subtree;

    parent_elem.mp_subtree = (*right_sib)[0].mp_subtree;

    vector_insert (parent_elem);

    for (unsigned int i=1; i<right_sib->m_count; i++) {
        vector_insert ((*right_sib)[i]);
    }

    mp_parent->vector_delete(parent_index_this+1);

    delete right_sib;

    if (mp_parent==get_root() && !mp_parent->key_count()) {

        m_root.set_root(m_root.get_root(), this);
	cerr << "delete root\n";
        delete mp_parent;
        mp_parent = 0;

        return null_ptr;
    }
    else if (mp_parent==get_root() && mp_parent->key_count()) {
        return null_ptr;
    }
    if (mp_parent && (mp_parent->key_count() >= (int)b_min_keys)) {
        return null_ptr; // no need for parent to import an element
    }
 
    return mp_parent; // parent must import an element

} //_______________________________________________________________________

//TODO: extract_nodeInfo reaches beyond current node which is bad design

Node* Node::merge_left (int parent_index_this) {

// copy all elements from this node into the left sibling, along with the
// element in the parent node vector that has this node as its subtree.
// this node and its parent element are then deleted.

    Elem parent_elem = (*mp_parent)[parent_index_this];

    parent_elem.mp_subtree = (*this)[0].mp_subtree;

    Node* left_sib = (*mp_parent)[parent_index_this-1].mp_subtree;

    left_sib->vector_insert(parent_elem);

    for (unsigned int i=1; i<m_count; i++) {
        left_sib->vector_insert(m_vector[i]);
    }
    
    mp_parent->vector_delete(parent_index_this);

    Node* parent_node = mp_parent;  // copy before deleting this node

      
    if (mp_parent==get_root() && !mp_parent->key_count()) {

        m_root.set_root(m_root.get_root(), left_sib);

	cerr << "delete root\n";
        delete mp_parent;

        left_sib->mp_parent = null_ptr;
	
        delete this;

        return null_ptr;

    } 
    else if (mp_parent==get_root() && mp_parent->key_count()) {

        delete this;

	return null_ptr;

    }

    //mp_parent not root
    delete this;

    if (parent_node->key_count() >= (int)b_min_keys) {

        return null_ptr; // no need for parent to import an element
    }

    return parent_node; // parent must import an element

} //_______________________________________________________________________

 
// outputs sibling and parent_index_this
Node* Node::right_sibling (int& parent_index_this) {

    parent_index_this = index_has_subtree (); // for element with THIS as subtree

    if (parent_index_this == invalid_index)

        return 0;

    // now mp_parent is known not to be null

    if (parent_index_this >= (int) (mp_parent->m_count-1))

        return 0;  // no right sibling

    return mp_parent->m_vector[parent_index_this+1].mp_subtree;  // might be null

} //__________________________________________________________________________

 
// outputs sibling and parent_index_this
Node* Node::left_sibling (int& parent_index_this) {

    parent_index_this = index_has_subtree (); // for element with THIS as subtree

    if (parent_index_this == invalid_index)

        return 0;

    // now mp_parent is known not to be null

    if (parent_index_this==0)

        return 0;  // no left sibling

    return mp_parent->m_vector[parent_index_this-1].mp_subtree;  // might be null

} //____________________________________________________________________________

 
Elem &
Node::smallest_key() {
    assert_s(m_count > 0, "node is empty so not smallest key");
    return m_vector[1];
}

int Node::index_has_subtree () {

// return the element in this node's parent that has the "this" pointer as its subtree

    if (!mp_parent) {
	return invalid_index;
    }
    
    if (key_count() > 0) { //binary search
	int first = 0;
	int last = mp_parent->m_count-1;
	
	while (last-first > 1) {
	    int mid = first+(last-first)/2;
	    
	    if (smallest_key()>=mp_parent->m_vector[mid])
		first = mid;
	    else
		last = mid;
	}
	
	if (mp_parent->m_vector[first].mp_subtree == this) {
	    return first;
	}
	else if (mp_parent->m_vector[last].mp_subtree == this) {
	    return last;
	}
	else {
	    throw "error in index_has_subtree";
	}
    } else {//exhaustive search because there is no key
	for (uint i = 0; i < mp_parent->m_count; i++) {
	    if (this == mp_parent->m_vector[i].mp_subtree) {
	        return i;
	    }
	}
    }
    
    assert_s(false, "node not found in parent");
    return 0;
   
} 

Elem& Node::smallest_key_in_subtree () {
    
    if (is_leaf()) {	
        return m_vector[1];
    }
    else {
        return m_vector[0].mp_subtree->smallest_key_in_subtree();
    }
} //___________________________________________________________________

 

Elem& Node::search (Elem& desired, Node*& last_visited_ptr) {

    // the zeroth element of the vector is a special case (no key value or
    // payload, just a subtree).  the seach starts at the *this node, not
    // at the root of the b-tree.

    Node* current = this;

    if (!key_count())
        current = 0;

    while (current) {

        last_visited_ptr = current;

        // if desired is less than all values in current node

        if (current->m_count>1 && desired<current->m_vector[1])

            current = current->m_vector[0].mp_subtree;

        // if desired is greater than all values in current node

        else if (desired>current->m_vector[current->m_count-1])

            current = current->m_vector[current->m_count-1].mp_subtree;

        else {

            // binary search of the node
            int first = 1;

            int last = current->m_count-1;

            while (last-first > 1) {
                int mid = first+(last-first)/2;

                if (desired>=current->m_vector[mid])
                    first = mid;
                else
                    last = mid;
            }

            if (current->m_vector[first]==desired)
                return current->m_vector[first];

            if (current->m_vector[last]==desired)
                return current->m_vector[last];

            else if (current->m_vector[last]>desired)
                current = current->m_vector[first].mp_subtree;
            else
                current = current->m_vector[last].mp_subtree;

        }

    }

    return m_failure;
} 


// initialize static data at file scope

Elem Node::m_failure = Elem();
 
void Node::check_merkle_tree() {
    
   for (uint i = 0; i < m_count; i++) {
	if (m_vector[i].has_subtree()) {
	    m_vector[i].mp_subtree->check_merkle_tree();
	}
    }
 
    string hash = hash_node();
   
    if (hash != merkle_hash) {
	cerr << "merkle hash does not match; ";
	cerr << "it is " << read_short(short_string(merkle_hash)) << " should be " << read_short(short_string(hash)) << "\n";
	cerr << "dump: \n";	
	dump(false);
	
        assert_s(false, "merkle hash not verified");
    }
        
}

void Node::recompute_merkle_subtree() {
    // recompute for children first
    for (uint i = 0; i < m_count; i++) {
	if (m_vector[i].has_subtree()) {
	    cerr << "recompute merkle: has subtree\n";
	    m_vector[i].mp_subtree->check_merkle_tree();
	}
    }
    merkle_hash = hash_node();
}

uint
Node::max_height() {

    uint maxh = 0;
    for (uint i = 0; i < m_count; i++) {
	Elem e = m_vector[i];
	if (e.has_subtree()) {
	    uint child_height = e.mp_subtree->max_height();
	    if (child_height + 1 > maxh) {
		maxh = child_height + 1;
	    }
	}
    }
    return maxh;
}

void
Node::in_order_traverse(list<string> & result) {
    for (unsigned int i=0; i<m_count; i++) {
	if (m_vector[i].m_key != "") {
	    result.push_back(m_vector[i].m_key);
	}
	if (m_vector[i].has_subtree()) {	    
	    m_vector[i].mp_subtree->in_order_traverse(result);
	}
    }
}


ostream&
operator<<(ostream & out, const Elem & e) {
    out << "Elem: (" << e.m_key << ", ";
    if (e.has_subtree()) {
	out << read_short(short_string(e.mp_subtree->merkle_hash)); 
    } else {
	out << "";
    }
    out << ")";
    return out;
}


template<class payload>
string
Element<payload>::repr() {
    return format_concat(m_key, b_key_size, get_subtree_merkle(), sha256::hashsize);
}

template<class payload>
Element<payload>::Element(string key) {
    assert_s(key.size() < b_key_size, "given key to element is larger than key_size");
    this.key = key;
    mp_subtree = NULL;
}

int
Node::index_of_child(Node * child) {
    for (uint i = 0 ; i < m_count ; i++) {
	Elem e = m_vector[i];
	if (e.has_subtree() && child == e.mp_subtree) {
	    return i;
	}
    }

    return -1;
}

string
Node::pretty() const {
    stringstream ss;
    ss << "[Node " << this;
    ss << " keys: ";
    for (uint i = 0; i < m_count; i++) {
	ss << m_vector[i].pretty() << " ";
    }
    ss << "]";
    return ss.str();
}

template<class payload>
string
Element<payload>::pretty() const {
    stringstream ss;
    ss << "(" << m_key << ", sub: " << mp_subtree << ")";
    return ss.str();
}


template<class EncT>
BTree<EncT>::BTree(OPETable<EncT> * ot, Connect * _db) : ope_table(ot), _db(db) {
    Node::m_failure.invalidate();
    Node::m_failure.m_key = "";
    RootTracker  * tracker = new RootTracker() ;  // maintains a pointer to the current root of the b-tree
    Node* root_ptr = new Node(*tracker);
    tracker->set_root(null_ptr, root_ptr);
}
