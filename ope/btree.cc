/*

  B-tree + Merkle tree implementation,
  The initial B-tree was adapted from toucan@textelectric.net, 2003

*/

#include "btree.hh"


using namespace std;

// two special values of the hash,
// it is unlikely a real hash will take them
const string hash_empty_node = "0";
const string hash_not_extracted = "1";

// concatenates the m_key of an Elem (sized to length Elem::key_size bytes)
// with hash (sized to length sha256::hashsize)
static string
format_concat(const string & m_key, uint key_size, const string & hash, uint repr_size) {
    string repr = string(repr_size, 0);

    assert_s(m_key.size() <= key_size, "size of key is larger than max allowed");

    repr.replace(0, m_key.size(), m_key);
    
    assert_s(hash.size() <= sha256::hashsize, "size of hash is larger than sha256::hashsize");

    repr.replace(key_size, hash.size(), hash);

    return repr;
}


template<class payload>
void Element<payload>::dump () {
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

uint
Node::Merkle_cost() {
    if (!mp_parent) {
	return m_count;
    } else {
	uint cost = mp_parent->m_count;
	return cost + mp_parent->Merkle_cost();
    }
}

uint
Node::minimum_keys () {

    // minus 1 for the empty slot left for splitting the node

    int size = num_elements();

    int ceiling_func = (size-1)/2;

    if (ceiling_func*2 < size-1) {
        ceiling_func++;
    }

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

int
num_elements() {

// limit the size of the vector to 4 kilobytes max and 200 entries max.
// TODO: sizeof(Elem) does not compute the size correctly because string
// contains pointer
    
    int num_elements = max_elements*sizeof(Elem)<=max_array_bytes ?	
	max_elements : max_array_bytes/sizeof(Elem);
    
    if (num_elements < 6) { // in case key or payload is really huge
	num_elements = 6;
    }

    return num_elements;
}

Node::Node(RootTracker& root_track)  : m_root(root_track) {

    m_vector.resize(num_elements());
    
    m_count = 0;
    
    mp_parent = 0;

    merkle_hash = hash_empty_node; 
    
    insert_zeroth_subtree (0);
}

 

Node* Node::find_root () {

    Node* current = this;

    while (current->mp_parent) {

        current = current->mp_parent;
    }
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

//TODO: remove this and use only NodeInfo
NodeMerkleInfo
Node::extract_NodeMerkleInfo(int pos) {
    NodeMerkleInfo mi = NodeMerkleInfo();
    mi.childr.resize(m_count);
    mi.pos_of_child_to_check = pos;

    for (uint i = 0; i < m_count; i++) {
	Elem e = m_vector[i];
	mi.childr[i] = make_pair(e.m_key, e.get_subtree_merkle());
    }
    return mi;
}



// notextract is a position for which we should
// not extract the hash because it may have changed from the
// time we want to snaposhot
NodeInfo
Node::extract_NodeInfo(int notextract = -1) {
    NodeInfo ni = NodeInfo();

    if (this == find_root()) {
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
    if (this == find_root()) {
	    return -1;
    } else {
	return mp_parent->index_of_child(this);
    }
}

MerklePath
get_merkle_path(Node * curr_node) {

    MerklePath path = MerklePath();

    // TODO: is find_root efficient?, should be constant work

    int pos = -1;

    while (true) {
	path.push_back(curr_node->extract_NodeMerkleInfo(pos));

	if (curr_node == curr_node->find_root()) {
	    break;
	}
	pos = curr_node->index_in_parent();
	curr_node = curr_node->mp_parent;
    }

    return path;
}

MerkleProof
get_search_merkle_proof(Node * curr_node) {
    MerkleProof p = MerkleProof();
    p.path = get_merkle_path(curr_node);
    return p;
}

std::ostream&
operator<<(std::ostream &out, const NodeMerkleInfo & mi) {
    out << " children( ";
    for (int i=0; i<(int) mi.childr.size(); i++) {
    std::pair<std::string,std::string> c = mi.childr[i];
    if(c.first=="") c.first="null_key";
    if(c.second=="") c.second="null_key";
    out << c.first << " " << c.second << " ";
    }
    out << "; ";
    out << mi.pos_of_child_to_check;

    return out;
}

std::istream&
operator>>(std::istream &is, NodeMerkleInfo & mi) {
    std::string a;   
    std::string b;
    bool flag=false;
    is>>a;
    while(!is.eof()){
        is>>a;
        //cout<<a<<endl;
        if(a==";") {
            flag=true;
            continue;
        }
        if(!flag){
            is>>b;
            if(a=="null_key") a="";
            if(b=="null_key") b="";
            //cout<<b<<endl;
            mi.childr.push_back(make_pair(a,b));
        }else{
            mi.pos_of_child_to_check=atoi(a.c_str());
            break;
        }

    }
    return is;
}

std::ostream&
operator<<(std::ostream &out, const MerkleProof & mp){
    std::list<NodeMerkleInfo>::const_iterator it;
    for(it=mp.path.begin() ; it!=mp.path.end(); it++){
        NodeMerkleInfo nmi = *it;
        out<<nmi<<" | ";
    }
    out<<"endmp";
    return out;

}

std::istream&
operator>>(std::istream &is, MerkleProof & mp){
    std::stringstream s;
    std::string a;   
    while(!is.eof()){
        is>>a;
        if(a=="endmp"){
            break;
        }else if(a=="|") {
            NodeMerkleInfo nmi;
            s>>nmi;
            mp.path.push_back(nmi);
            s.str("");
            s.clear();
        }else{
            s<<a+" ";
        }

    }   
    return is;
}


ostream&
operator<<(ostream& out, const State & st) {
    for (uint i = 0; i < st.size(); i++) {
	out << "[" << st[i] << "]\n";
    }

    return out;
}
std::ostream&
operator<<(std::ostream &out, const UpdateMerkleProof & dmp){

    out << "== DelProof:\n before: " << dmp.st_before << "\n after: " << dmp.st_after << "\n";

/*   out<<dmp.oldproof<<" "<<dmp.newproof<<" ";
    out<<dmp.new_tree_empty<< " "<<dmp.is_sib<<" "<<dmp.sib_was_deleted<<" "<<dmp.pos_of_sib_in_parent<<" ";
    out<<dmp.non_leaf<<" "<<dmp.index_in_path<<" ";
    out<<dmp.oldsib<<" | "<<dmp.newsib<<" | "<<dmp.non_leaf_old<<" | "<<dmp.non_leaf_new<<" |";
    */

    return out;

}

std::istream&
operator>>(std::istream &is, UpdateMerkleProof & dmp){
/*    std::stringstream s;
    std::string a;   
    int state=0;
    while(true){
        is>>a;
        if(a=="endmp") {
            MerkleProof mp;
            s<<a;
            s>>mp;
            if(state==0){
                dmp.oldproof=mp;
                //cout<<"setting oldproof"<<endl;
                state++;
                s.str("");
                s.clear();
            }else if(state==1){
                dmp.newproof=mp;
                //cout<<"setting newproof "<<dmp.newproof<<endl;
                s.str("");  
                s.clear();          
                break;
            }
        }else{
            s<<a+" ";
            //cout<<a<<": "<<s.str()<<endl;
        }
    }
    is>>dmp.new_tree_empty>>dmp.is_sib>>dmp.sib_was_deleted>>dmp.pos_of_sib_in_parent;
    is>>dmp.non_leaf>>dmp.index_in_path;

    state=0;
    while(!is.eof()){
        is>>a;
        if(a=="|"){
            NodeMerkleInfo nmi;
            s>>nmi;
            switch(state){
                case 0: dmp.oldsib=nmi; break;
                case 1: dmp.newsib=nmi; break;
                case 2: dmp.non_leaf_old =nmi; break;
                case 3: dmp.non_leaf_new=nmi; break;
            }
            state++;
            s.str("");
            s.clear();
        }else{
            s<<a+" ";
        }
    }
*/  return is;

}

string
NodeMerkleInfo::hash() const {
    string concat = string(childr.size() * Elem::repr_size, 0);

    uint count = 0;

    for (auto p : childr) {
	concat.replace(Elem::repr_size * (count++), Elem::repr_size,
		       format_concat(p.first, Elem::key_size, p.second, Elem::repr_size));
    }

    return sha256::hash(concat);
}

string
NodeInfo::hash() const {
    string concat = string(childr.size() * Elem::repr_size, 0);

    uint count = 0;

    for (auto p : childr) {
	assert_s(p.hash != hash_not_extracted, "child with not extracted hash in NodeInfo::hash");
	concat.replace(Elem::repr_size * (count++), Elem::repr_size,
		       format_concat(p.key, Elem::key_size, p.hash, Elem::repr_size));
    }

    return sha256::hash(concat);
}


static bool
Merkle_path_hash(const MerkleProof & proof, string & hash){
    assert_s(proof.path.begin() != proof.path.end(), "proof is empty");
    
    NodeMerkleInfo origin_node = *(proof.path.begin());

    hash = "";

    for (auto mi : proof.path) {

	if (mi.pos_of_child_to_check >= 0) {
	    assert_s(hash.size() > 0, "position of child to check is >= 0 for origin node");
            // check that hash at pos_of_child_to_check corresponds to the one calc
	    // so far
	    if (mi.childr.at(mi.pos_of_child_to_check).second != hash) {
		cerr << " hash at pos of child does not match has so far \n";
		return false;
	    }
	}

	// compute parent's hash
	hash = mi.hash();
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

    bool r = is_del ? proof.check_del_change(key_target)
	: proof.check_ins_change(key_target);
    
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

static string
short_string(string s) {
    if (s.size() <= 10) {
	return s;
    }
    return s.substr(s.size() - 11, 10);
}
static string
read_short(string s) {
    stringstream r;
    for (uint i = 0; i < s.size(); i++) {
	r << (((int)s[i] - '0') % 10);
    }
    return r.str();
}
std::ostream&
operator<<(std::ostream &out, const ElInfo & el) {
    out << "(" << el.key << ", " << read_short(short_string(el.hash)) << ")";
    return out;
}

    
std::ostream&
operator<<(std::ostream &out, const NodeInfo & node) {
    out << "(NodeInfo: key " << node.key << " pos in parent " << node.pos_in_parent << " childr ";
    for (uint i = 0; i < node.childr.size(); i++) {
	out << node.childr[i] << " ";
    }
    out << ")";
    return out;
}

static void
hash_match(NodeInfo child, NodeInfo parent) {
    assert_s(child.hash() == parent.childr[child.pos_in_parent].hash, "hash mismatch parent and child");
}
static void
hash_match(const DelInfo & child, const DelInfo &  parent) {
    
    hash_match(child.node, parent.node);
    if (child.has_left_sib) {
	hash_match(child.left_sib, parent.node);
    }
    if (child.has_right_sib) {
	hash_match(child.right_sib, parent.node);
    }
    
}
string UpdateMerkleProof::old_hash() const {
    
    // first level
    auto it = st_before.begin();
    assert_s(it != st_before.end(), "levels in del proof empty");
    DelInfo di_parent = *it;
    it++;
    
    for (; it != st_before.end(); it++) {
	DelInfo di_child = *it;

	hash_match(di_child, di_parent);
	
	di_parent = di_child;
    }
    
    return st_before[0].node.hash();
}

// Should return the position pos where v[pos] <= target < v[pos+1]
static ElInfo &
binary_search(vector<ElInfo> & v, string target, bool & matched, int & pos) {
    int first = 0;
    int last = v.size() -1;
    while (last - first > 1) {
	int mid = first + (last - first)/2;

	if (target >= v[mid].key) {
	    first = mid;
	} else {
	    last = mid;
	}
    }

    matched = false;
    
    if (v[first].key == target) {
	matched = true;
	pos = first;
	return v[first];
    }

    if (v[last].key == target) {
	matched = true;
	pos = last;
	return v[last];
    }

    if (v[last].key > target) {
	pos = first;
	return v[first];
    } else {
	pos = last;
	return v[last];
    }

}
static ElInfo &
find(State & st, string target, int & index) {

    vector<ElInfo> & childr = st[index].node.childr;
    bool matched;
    int pos;
    ElInfo & ei = binary_search(childr, target, matched, pos);
    if (matched) {
	return ei;
    }
    assert_s(index+1 <= (int)st.size(), "not found!");
    assert_s(st[index+1].node.key == ei.key, " deletion path is not the correct one");

    index = index + 1;
    return find(st, target, index);  
}

static void
del_elinfo(int pos, vector<ElInfo> & v) {
    for (int i = pos; i < (int)v.size()-1; i++) {
	v[i] = v[i+1];
    }
    v.resize(v.size() - 1);
}

static void
del_elinfo(string del_target, vector<ElInfo> & v) {
    bool matched;
    int pos;
    binary_search(v, del_target, matched, pos);
    assert_s(matched, "del_elinfo received key to remove that does not exist in vector");
    del_elinfo(pos, v);
}

static bool
ins_elinfo(ElInfo ei, vector<ElInfo> & v) {

    if ((int)v.size() >= (int)num_elms - 1) {
	return false;
    }
    bool matched;
    int pos;
    binary_search(v, ei.key, matched, pos);
    v.resize(v.size()+1);
    assert_s(!matched, "del_elinfo received key to insert that already existsin vector");
    // must insert on pos + 1
    for (int i = (int)v.size()-2; i> pos ; i-- ) {
	v[i+1] = v[i];
    }
    v[pos+1] = ei;
    
    return true;
}

static void
rotate_from_right(State & st, int index) {

    if (DEBUG_PROOF) { cerr << "rotate right\n";}
    DelInfo & sl = st[index];

    NodeInfo & node = sl.node;
    NodeInfo & sib = sl.right_sib;

    assert_s(index-1 >= 0, "lack of parent");
    NodeInfo & parent = st[index-1].node;

    uint parent_index_this = node.pos_in_parent; 

    ElInfo filler = parent.childr[parent_index_this + 1];
    filler.hash = sib.childr[0].hash;
    
    parent.childr[parent_index_this + 1].key = sib.childr[1].key;
    sib.key = parent.childr[parent_index_this + 1].key;
    
    ins_elinfo(filler, node.childr);
    del_elinfo(0, sib.childr);
    sib.childr[0].key = "";
}

// changes keys of nodes on level index if they have oldkey to newkey
static void
change_key(State & st, int index, string oldkey, string newkey) {
    if (index <= (int)st.size()-1) {
	DelInfo & sl = st[index];
	if (sl.node.key == oldkey) {
	    sl.node.key = newkey;
	}
	if (sl.has_left_sib && sl.left_sib.key == oldkey) {
	    sl.left_sib.key = newkey;
	}
	if (sl.has_right_sib && sl.right_sib.key == oldkey) {
	    sl.right_sib.key = newkey;
	}
    }
}

static void
rotate_from_left(State & st, int index) {
    if (DEBUG_PROOF) { cerr << "rotate left\n";}
    DelInfo & sl = st[index];

    NodeInfo & node = sl.node;
    NodeInfo & sib = sl.left_sib;

    assert_s(index-1 >= 0, "lack of parent");
    NodeInfo & parent = st[index-1].node;

    uint parent_index_this = node.pos_in_parent; 

    ElInfo filler = parent.childr[parent_index_this];
    filler.hash = node.childr[0].hash;
    
    node.childr[0].hash = sib.childr[sib.childr.size()-1].hash;
    
    parent.childr[parent_index_this].key = sib.childr[sib.childr.size()-1].key;
    
    ins_elinfo(filler, node.childr);
    del_elinfo(sib.childr.size() - 1, sib.childr);

    node.key = parent.childr[parent_index_this].key;
    // if node has child with key ""
    change_key(st, index+1, "", filler.key);
 
}


static bool
merge_right(State & st, int index) {
    if (DEBUG_PROOF) { cerr << "merge right\n";}
    DelInfo & sl = st[index];
    DelInfo & sl_parent = st[index-1];
    
    NodeInfo & node = sl.node;
    NodeInfo & sib = sl.right_sib;

    assert_s(index-1 >=0, "lack of parent");
    NodeInfo & parent = sl_parent.node;

    uint parent_index_this = node.pos_in_parent;

    ElInfo parent_elem = parent.childr[parent_index_this+1];
    parent_elem.hash = sib.childr[0].hash;
    ins_elinfo(parent_elem, node.childr);
    
    for (uint i = 1; i < sib.childr.size(); i++) {
	ins_elinfo(sib.childr[i],node.childr);
    }
    
    del_elinfo(parent_index_this+1, parent.childr);
    
    sl.right_was_del = true;

    if (index - 1  == 0 && !parent.key_count()) {
	sl_parent.this_was_del = true;
	return true;
    }
    if (index - 1 == 0 && parent.key_count()) {
	return true;
    }

    if (parent.key_count() >= Node::minimum_keys()) {
	return true;
    }
    return false;
}


static bool
merge_left(State & st, int index) {
    if (DEBUG_PROOF) { cerr << "merge left\n";}
    DelInfo & sl = st[index];
    DelInfo & sl_parent = st[index-1];

    NodeInfo & node = sl.node;
    NodeInfo & sib = sl.left_sib;

    assert_s(index-1 >=0, "lack of parent");
    NodeInfo & parent = sl_parent.node;

    uint parent_index_this = node.pos_in_parent;

    ElInfo parent_elem = parent.childr[parent_index_this];
    
    parent_elem.hash = node.childr[0].hash;
    ins_elinfo(parent_elem, sib.childr);
    
    for (uint i = 1; i < node.childr.size(); i++) {
	ins_elinfo(node.childr[i], sib.childr);
    }
    
    del_elinfo(parent_index_this, parent.childr);
    
    sl.this_was_del = true;

    change_key(st, index+1, "", parent_elem.key);

    if (index - 1  == 0 && !parent.key_count()) {
	sl_parent.this_was_del = true;
	return true;
    }
    if (index - 1 == 0 && parent.key_count()) {
	return true;
    }

    if (parent.key_count() >= Node::minimum_keys()) {
	return true;
    }
  
    return false;
}


static ElInfo &
smallest_in_subtree(State & st, int index) {
   
    DelInfo & sl = st[index];
    assert_s(sl.node.childr.size() >= 2, "too few children"); // first is empty child
    ElInfo & el = sl.node.childr[1];
    
    if (index == (int)st.size()-1)  { // leaf
	assert_s(el.key != "" && el.hash == hash_empty_node, "incorrect deletion information");
	return el;
    }
    
    assert_s("" == st[index+1].node.key, "incorrect del path");
    return smallest_in_subtree(st, index+1);
}


// for gdb -- could not figure out how to call template funcs from
// gdb, so the below has repetitions
const char *
myprint(const UpdateMerkleProof & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
}
const char *
myprint(const NodeInfo & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
}
const char *
myprint(const ElInfo & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
}
const char *
myprint(const DelInfo & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
}
const char *
myprint(const Elem & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
}
const char *
myprint(const Node & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
}

static void
sim_delete(State & st, string del_target, int index) {

    ElInfo &  found = find(st, del_target, index);

  
    NodeInfo & node = st[index].node;
    
    if (index == (int)st.size()-1) { // leaf
	
	if ((int)node.key_count() > Node::minimum_keys()) {
	    if (DEBUG_PROOF) { cerr << "basic del enough elems; my key count " << node.key_count()
			      << " min " << Node::minimum_keys() << "\n"; }
	    del_elinfo(del_target, node.childr);
	    
	} else {
	    del_elinfo(del_target, node.childr);

	    bool done = false;

	    while (!done) {
		DelInfo & sl = st[index];

		if (index == 0 && st.size() == 1) {
		    st[0].this_was_del = true;
		    break;
		}

		if (index == 0 && st.size() > 1) { 
		    assert_s(false, "should not get at root in this loop");
		}

		if (sl.has_right_sib && sl.right_sib.key_count() > Node::minimum_keys()) {
		    rotate_from_right(st, index);
		    break;
		}

		if (sl.has_left_sib && sl.left_sib.key_count() > Node::minimum_keys()) {
		    rotate_from_left(st, index);
		    break;
		}

		if (sl.has_right_sib) {
		    done = merge_right(st, index);
		} else {
		    assert_s(sl.has_left_sib, "incorrect state tree, must have right or left sib");
		    done = merge_left(st, index);
		}
		
		index--;
	    }
	}
    } else {
	// non-leaf
	if (DEBUG_PROOF) { cerr << "non-leaf\n"; }
	assert_s(index < (int)st.size()-1 &&
		 st[index+1].node.key == found.key, "incorrect path after nonleaf");
	ElInfo sm_in_subtree = smallest_in_subtree(st, index+1);
	
	found.key  = sm_in_subtree.key;
	st[index+1].node.key = found.key;
	sim_delete(st, sm_in_subtree.key, index+1); 
    }
    
    if (DEBUG_PROOF) {cerr << "done sim delete\n";}

}


bool operator==(const ElInfo & e1, const ElInfo & e2) {
    return e1.key == e2.key && e2.hash == e2.hash;
}

static bool
operator==(const vector<ElInfo> & v1, const vector<ElInfo> & v2) {
    if (v1.size() != v2.size()) {
	return false;
    }
    for (uint i = 0; i < v1.size(); i++) {
	if (!(v1[i] == v2[i])) {
	    return false;
	}
    }

    return true;
}
bool
NodeInfo::equals(const NodeInfo & node) const {
    return (key == node.key) &&
	(pos_in_parent == node.pos_in_parent) &&
        (childr == node.childr);
}

static void
update_node_parent(NodeInfo & node, NodeInfo & parent) {
    if (node.pos_in_parent>=0 &&
	parent.childr.size() > (uint)node.pos_in_parent &&
	parent.childr[node.pos_in_parent].key == node.key) {
	// pos is correct
    } else {
	bool matched = false;
	binary_search(parent.childr, node.key, matched, node.pos_in_parent);
	assert_s(matched, "could not find child in parent");
    }

    // now pos_in_parent is up to date
    parent.childr[node.pos_in_parent].hash = node.hash();
}

// recomputes hashes and positions
static void
recomp_hash_pos(State & st) {

    // compute hashes for nodes in a level and update the parent
    
    for (int i = st.size()-1; i > 0; i--) {
	DelInfo & sl = st[i];
	DelInfo & sl_parent = st[i-1];
      
	if (!sl.this_was_del) {
	    if (sl_parent.this_was_del) {
		update_node_parent(sl.node, sl_parent.left_sib);
	    } else {
		update_node_parent(sl.node, sl_parent.node);
	    }
	}

	if (sl.has_left_sib) {
	    if (sl_parent.this_was_del) {
		update_node_parent(sl.left_sib, sl_parent.left_sib);
	    } else {
		update_node_parent(sl.left_sib, sl_parent.node);
	    }
	}

	if (sl.has_right_sib && !sl.right_was_del) {
	      if (sl_parent.this_was_del) {
		update_node_parent(sl.right_sib, sl_parent.left_sib);
	    } else {
		update_node_parent(sl.right_sib, sl_parent.node);
	    }
	}
    }
}

bool
equals(const DelInfo & sim, const DelInfo & given) {

    if (sim.this_was_del) {
	if (DEBUG_PROOF) {cerr << "equals:MERGE LEFT";}
	// there was a merge left
	return sim.has_left_sib &&
	    sim.left_sib.equals(given.node);
    }

    if (sim.right_was_del) {
	// there was a merge right
	if (DEBUG_PROOF) { cerr << "equals:MERGE RIGHT\n";}
	return sim.node.equals(given.node);
    }

    if (DEBUG_PROOF) { cerr << "equals:BASIC DELETE OR ROTATION\n";}
    // there were either rotations or basic deletes
    assert_s(sim.node.equals(given.node), "node mismatch");
    assert_s(sim.has_left_sib == given.has_left_sib &&
	     (!sim.has_left_sib || sim.left_sib.equals(given.left_sib)), "left sib mismatch");
    assert_s(sim.has_right_sib == sim.has_right_sib &&
	     (!sim.has_right_sib || sim.right_sib.equals(given.right_sib)), "right sib mismatch");
    return true;
}

static void
check_equals(const State & st1, const State & st2) {

    // start from the leaves because delete may remove root
    if (st1.size() != st2.size()) {
	assert_s(st1.size() == st2.size() + 1 && st1[0].this_was_del, "not equal states");
    }
    
    for (int i = st2.size()-1; i >= 0; i--) {
	if (!equals(st1[i],st2[i])) {
	    cerr << "difference in level " << i << "\n";
	    cerr << "one is\n" << st1[i] << "\nthe other\n" << st2[i] << "\n";
	    assert_s(false, "difference in level");
	}
    }
    
}

//TODO: make find root efficient

std::ostream&
operator<<(std::ostream &out, const DelInfo & di) {
    out << "{ DelInfo: node " << di.node << "\n"
	<< " \t\t has_left_sib " << di.has_left_sib
	<< " left_sib " << di.left_sib << "\n"
	<< " \t\t has_right_sib " << di.has_right_sib
	<< " right_sib " << di.right_sib
	<< " this del " << di.this_was_del
	<< " right del " << di.right_was_del
	<< "}";

    return out;
}
/*
static void
vector_insert_for_split(NodeInfo & node, ElInfo & ins_ei) {
    assert_s(false, "vector insert for split no implemented");
}
*/
/*
static void
split_insert(State & st, int index, ElInfo & ins_ei) {
    NodeInfo & node = st[index].node;

    int m_count = node.childr.size();
    
    assert_s(m_count != num_elms - 1, "split insert should not have been called");

    vector_insert_for_split(node, ins_ei);

    unsigned int split_point = m_count/2;

    if (2 * split_point < (uint) m_count) {
	split_point++;
    }

    NodeInfo new_node = NodeInfo();

    ElInfo upward_elm = node.childr[split_point];

    insert_zeroth_subtree(node, upward_elm.hash);

    new_node.key = index > 0 ? st[index-1].node.key : "";
    node.key = upward_elm.key;

    for (int i = 1; i < (int) (m_count - split_point); i++) {
	ins_elinfo(new_node.childr, node.childr[split_point+i]);
    }

    if (index > 0 && ins_elinfo(st[index-1].node.childr, upward_elm)) { // this node has a parent
	return;
    }
    else if (index > 0 && split_insert(st, index-1, upward_elm)) {
	return true;
    } 
    else if (index == 0) { need to move stuff around
	NodeInfo new_root = NodeInfo();
	node.key = "";
	DelInfo new_di = DelInfo();
	new_node.key = "";
	ins_elinfo(new_root.childr, upward_elm);
	di.node = new_root;
	st.insert(new_di, 0);
    }
   
    }*/
/*
static void
sim_insert(State & st, ElInfo & ins_ei, int index) {
    assert_s(false, "impl not finished");
    find(st, ins_ei.key, index);

    if (ins_elinfo(ins_target, st[index].node.childr)) {
	return; //done
    }

    split_insert(st, index, ins_ei);
}*/

/*  We check the change by rerunning the insertion
    on st_before. It turns out this is much simpler
    than trying to check if every operation the server
    did in the deletion was correct. */
bool UpdateMerkleProof::check_ins_change(std::string ins_target) const {

    assert_s(false, "check ins change not implemented fully");
    /*   State st_sim = st_before;
    
       // simulate insertion
    sim_insert(st_sim, ElInfo(ins_target, ""), 0);
    // keys and childr are up to date and
    // hashes of subtrees not involved in the delete;
    // other hashes in pos in parent may be stale

    // recompute all hashes
    recomp_hash_pos(st_sim);

    // compare new state to the one from the server
    check_equals(st_sim, st_after);
    */
    return true;
}


/*  We check the change by rerunning the deletion
    on st_before. It turns out this is much simpler
    than trying to check if every operation the server
    did in the deletion was correct. */
bool UpdateMerkleProof::check_del_change(std::string del_target) const {
   
    State st_sim = st_before;
    
    // simulate deletion
    sim_delete(st_sim, del_target, 0);
    // keys and childr are up to date and hashes of subtrees not involved in the
    // delete
    // other hashes in pos in parent may be stale

    // recompute all hashes
    recomp_hash_pos(st_sim);

    // compare new state to the one from the server
    check_equals(st_sim, st_after);
    
    return true;
}

string UpdateMerkleProof::new_hash() const {

    if (st_after.size() == 0) {
	return hash_empty_node;
    }
    DelInfo root = st_after[0];
    if (!root.this_was_del) {
	return root.node.hash();
    } else {
	assert_s(st_after.size() >= 2, "inconsistency");
	return st_after[1].node.hash();
    }
}


string
Node::hash_node(){
    string hashes_concat = string(m_count * Elem::repr_size, 0);

    for (uint i = 0 ; i < m_count ; i++) {
	Elem e = m_vector[i];
	hashes_concat.replace(i*Elem::repr_size, Elem::repr_size, e.repr());
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
    
    if (this != find_root()) {
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
 

bool Node::tree_insert(Elem& element, UpdateMerkleProof & p) {

    Node* last_visited_ptr = this;

    if (search(element, last_visited_ptr).valid()) { // element already in tree
        return false;
    }

    // ---- Merkle -----
    // last_visited_ptr is the start point for the proof
    record_state(last_visited_ptr, p.st_before);
    // -----------------
    
    // insert the element in last_visited_ptr if this node is not fulls
    if (last_visited_ptr->vector_insert(element)) {

        // -----Merkle ----
	// done making the changes for insert so update merkle hash and record
	// new state
	last_visited_ptr->update_merkle_upward();
	record_state(last_visited_ptr, p.st_after);
	// ----------------
	
        return true;
    }

    //last_visited_ptr node is full so we will need to split
    bool r = last_visited_ptr->split_insert(element);

    // -----Merkle ----
    // done making the changes for insert so update merkle hash and record
    // new state
    last_visited_ptr->update_merkle_upward();
    record_state(last_visited_ptr, p.st_after);
    // ----------------

    return r;
} 


void
record_state(Node * node, State & state) {

    bool is_root = false;
    if (node != node->find_root()) {
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

    if (node->key_count() > (int)Node::minimum_keys()) {	
	if (DEBUG_PROOF) { cerr << "basic delete enough elems; my key count " << node->key_count()
			  << " min " << Node::minimum_keys() << " \n"; }
	
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
	    
	    if (node==node->find_root() && node->is_leaf()) {
		//empty tree
		break;
	    }
	    
	    if (node==node->find_root() && !node->is_leaf()) { // sanity check
		throw "node should not be root in delete_element loop";
	    }
	    
	    Node* right = node->right_sibling(parent_index_this);
	    Node* left = node->left_sibling(parent_index_this);
	    
	    // is an extra element available from the right sibling (if any)         
	    if (right && right->key_count() > (int)right->minimum_keys()) {
		// node will be null after this function, because delete
		// terminates, so the function will finalize the proof
		if (DEBUG_PROOF) { cerr << "rotate right \n";}
		node->rotate_from_right(parent_index_this);
		break;
	    }
	    
	    else if (left && left->key_count() > (int)left->minimum_keys()) {
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

    if (mp_parent==find_root() && !mp_parent->key_count()) {

        m_root.set_root(m_root.get_root(), this);

        delete mp_parent;
        mp_parent = 0;

        return null_ptr;
    }
    else if (mp_parent==find_root() && mp_parent->key_count()) {

        return null_ptr;
    }
    if (mp_parent && (mp_parent->key_count() >= (int)mp_parent->minimum_keys())) {
        
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

      
    if (mp_parent==find_root() && !mp_parent->key_count()) {

        m_root.set_root(m_root.get_root(), left_sib);

        delete mp_parent;

        left_sib->mp_parent = null_ptr;
	
        delete this;

        return null_ptr;

    } 
    else if (mp_parent==find_root() && mp_parent->key_count()) {

        delete this;

	return null_ptr;

    }

    //mp_parent not root
    delete this;

    if (parent_node->key_count() >= (int)parent_node->minimum_keys()) {

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

    if (mp_parent->m_vector[first].mp_subtree == this) {

        return first;
    }

    else if (mp_parent->m_vector[last].mp_subtree == this) {

        return last;
    }
    else

        throw "error in index_has_subtree";

} 

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
    return format_concat(m_key, key_size, get_subtree_merkle(), repr_size);
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

