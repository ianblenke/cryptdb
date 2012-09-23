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
Node::num_elements() {

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


std::ostream&
operator<<(std::ostream &out, const DelMerkleProof & dmp){

    out << "== DelP: levels:\n";
    for (uint i = 0; i < dmp.levels.size(); i++) {
	out << "\t\t" << dmp.levels[i] << "\n\n";
    }
    out << "\t has_non_leaf " << dmp.has_non_leaf << " non_leaf " << dmp.non_leaf << "\n ===";
/*   out<<dmp.oldproof<<" "<<dmp.newproof<<" ";
    out<<dmp.new_tree_empty<< " "<<dmp.is_sib<<" "<<dmp.sib_was_deleted<<" "<<dmp.pos_of_sib_in_parent<<" ";
    out<<dmp.non_leaf<<" "<<dmp.index_in_path<<" ";
    out<<dmp.oldsib<<" | "<<dmp.newsib<<" | "<<dmp.non_leaf_old<<" | "<<dmp.non_leaf_new<<" |";
    */
    return out;

}

std::istream&
operator>>(std::istream &is, DelMerkleProof & dmp){
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

std::ostream&
operator<<(std::ostream &out, const InsMerkleProof & imp){
    /*  out<<imp.oldproof<<" "<<imp.newproof;*/
    return out;
}

std::istream&
operator>>(std::istream &is, InsMerkleProof & imp){
    /*   std::stringstream s;
    std::string a;   
    int state=0;
    while(true){
        is>>a;
        if(a=="endmp") {
            MerkleProof mp;
            s<<a;
            s>>mp;
            if(state==0){
                imp.oldproof=mp;
                //cout<<"setting oldproof"<<endl;
                state++;
                s.str("");
                s.clear();
            }else if(state==1){
                imp.newproof=mp;
                //cout<<"setting newproof "<<dmp.newproof<<endl;
                s.str("");  
                s.clear();          
                break;
            }
        }else{
            s<<a+" ";
            //cout<<a<<": "<<s.str()<<endl;
        }
	}*/
    return is;
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

bool
verify_ins_merkle_proof(const InsMerkleProof & p, const string & old_merkle_root, string & new_merkle_root) {
    //TODO
    
    return true;
}


bool
verify_del_merkle_proof(const DelMerkleProof & proof,
			string del_target,
			const string & old_merkle_root,
			string & new_merkle_root) {

    if (old_merkle_root != proof.old_hash()) {
	cerr << "merkle hash of old state does not verify \n";
	return false;
    }

    if (!proof.check_change(del_target)) {
	cerr << "the change information";
	return false;
    }

    new_merkle_root = proof.new_hash();
    
    return true;
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
set_hash(NodeInfo & child, NodeInfo & parent) {
    assert_s(child.pos_in_parent != -1, "incorrect child pos in parent");
    parent.childr[child.pos_in_parent].hash = child.hash();
}

string DelMerkleProof::old_hash() const {
    
    // first level
    auto it = levels.begin();
    assert_s(it != levels.end(), "levels in del proof empty");
    DelInfo di_child = *it;
    it++;
    
    for (; it != levels.end(); it++) {
	DelInfo di_parent = *it;

	set_hash(di_child.old_node, di_parent.old_node);
	if (di_child.is_sib) {
	    set_hash(di_child.old_sib, di_parent.old_node);
	}
	
	di_child = di_parent;
    }
    
    return di_child.old_node.hash();
}

static ostream & 
operator<<(ostream & out, const SimLevel & sl) {
    out << "[SL: node " << sl.node << "\n"
	 << "\t\t is_sib " << sl.is_sib
	 << " is_left_sib " << sl.is_left_sib
	<< " sib " << sl.sib << "\n"
	 << "\t\t sib_was_del " << sl.sib_was_deleted
	 << " this_was_del " << sl.this_was_deleted 
	 << "]";

    return out;
}

static ostream &
operator<<(ostream &out, const SimTree & st) {
    out << "{\n ";
    for (uint i =  0; i < st.size(); i++) {
	out << "\t" << st[i] << "\n"; 
    }
    out << "}";
    return out;
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
find(vector<SimLevel> & simtree, string target, int & index) {

    vector<ElInfo> & childr = simtree[index].node.childr;
    bool matched;
    int pos;
    ElInfo & ei = binary_search(childr, target, matched, pos);
    if (matched) {
	return ei;
    }
    assert_s(index+1 <= (int)simtree.size(), "not found!");
    assert_s(simtree[index+1].node.key == ei.key, " deletion path is not the correct one");

    index = index + 1;
    return find(simtree, target, index);  
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

static void
ins_elinfo(ElInfo ei, vector<ElInfo> & v) {
    v.resize(v.size()+1);
    bool matched;
    int pos;
    binary_search(v, ei.key, matched, pos);
    assert_s(!matched, "del_elinfo received key to remove that does not exist in vector");
    for (int i = (int)v.size()-2; i> pos ; i++ ) {
	v[i+1] = v[i];
    }
    v[pos+1] = ei;
}

static void
rotate_from_sib(vector<SimLevel> & simtree, int index) {
    SimLevel & sl = simtree[index];

    NodeInfo & node = sl.node;
    NodeInfo & sib = sl.sib;

    assert_s(index-1 >= 0, "lack of parent");
    NodeInfo & parent = simtree[index-1].node;

    uint parent_index_this = node.pos_in_parent; 
    if (sl.is_left_sib) {
	//rotate from left sib
	ElInfo  filler = parent.childr[node.pos_in_parent];
	filler.hash = node.childr[0].hash;
	
	node.childr[0].hash = sib.childr[sib.childr.size()-1].hash;
	
	parent.childr[parent_index_this].key = sib.childr[sib.childr.size()-1].key;

	ins_elinfo(filler, node.childr);
	del_elinfo(sib.childr.size() - 1, sib.childr);

    } else {
	ElInfo filler = parent.childr[parent_index_this + 1];
	filler.hash = sib.childr[0].hash;

	parent.childr[parent_index_this + 1].key = sib.childr[1].key;

	ins_elinfo(filler, node.childr);
	del_elinfo(0, sib.childr);
	sib.childr[0].key = "";
    }
}

static void
merge_sib(SimTree & simtree, int index) {
    SimLevel & sl = simtree[index];

    NodeInfo & node = sl.node;
    NodeInfo & sib = sl.sib;

    assert_s(index-1 >=0, "lack of parent");
    NodeInfo & parent = simtree[index-1].node;

    uint parent_index_this = node.pos_in_parent;

    if (sl.is_left_sib) {
	ElInfo parent_elem = parent.childr[parent_index_this];

	parent_elem.hash = node.childr[0].hash;
	ins_elinfo(parent_elem, sib.childr);

	for (uint i = 0; i < node.childr.size(); i++) {
	    ins_elinfo(node.childr[i], sib.childr);
	}

	del_elinfo(parent_index_this, parent.childr);

	if (index - 1 == 0 && !parent.childr.size()) {
	    simtree[index-1].this_was_deleted = true;
	}
	
	sl.this_was_deleted = true;
    } else {
	ElInfo parent_elem = parent.childr[parent_index_this+1];
	parent_elem.hash = sib.childr[0].hash;
	ins_elinfo(parent_elem, node.childr);

	for (uint i = 1; i < sib.childr.size(); i++) {
	    ins_elinfo(sib.childr[i],node.childr);
	}

	del_elinfo(parent_index_this+1, parent.childr);

	if (index - 1 == 0 && !parent.childr.size()) {
	    simtree[index-1].this_was_deleted = true;
	}
	
	sl.sib_was_deleted = true;
    }
}
static ElInfo &
smallest_in_subtree(SimTree & simtree, int index) {
   
    SimLevel & sl = simtree[index];
    assert_s(sl.node.childr.size() >= 2, "too few children"); // first is empty child
    ElInfo & el = sl.node.childr[1];
    
    if (index == (int)simtree.size()-1)  { // leaf
	assert_s(el.key != "" && el.hash == "", "incorrect deletion information");
	return el;
    }
    
    assert_s("" == simtree[index+1].node.key, "incorrect del path");
    return smallest_in_subtree(simtree, index+1);
}


// for gdb -- could not figure out how to call template funcs from
// gdb, so the below has repetitions
const char *
myprint(const DelMerkleProof & v) {
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
const char *
myprint(const SimTree & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
}
const char *
myprint(const SimLevel & v) {
    stringstream out;
    out << v;
    return out.str().c_str();
    }



static void
sim_delete(SimTree & simtree, string del_target, int index) {

    ElInfo &  found = find(simtree, del_target, index);

    SimLevel & sl = simtree[index];
    NodeInfo & node = sl.node;
    
    if (index == (int)simtree.size()-1) { // leaf
	
	if ((int)node.childr.size() > Node::minimum_keys()) {

	    del_elinfo(del_target, node.childr);
	    
	} else {
	    del_elinfo(del_target, node.childr);

	    while (true) {
		if (index == 0 && simtree.size() == 1) {
		    break;
		}

		if (index == 0 && index < (int)simtree.size() - 1) {
		    assert_s(false, "sanity check wrong");
		}

		// there need be at least one sibling
		assert_s(sl.is_sib, "node does not have sibling when it should");
		
		if (sl.sib.childr.size() > Node::minimum_keys()) {
		    rotate_from_sib(simtree, index);
		    break;
		}

		merge_sib(simtree, index);
		index++;
		sl = simtree[index];
	    }
	}
    } else {
	// non-leaf
	assert_s(index < (int)simtree.size()-1 &&
		 simtree[index+1].node.key == found.key, "incorrect path after nonleaf");
	ElInfo sm_in_subtree = smallest_in_subtree(simtree, index+1);
	
	found.key  = sm_in_subtree.key;
	sim_delete(simtree, sm_in_subtree.key, index+1); 
    }

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
recomp_hash(vector<SimLevel> & simtree) {

    for (int i = simtree.size()-1; i >= 0; i--) {
	SimLevel & sl = simtree[i];

	if (i > 0 && !simtree[i-1].this_was_deleted) {
	    //update hashes in parent
	    NodeInfo & parent = simtree[i-1].node;

	    if (!sl.this_was_deleted) {
		NodeInfo node = sl.node;
		parent.childr[node.pos_in_parent].hash = node.hash();
	    }
	    
	    if (sl.is_sib && !sl.sib_was_deleted) {
		NodeInfo sib = sl.sib;
		parent.childr[sib.pos_in_parent].hash = sib.hash();
	    }
	    
	}
    }
}

bool
SimLevel::equals(const SimLevel & sl) const {
    return node.equals(sl.node) &&
	is_sib == sl.is_sib &&
	is_left_sib == sl.is_left_sib &&
	(!is_sib || sib.equals(sl.sib)) &&
	sib_was_deleted == sl.sib_was_deleted &&
	this_was_deleted == sl.this_was_deleted;
}

static bool
equals(const SimTree & st1, const SimTree & st2) {
    if (st1.size() != st2.size()) {
	cerr << "different simtree sizes";
	return false;
    }

    for (int i = st1.size()-1; i >= 0; i--) {
	if (!st1[i].equals(st2[i])) {
	    cerr << "difference in level " << i << "\n";
	    cerr << "one is " << st1[i] << "\nthe other " << st2[i] << "\n";
	    return false;
	}
    }
    return true;
}
//TODO: find root inefficient

std::ostream&
operator<<(std::ostream &out, const DelInfo & di) {
    out << "{ DelInfo: deleted " << di.deleted << "\n"
	<< "\t\t old_node " << di.old_node << "\n"
	<< "\t\t new_node " << di.new_node << "\n"
	<< " \t\t is_sib " << di.is_sib
	<< " is_left_sib " << di.is_left_sib
	<< " sib_del " << di.sib_was_deleted
	<< " this del " << di.this_was_deleted
	<< "}";

    return out;
}


static SimTree
old_simtree(const DelMerkleProof & proof) {
    SimTree st;

    for (auto it = proof.levels.rbegin(); it != proof.levels.rend(); it++) {
	DelInfo di = *it;
	SimLevel sl;
	sl.node = di.old_node;
	sl.is_sib = di.is_sib;
	sl.is_left_sib = di.is_left_sib;
	sl.sib = di.old_sib;
	st.push_back(sl);
    }
    
    return st;
    
}

static SimTree
new_simtree(const DelMerkleProof & proof) {
    SimTree st;

    for (auto it = proof.levels.rbegin(); it != proof.levels.rend(); it++) {
	DelInfo di = *it;
	SimLevel sl;
	sl.node = di.new_node;
	sl.is_sib = di.is_sib;
	sl.is_left_sib = di.is_left_sib;
	sl.sib = di.new_sib;
	sl.sib_was_deleted = di.sib_was_deleted;
	sl.this_was_deleted = di.this_was_deleted;
	st.push_back(sl);
    }

    return st;
    
}

/*  We check the change by rerunning the deletion on a
    simulated tree. It turns out this is much simpler
    than trying to check if every operation the server
    did in the deletion was correct. */
bool DelMerkleProof::check_change(std::string del_target) const {
   
    //TODO: make sure simtree is a complete copy

    // simulate tree
    vector<SimLevel> simtree = old_simtree(*this);

    cerr << "old sim tree is " << myprint(simtree) << "\n";
    
    // simulate deletion
    sim_delete(simtree, del_target, 0);

    cerr << "simtree after sim del is " << simtree << "\n";
  
    // recompute all hashes
    recomp_hash(simtree);

    SimTree newsimtree = new_simtree(*this);
    cerr << "new simtree is " << newsimtree << "\n";
    // compare new state to the one from the server
    assert_s(equals(simtree, newsimtree), "incorrect deletion");
    
    return true;
}

string DelMerkleProof::new_hash() const {
    //TODO
    DelInfo root;
    if (levels[levels.size() - 1].this_was_deleted) {
	root = levels[levels.size() - 2];
    } else {
	root = levels[levels.size() - 1];
    }
    return root.new_node.hash();
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
	mp_parent->update_merkle_upward();
    }
}
 

bool Node::tree_insert(Elem& element, InsMerkleProof & p) {

    Node* last_visited_ptr = this;

    if (search(element, last_visited_ptr).valid()) { // element already in tree
        return false;
    }

    // insert the element in last_visited_ptr if this node is not fulls
    if (last_visited_ptr->vector_insert(element)) {
	last_visited_ptr->update_merkle_upward();
        return true;
    }

    //last_visited_ptr node is full so we will need to split
    return last_visited_ptr->split_insert(element);

} //__________________________________________________________________


// should be called on the last node which HAS the merkle updated
// it will work from its parent above
void
Node::finish_del(DelMerkleProof & proof) {
    if (this == find_root()) {
	//the proof is finished
	return;
    }

    NodeInfo old_parent = mp_parent->extract_NodeInfo();
    mp_parent->update_merkle();
    NodeInfo new_parent = mp_parent->extract_NodeInfo();
    
    proof.add(DelInfo(old_parent, new_parent));

    mp_parent->finish_del(proof);
}

bool
Node::tree_delete (Elem& target, DelMerkleProof & proof) {

// target is just a package for the key value.  the reference does not
// provide the address of the Elem instance to be deleted.

    // first find the node contain the Elem instance with the given key

    Node* node = 0;

    int parent_index_this = invalid_index;

    Elem& found = search(target, node);

    if (!found.valid()) {
        return false;
    }
     
    if (node->is_leaf()) {

	//------ Merkle -----
	DelInfo di(target.m_key, node->extract_NodeInfo());
	//-------------------
	
	if (node->key_count() > (int)node->minimum_keys()) {	

	    bool r = node->vector_delete(target);

	    //----- Merkle ------
	    node->update_merkle();
	    di.new_node = node->extract_NodeInfo();
	    proof.add(di);
	    node->finish_del(proof);
	    
	    
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

		    // ---- Merkle -------
		    node->update_merkle();
		    di.new_node = node->extract_NodeInfo();
		    proof.add(di);
		    // -------------------

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
		    node->rotate_from_right(parent_index_this, di, proof);
		    break;
		}
		else if (left && left->key_count() > (int)left->minimum_keys()) {
		    // check if an extra element is available from the left sibling (if any)
		    // node will be null after this function, because delete
		    // terminates, so the function will finalize the proof
		    node->rotate_from_left(parent_index_this, di, proof);
		    break;
		}
		else if (right) {
		    node = node->merge_right(parent_index_this, di, di_parent, proof);
		}
		else if (left) {
		    node = node->merge_left(parent_index_this, di, di_parent, proof);
		}

		// --- Merkle ---
		di = di_parent;
		// -------------
	    }

	}
    } else { // node is not leaf

	cerr << "not leaf\n";

	// --- Merkle -----
	proof.has_non_leaf = true;
	proof.non_leaf = DelInfo(target.m_key, node->extract_NodeInfo());
	// the new part of non-leaf will be established during tree_delete below
	// ----------------
	
	Elem& smallest_in_subtree = found.mp_subtree->smallest_key_in_subtree();

        found.m_key = smallest_in_subtree.m_key;

        found.m_payload = smallest_in_subtree.m_payload;
	
        found.mp_subtree->tree_delete(smallest_in_subtree, proof);

    }

    return true;

} 

void
Node::rotate_from_right(int parent_index_this, DelInfo & di,
			      DelMerkleProof & proof) {

    // new element to be added to this node
    Elem underflow_filler = (*mp_parent)[parent_index_this+1];

    // right sibling of this node

    Node* right_sib = (*mp_parent)[parent_index_this+1].mp_subtree;

    // ------ Merkle -----
    // DelInfo before rotating from sibling
    di.is_sib = true;
    di.is_left_sib = false;
    di.old_sib = right_sib->extract_NodeInfo();
    DelInfo di_parent;
    di_parent.old_node = mp_parent->extract_NodeInfo(parent_index_this);
    // -------------------
    
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

    // ---- Merkle -------
    //need to update this and its right sibling's merkle hash as well as all
    // the way up
    right_sib->update_merkle();
    this->update_merkle();
    mp_parent->update_merkle();
    
    // DelInfo after rotating from sibling
    di.new_node = this->extract_NodeInfo();
    di.new_sib = right_sib->extract_NodeInfo();
    di_parent.new_node = mp_parent->extract_NodeInfo();

    proof.add(di);
    proof.add(di_parent);
 
    mp_parent->finish_del(proof);
    //---------------------
 
}

 

void
Node::rotate_from_left(int parent_index_this, DelInfo & di,
		       DelMerkleProof & proof) {

    // new element to be added to this node

    Elem underflow_filler = (*mp_parent)[parent_index_this];

    // left sibling of this node

    Node* left_sib = (*mp_parent)[parent_index_this-1].mp_subtree;

    // ----- Merkle --------
    //DelInfo before rotating about sibling
    di.is_sib = true;
    di.is_left_sib = true;
    di.old_sib = left_sib->extract_NodeInfo();
    DelInfo di_parent;
    di_parent.old_node = mp_parent->extract_NodeInfo(parent_index_this);
    // ---------------------
    
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

    // ----- Merkle -------
    // need to update merkle hash of left sibling and this and upward to the
    // root
    left_sib->update_merkle();
    this->update_merkle();
    mp_parent->update_merkle();
    
    //DelInfo after rotating from sibling about this node and sibling
    di.new_node = this->extract_NodeInfo();
    di.new_sib = left_sib->extract_NodeInfo();
    di_parent.new_node = mp_parent->extract_NodeInfo();
    
    proof.add(di);
    proof.add(di_parent);
    mp_parent->finish_del(proof);
    //----------------------- 

} 

 

Node*
Node::merge_right (int parent_index_this, DelInfo & di, DelInfo & di_parent, DelMerkleProof & proof) {
// copy elements from the right sibling into this node, along with the
// element in the parent node vector that has the right sibling as it subtree.
// the right sibling and that parent element are then deleted


    Elem parent_elem = (*mp_parent)[parent_index_this+1];

    Node* right_sib = (*mp_parent)[parent_index_this+1].mp_subtree;

    //---- Merkle -------
    // DelInfo about sibling before merging
    di.is_sib = true;
    di.is_left_sib = false;
    di.old_sib = right_sib->extract_NodeInfo();
    di.sib_was_deleted = true;
    // DelInfo about parent before changing parent
    di_parent = DelInfo(parent_elem.m_key, mp_parent->extract_NodeInfo(parent_index_this));
    //------------------
    
    parent_elem.mp_subtree = (*right_sib)[0].mp_subtree;

    vector_insert (parent_elem);

    for (unsigned int i=1; i<right_sib->m_count; i++) {
        vector_insert ((*right_sib)[i]);
    }

    mp_parent->vector_delete(parent_index_this+1);

    delete right_sib;

    // --- Merkle ----
    this->update_merkle();
    // DelInfo after merging about this node 
    di.new_node = this->extract_NodeInfo();
    proof.add(di);
    // -------------

    if (mp_parent==find_root() && !mp_parent->key_count()) {

        m_root.set_root(m_root.get_root(), this);

        delete mp_parent;
        mp_parent = 0;

	// --- Merkle -----
	di_parent.this_was_deleted = true;
	proof.add(di_parent);
	// --------------

        return null_ptr;
    }
    else if (mp_parent==find_root() && mp_parent->key_count()) {
	//---- Merkle ------
	mp_parent->update_merkle();
	di_parent.new_node = mp_parent->extract_NodeInfo();
	proof.add(di_parent);
	mp_parent->finish_del(proof);
	// -----------------
        return null_ptr;
    }
    if (mp_parent && (mp_parent->key_count() >= (int)mp_parent->minimum_keys())) {
	// ----- Merkle ----
	mp_parent->update_merkle();
	di_parent.new_node = mp_parent->extract_NodeInfo();
	proof.add(di_parent);
	mp_parent->finish_del(proof);
	// -----------------
        
        return null_ptr; // no need for parent to import an element
    }

 
    return mp_parent; // parent must import an element

} //_______________________________________________________________________

//TODO: extract_nodeInfo reaches beyond current node which is bad design

Node* Node::merge_left (int parent_index_this, DelInfo & di, DelInfo & di_parent, DelMerkleProof & proof) {

// copy all elements from this node into the left sibling, along with the
// element in the parent node vector that has this node as its subtree.
// this node and its parent element are then deleted.

    Elem parent_elem = (*mp_parent)[parent_index_this];

    parent_elem.mp_subtree = (*this)[0].mp_subtree;

    Node* left_sib = (*mp_parent)[parent_index_this-1].mp_subtree;

    // --- Merkle ------
    // DelInfo about sibling before merging
    di.is_sib = true;
    di.is_left_sib = true;
    di.old_sib = left_sib->extract_NodeInfo();
    di.this_was_deleted = true;
    // DelInfo about parent before changing parent
    di_parent = DelInfo(parent_elem.m_key, mp_parent->extract_NodeInfo());
    // -----------------
    
    left_sib->vector_insert(parent_elem);

    for (unsigned int i=1; i<m_count; i++) {
        left_sib->vector_insert(m_vector[i]);
    }
    
    mp_parent->vector_delete(parent_index_this);

    Node* parent_node = mp_parent;  // copy before deleting this node

    // --- Merkle -----
    left_sib->update_merkle();
    di.new_sib = left_sib->extract_NodeInfo();
    proof.add(di);
    // ----------------
    
    if (mp_parent==find_root() && !mp_parent->key_count()) {

        m_root.set_root(m_root.get_root(), left_sib);

        delete mp_parent;

        left_sib->mp_parent = null_ptr;
	
        delete this;

	// --- Merkle ---
	di_parent.this_was_deleted = true;
	proof.add(di_parent);
	// --------------
	
        return null_ptr;

    } 
    else if (mp_parent==find_root() && mp_parent->key_count()) {

        delete this;

	// ----- Merkle -----
	mp_parent->update_merkle();
	di_parent.new_node = mp_parent->extract_NodeInfo();
	proof.add(di_parent);
	mp_parent->finish_del(proof);
	// ------------------
	
	return null_ptr;

    }

    //mp_parent not root
    delete this;

    if (parent_node->key_count() >= (int)parent_node->minimum_keys()) {

	// ----- Merkle -------
	parent_node->update_merkle();
	di_parent.new_node = mp_parent->extract_NodeInfo();
	proof.add(di_parent);
	mp_parent->finish_del(proof);
	// ----------------------
	
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

void
DelMerkleProof::add(DelInfo di) {
    /* Add treats a special case when we add DelInfo corresponding to non-leaf
    node; the old_node in di may then be not the oldest so it needs to be set
    to the oldest */
    
    if (has_non_leaf) {
	// check if to be added is non-leaf
	if (di.old_node.key == non_leaf.old_node.key) {
	    di.old_node = non_leaf.old_node;
	    non_leaf.new_node = di.new_node;
	}
	// check if to be added is child of non-leaf
	// horrible hack..
	if (di.old_node.key == non_leaf.new_node.childr[di.old_node.pos_in_parent].key) {
	    di.old_node.key = non_leaf.old_node.childr[di.old_node.pos_in_parent].key;
	}
    }

    levels.push_back(di);
}
