/*

  B-tree implementation, adapted from toucan@textelectric.net, 2003

*/

#include "btree.hh"


using namespace std;


 
template<class payload> void Element<payload>::dump () {
    std::cout << " (key = " <<  m_key << "\n";
/*
	      <<

	" mhash = ";
    if (has_subtree()) {
	cout << mp_subtree->merkle_hash << ") "; 
    } else {
	cout << "NO SUBTREE) ";
    }
*/
} 

int Node::minimum_keys () {

    // minus 1 for the empty slot left for splitting the node

    int size = m_vector.size();

    int ceiling_func = (size-1)/2;

    if (ceiling_func*2 < size-1)

        ceiling_func++;

    return ceiling_func-1;  // for clarity, may increment then decrement

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

// limit the size of the vector to 4 kilobytes max and 200 entries max.
// TODO: sizeof(Elem) does not compute the size correctly because string contains pointer
    int num_elements = max_elements*sizeof(Elem)<=max_array_bytes ?	
	max_elements : max_array_bytes/sizeof(Elem);
    
    if (num_elements < 6) { // in case key or payload is really huge
	num_elements = 6;
    }
    
    m_vector.resize(num_elements);
    
    m_count = 0;
    
    mp_parent = 0;

    merkle_hash = "0"; 
    
    insert_zeroth_subtree (0);
}

 

Node* Node::find_root () {

    Node* current = this;

    while (current->mp_parent)

        current = current->mp_parent;

    return current;

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

    this->update_merkle();
    new_node->update_merkle();
    
    // now insert the upward_element into the parent, splitting it if necessary
    if (mp_parent && mp_parent->vector_insert(upward_element)) {
	mp_parent->update_merkle_upward();
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

	//update merkle hash of the root
	new_root->update_merkle();

    }
    return true;
}

string Elem::get_subtree_merkle() {
    if (has_subtree()) {
	return mp_subtree->merkle_hash;
    } else {
	return "0";
    }
}

merkleInfoForLevel& extract_node_merkle_info(int pos) {
    merkleInfoForLevel mil = merkleInfoForLevel();
    mil.children.resize(m_vector.size());
    mil.pos = pos;
    uint ind = 0;
    for (Elem e : m_vector) {
	mil.children[ind] = make_pair<string, string>(e.m_key,
						      e.get_subtree_merkle());
    }
    return mil;
}

merkleInfo Node::merkle_info_for_check() {
    Node * curr_node = this;

    merkleInfo mi = merkleInfo();

    // TODO: is find_root efficient?, should be constant workmak
    
    while (true) {

	if (curr_node->find_root() == curr_node) {
	    int pos = 
	    mil.push_back(extract_node_merkle_info(-1));
	    break;
	} else {
	    mil.push_back(extract_node_merkle_info(node_pos()));
	    curr_node = curr_node->mp_parent;
	}
    }

    return mi;
}

bool check_merkle_info(const merkleInfo & mi, const string & root_hash) {
    string hash = "";
    for (merkleInfoForLevel mil : mi) {
	clean up node hash and merkle hash and all these hashes, also in interface
    }
}

string
Node::hash_node(){
    uint blocksize = sha256::hashsize + nodekeysize ;

    string hashes_concat = string(m_count * blocksize, 0);

    for (uint i = 0 ; i < m_count ; i++) {
	Elem e = m_vector[i];
	assert_s(e.m_key.size() <= nodekeysize, "size of key is larger than max allowed");
	string mkey = e.m_key;
	hashes_concat.replace(i*blocksize, mkey.size(), mkey);

	string hash = e.get_subtree_merkle();
	assert_s(hash.size() <= sha256::hashsize, "size of hash is larger than sha256::hashsize");
	hashes_concat.replace(i*blocksize + nodekeysize, hash.size(), hash);
    }
    
    return sha256::hash(hashes_concat);
 
}

void Node::update_merkle() {
    merkle_hash = hash_node();
}

void Node::update_merkle_upward() {
    update_merkle();
    
    if (this != find_root()) {
	mp_parent->update_merkle_upward();
    }
}
 

bool Node::tree_insert(Elem& element) {

    Node* last_visited_ptr = this;

    if (search(element, last_visited_ptr).valid()) { // element already in tree
        return false;
    }

    // insert the element in last_visited_ptr if this node is not full
    if (last_visited_ptr->vector_insert(element)) {
	last_visited_ptr->update_merkle_upward();
        return true;
    }

    //last_visited_ptr node is full so we will need to split
    return last_visited_ptr->split_insert(element);

} //__________________________________________________________________

 

bool Node::delete_element (Elem& target) {

// target is just a package for the key value.  the reference does not
// provide the address of the Elem instance to be deleted.

    // first find the node contain the Elem instance with the given key

    Node* node = 0;

    int parent_index_this = invalid_index;

    Elem& found = search(target, node);

    if (!found.valid()) {
        return false;
    }
 
    if (node->is_leaf() && node->key_count() > node->minimum_keys()) {
        bool r = node->vector_delete(target);
	node->update_merkle_upward();
	return r;
    }
    else if (node->is_leaf()) {
        node->vector_delete(target);

        // loop invariant: if _node_ is not null_ptr, it points to a node
        // that has lost an element and needs to import one from a sibling
        // or merge with a sibling and import one from its parent.
        // after an iteration of the loop, _node_ may become null or
        // it may point to its parent if an element was imported from the
        // parent and this caused the parent to fall below the minimum
        // element count.

        while (node) {
	    node->update_merkle();
            // NOTE: the "this" pointer may no longer be valid after the first
            // iteration of this loop!!!

            if (node==node->find_root() && node->is_leaf()) {
                break;
	    }

            if (node==node->find_root() && !node->is_leaf()) // sanity check
                throw "node should not be root in delete_element loop";

	    Node* right = node->right_sibling(parent_index_this);
	    Node* left = node->left_sibling(parent_index_this);
   
            // is an extra element available from the right sibling (if any)         
            if (right && right->key_count() > right->minimum_keys()) {
		node = node->rotate_from_right(parent_index_this);
	    }
            else if (left && left->key_count() > left->minimum_keys()) {
		// check if an extra element is available from the left sibling (if any)
		node = node->rotate_from_left(parent_index_this);
	    }
	    else if (right) {
		node = node->merge_right(parent_index_this);
	    }
	    else if (left) {
		node = node->merge_left(parent_index_this);
	    }
	    
           
        }

    }

    else { // node is not leaf

	Elem& smallest_in_subtree = found.mp_subtree->smallest_key_in_subtree();

        found.m_key = smallest_in_subtree.m_key;

        found.m_payload = smallest_in_subtree.m_payload;
	
        found.mp_subtree->delete_element (smallest_in_subtree);
	
    }

    return true;

} //___________________________________________________________________

 

Node* Node::rotate_from_right(int parent_index_this) {

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

    //need to update this and its right sibling's merkle hash as well as all
    // the way up
    right_sib->update_merkle();
    this->update_merkle_upward();

    return null_ptr;
} //_______________________________________________________________________

 

Node* Node::rotate_from_left(int parent_index_this) {

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

    // need to update merkle hash of left sibling and this and upward to the
    // root
    left_sib->update_merkle();
    this->update_merkle_upward();

    return null_ptr;

} //_______________________________________________________________________

 

Node* Node::merge_right (int parent_index_this) {
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

    this->update_merkle();
    
    if (mp_parent==find_root() && !mp_parent->key_count()) {

        m_root.set_root(m_root.get_root(), this);

        delete mp_parent;
        mp_parent = 0;

        return null_ptr;
    }

    else if (mp_parent==find_root() && mp_parent->key_count()) {
	mp_parent->update_merkle_upward();
        return null_ptr;
    }

    if (mp_parent && (mp_parent->key_count() >= mp_parent->minimum_keys())) {
	mp_parent->update_merkle_upward();
        return null_ptr; // no need for parent to import an element
    }

    return mp_parent; // parent must import an element

} //_______________________________________________________________________

 

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

    left_sib->update_merkle();
    
    if (mp_parent==find_root() && !mp_parent->key_count()) {

        m_root.set_root(m_root.get_root(), left_sib);

        delete mp_parent;

        left_sib->mp_parent = null_ptr;
	
        delete this;

        return null_ptr;

    } 
    else if (mp_parent==find_root() && mp_parent->key_count()) {

        delete this;
	mp_parent->update_merkle_upward();
        return null_ptr;

    }

    //mp_parent not root
    delete this;

    if (parent_node->key_count() >= parent_node->minimum_keys()) {
	parent_node->update_merkle_upward();
        return null_ptr; // no need for parent to import an element
    }
    
    return parent_node; // parent must import an element

 

} //_______________________________________________________________________

 

Node* Node::right_sibling (int& parent_index_this) {

    parent_index_this = index_has_subtree (); // for element with THIS as subtree

    if (parent_index_this == invalid_index)

        return 0;

    // now mp_parent is known not to be null

    if (parent_index_this >= (int) (mp_parent->m_count-1))

        return 0;  // no right sibling

    return mp_parent->m_vector[parent_index_this+1].mp_subtree;  // might be null

} //__________________________________________________________________________

 

Node* Node::left_sibling (int& parent_index_this) {

    parent_index_this = index_has_subtree (); // for element with THIS as subtree

    if (parent_index_this == invalid_index)

        return 0;

    // now mp_parent is known not to be null

    if (parent_index_this==0)

        return 0;  // no left sibling

    return mp_parent->m_vector[parent_index_this-1].mp_subtree;  // might be null

} //____________________________________________________________________________

 

int Node::index_has_subtree () {

// return the element in this node's parent that has the "this" pointer as its subtree

    if (!mp_parent)

        return invalid_index;

    int first = 0;

    int last = mp_parent->m_count-1;

    while (last-first > 1) {

        int mid = first+(last-first)/2;

        smallest_key();

        if (smallest_key()>=mp_parent->m_vector[mid])

            first = mid;

        else

            last = mid;

    }

    if (mp_parent->m_vector[first].mp_subtree == this)

        return first;

    else if (mp_parent->m_vector[last].mp_subtree == this)

        return last;

    else

        throw "error in index_has_subtree";

} //__________________________________________________________________

 

Elem& Node::smallest_key_in_subtree () {

    if (is_leaf())

        return m_vector[1];

    else

        return m_vector[0].mp_subtree->smallest_key_in_subtree();

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
	cerr << "merkle hash does not match at node "; dump(false); cerr << "\n";
//	cerr << "here is the subtree \n";
//	dump();
	cerr << "here is the recomputed merkle node \n";
	recompute_merkle_subtree();
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

void Node::max_height_help(uint height, uint & max_height) {
    if (height > max_height) {
	max_height = height;
    }
    for (uint i = 0; i < m_count; i++) {
	Elem e = m_vector[i];
	if (e.has_subtree()) {
	    e.mp_subtree->max_height_help(height + 1, max_height);
	}
    }
}

uint Node::max_height() {
    uint max_height = 0;
    
    max_height_help(0, max_height);

    return max_height;
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
