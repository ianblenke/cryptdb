#include <ope/merkle.hh>

#include <util/util.hh>
#include <util/ope-util.hh>
#include <crypto/sha.hh>

using namespace std;


static ostream &
marshall_binary(ostream & o, string & hash) {
    o << hash.size() << " ";
    o << hash;

    return o;
}

static string
unmarshall_binary(istream & o) {
    uint size;
    o >> size;

    char h[size];
    
    for (uint i = 0 ; i < size; i++) {
	char c;
	o >> c;
	h[i] = c;
    }
    return string(h, size);    
}

/**** ElInfo ******/

ostream &
ElInfo::operator>>(std::ostream &out) {
    out << key << " ";
    marshall_binary(out, hash);
    return out;
}

istream &
ElInfo::operator<<(std::istream & is) {
    is >> key;
    hash = unmarshall_binary(is);
    return is;
}

string
ElInfo::pretty() const {
    return "(" + key + ", " +  read_short(short_string(hash)) + ")"; 
}

bool
ElInfo::operator==(const ElInfo & e) const {
    return key == e.key && hash == e.hash;
}


/**** Node Info ****/

NodeInfo::NodeInfo() {
    pos_in_parent = -1;
    key = "";
    childr.clear();
}

uint
NodeInfo::key_count() {
    assert_s(childr.size() >= 1, "invalid childr size");
    return childr.size() - 1;
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

   
std::ostream&
NodeInfo::operator>>(std::ostream &out) {
    out << key << " " << pos_in_parent << " ";

    out << childr.size() << " ";

    for (auto ei : childr) {
	ei >> out;
	out << " ";
    }

    return out;
    
}

std::istream&
NodeInfo::operator<<(std::istream & is) {
    is >> key >> pos_in_parent;

    uint size;
    is >> size;

    childr = vector<ElInfo>(size);

    for (uint i = 0; i < size; i++) {
	childr[i] << is;
    }

    return is;
}

string
NodeInfo::pretty() const {
    stringstream ss;
    ss << "(NodeInfo: key " <<  key << " pos in parent " << pos_in_parent << " childr ";

    for (uint i = 0; i < childr.size(); i++) {
	ss << childr[i].pretty() +  " ";
    }
    ss << ")";

    return ss.str();
 
}

string
format_concat(const string & key, uint key_size, const string & hash, uint hash_size) {
    string repr = string(key_size + hash_size, 0);

    assert_s(key.size() <= key_size, "size of key is larger than max allowed");

    repr.replace(0, key.size(), key);
    
    assert_s(hash.size() <= hash_size, "size of hash is larger than sha256::hashsize");

    repr.replace(key_size, hash.size(), hash);

    return repr;
}



string
NodeInfo::hash() const {
    uint repr_size = ElInfo::repr_size;
    string concat = string(childr.size() * repr_size, 0);

    uint count = 0;

    for (auto p : childr) {
	assert_s(p.hash != hash_not_extracted, "child with not extracted hash in NodeInfo::hash");
	concat.replace(repr_size * (count++), repr_size,
		       format_concat(p.key, b_key_size, p.hash, sha256::hashsize));
    }

    return sha256::hash(concat);
}


bool
NodeInfo::operator==(const NodeInfo & node) const {
    return (key == node.key) &&
	(pos_in_parent == node.pos_in_parent) &&
        (childr == node.childr);
}


/****** Merkle proof ***************/

std::ostream&
MerkleProof::operator>>(std::ostream &out) {
    out << path.size();

    for (auto ni : path) {
	ni >> out;
	out << " ";
    }

    return out;
}

std::istream&
MerkleProof::operator<<(std::istream &is) {
    uint size;
    is >> size;

    path.clear();
    
    for (uint i = 0; i < size; i++) {
	NodeInfo ni;
	ni << is;
	path.push_back(ni);
    }

    return is;
}
    
std::string
MerkleProof::pretty() const {
    string res = " [MerkleProof: ";
    for (auto ni : path) {
	res += ni.pretty() + " ";
    }
    res += "]";
    return res;
}

/******************* Del Info **********/

// checks if the information in sim and given is
// equivalent, even if not equal
static bool
equiv_del(const UpInfo & sim, const UpInfo & given) {

    if (sim.this_was_del) {
	if (DEBUG_PROOF) {cerr << "equiv: MERGE LEFT";}
	// there was a merge left
	return sim.has_left_sib &&
	    sim.left_sib == given.node;
    }

    if (sim.right_was_del) {
	// there was a merge right
	if (DEBUG_PROOF) { cerr << "equiv: MERGE RIGHT\n";}
	return sim.node == given.node;
    }

    if (DEBUG_PROOF) { cerr << "equiv: BASIC DELETE OR ROTATION\n";}
    // there were either rotations or basic deletes
    assert_s(sim.node == given.node, "node mismatch");
    assert_s(sim.has_left_sib == given.has_left_sib &&
	     (!sim.has_left_sib || sim.left_sib == given.left_sib), "left sib mismatch");
    assert_s(sim.has_right_sib == sim.has_right_sib &&
	     (!sim.has_right_sib || sim.right_sib == given.right_sib), "right sib mismatch");
    return true;
}


// checks if the information in sim and given is
// equivalent, considering an insert has happened
static bool
equiv_ins(const UpInfo & sim, const UpInfo & given) {

    assert_s(sim.node == given.node, "node mismatch");
    assert_s(sim.has_left_sib == given.has_left_sib &&
	     (!sim.has_left_sib || sim.left_sib == given.left_sib), "left sib mismatch");
    assert_s(sim.has_right_sib == sim.has_right_sib &&
	     (!sim.has_right_sib || sim.right_sib == given.right_sib), "right sib mismatch");
    return true;
}

std::ostream&
UpInfo::operator>>(std::ostream &out) {
    out << has_left_sib << " ";
    left_sib >> out;
    out << " " << has_right_sib << " ";
    right_sib >> out;

    out << " " << this_was_del << " " << right_was_del << "\n";

    return out;
}

std::istream&
UpInfo::operator<<(std::istream &is) {
    is >> has_left_sib;
    left_sib << is;
    is >> has_right_sib;
    right_sib << is;

    is >> this_was_del >> right_was_del;

    return is;
}

std::string
UpInfo::pretty() const {
    stringstream ss;
    ss << "{ UpInfo: node " << node.pretty() << "\n"
       << " \t\t has_left_sib " << has_left_sib
       << " left_sib " << left_sib.pretty() << "\n"
       << " \t\t has_right_sib " << has_right_sib
       << " right_sib " << right_sib.pretty()
       << " this del " << this_was_del
       << " right del " << right_was_del
       << "}";

    return ss.str();
}



/***** State  ********/

static ostream&
operator<<(ostream & out, const State & st) {
    out << st.size() << " ";
    for (auto di : st) {
	di >> out;
	out << " ";
    }
    return out;
}

static istream&
operator>>(istream & is, State & st) {
    uint size;
    is >> size;
    st.clear();
    
    for (uint i = 0; i < size; i++) {
	UpInfo di;
	di << is;
	st.push_back(di);
    }

    return is;
}

std::string
PrettyPrinter::pretty(const State & st) {
    string res = " (State: ";
    for (auto di : st) {
	res += di.pretty() + " ";
    }
    res += ")";
    return res;
}


/***** Update Merkle Proof *********/

 
static void
hash_match(NodeInfo child, NodeInfo parent) {
    assert_s(child.hash() == parent.childr[child.pos_in_parent].hash, "hash mismatch parent and child");
}
static void
hash_match(const UpInfo & child, const UpInfo &  parent) {
    
    hash_match(child.node, parent.node);
    if (child.has_left_sib) {
	hash_match(child.left_sib, parent.node);
    }
    if (child.has_right_sib) {
	hash_match(child.right_sib, parent.node);
    }
    
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


static void
check_equals(bool is_ins, const State & st_sim, const State & st_real) {

    assert_s(st_sim.size() == st_real.size(), "real and simulated states differ in height");
    
    for (uint i = 0; i < st_real.size(); i++) {
	if (is_ins) {
	    if (!equiv_ins(st_sim[i],st_real[i])) {
		cerr << "difference in level " << i  << "\n";
		cerr << " sim is\n" << st_sim[i].pretty() << "\n real is\n" << st_real[i].pretty() << "\n";
		assert_s(false, "difference in level");
	    }
	} else {
	    if (!equiv_del(st_sim[i],st_real[i])) {
		cerr << "difference in level " << i  << "\n";
		cerr << " sim is\n" << st_sim[i].pretty() << "\n real is\n" << st_real[i].pretty() << "\n";
		assert_s(false, "difference in level");
	    }
	}
    }
}


// recomputes hashes and positions
static void
recomp_hash_pos(State & st) {

    // compute hashes for nodes in a level and update the parent
    
    for (int i = st.size()-1; i > 0; i--) {
	UpInfo & sl = st[i];
	UpInfo & sl_parent = st[i-1];
      
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

string
UpdateMerkleProof::old_hash() const {
    
    // first level
    auto it = st_before.begin();
    assert_s(it != st_before.end(), "levels in del proof empty");
    UpInfo di_parent = *it;
    it++;
    
    for (; it != st_before.end(); it++) {
	UpInfo di_child = *it;

	hash_match(di_child, di_parent);
	
	di_parent = di_child;
    }
    
    return st_before[0].node.hash();
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

    if ((int)v.size() >= (int)b_max_keys - 1) {
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
    UpInfo & sl = st[index];

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
	UpInfo & sl = st[index];
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
    UpInfo & sl = st[index];

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
    UpInfo & sl = st[index];
    UpInfo & sl_parent = st[index-1];
    
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
	cerr << "delete root\n";
	node.pos_in_parent = -1;
	node.key = "";
	st.erase(st.begin());
	return true;
    }
    if (index - 1 == 0 && parent.key_count()) {
	return true;
    }

    if (parent.key_count() >= b_min_keys) {
	return true;
    }
    return false;
}


static bool
merge_left(State & st, int index) {
    if (DEBUG_PROOF) { cerr << "merge left\n";}
    UpInfo & sl = st[index];
    UpInfo & sl_parent = st[index-1];

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
	cerr << "delete root\n";
	sib.pos_in_parent = -1;
	sib.key = "";
	st.erase(st.begin());
	return true;
    }
    if (index - 1 == 0 && parent.key_count()) {
	return true;
    }

    if (parent.key_count() >= b_min_keys) {
	return true;
    }
  
    return false;
}


static ElInfo &
smallest_in_subtree(State & st, int index) {
   
    UpInfo & sl = st[index];
    assert_s(sl.node.childr.size() >= 2, "too few children"); // first is empty child
    ElInfo & el = sl.node.childr[1];
    
    if (index == (int)st.size()-1)  { // leaf
	assert_s(el.key != "" && el.hash == hash_empty_node, "incorrect deletion information");
	return el;
    }
    
    assert_s("" == st[index+1].node.key, "incorrect del path");
    return smallest_in_subtree(st, index+1);
}


static void
sim_delete(State & st, string del_target, int index) {

    ElInfo &  found = find(st, del_target, index);

  
    NodeInfo & node = st[index].node;
    
    if (index == (int)st.size()-1) { // leaf
	
	if ((int)node.key_count() > b_min_keys) {
	    if (DEBUG_PROOF) { cerr << "basic del enough elems; my key count " << node.key_count()
			      << " min " << b_min_keys << "\n"; }
	    del_elinfo(del_target, node.childr);
	    
	} else {
	    del_elinfo(del_target, node.childr);

	    bool done = false;

	    while (!done) {
		UpInfo & sl = st[index];

		if (index == 0 && st.size() == 1) {
		    cerr << "delete root\n";
		    st.erase(st.begin());
		    break;
		}

		if (index == 0 && st.size() > 1) { 
		    assert_s(false, "should not get at root in this loop");
		}

		if (sl.has_right_sib && sl.right_sib.key_count() > b_min_keys) {
		    rotate_from_right(st, index);
		    break;
		}

		if (sl.has_left_sib && sl.left_sib.key_count() > b_min_keys) {
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




static void
vector_insert_for_split(NodeInfo & node, ElInfo & ins_ei) {
    assert_s(false, "vector insert for split no implemented");
}

static void
insert_zeroth_subtree(NodeInfo & node, string hash) {
    assert_s(node.childr.size() == 0, "expecting empty node");
    ElInfo ei = ElInfo("", hash);
    node.childr.push_back(ei);
}

static void
split_insert(State & st, int index, ElInfo & ins_ei) {
    NodeInfo & node = st[index].node;

    int m_count = node.childr.size();
    
    assert_s(m_count != b_max_keys-1, "split insert should not have been called");

    vector_insert_for_split(node, ins_ei);

    unsigned int split_point = m_count/2;

    if (2 * split_point < (uint) m_count) {
	split_point++;
    }

    NodeInfo new_node = NodeInfo();

    ElInfo upward_elm = node.childr[split_point];

    insert_zeroth_subtree(new_node, upward_elm.hash);

    new_node.key = index > 0 ? st[index-1].node.key : "";
    node.key = upward_elm.key;

    for (int i = 1; i < (int) (m_count - split_point); i++) {
	ins_elinfo(node.childr[split_point+i], new_node.childr);
    }

    if (index > 0 && ins_elinfo(upward_elm, st[index-1].node.childr)) { // this node has a parent
	return;
    }
    else if (index > 0) {
	split_insert(st, index-1, upward_elm);
	return;
    } 
    else if (index == 0) { // this node was the root
	NodeInfo new_root = NodeInfo();
	node.key = "";
	UpInfo new_di = UpInfo();
	new_node.key = "";
	ins_elinfo(upward_elm, new_root.childr);
	new_di.node = new_root;
	st.insert(st.begin(), new_di);
    }
   
}

static void
sim_insert(State & st, ElInfo & ins_ei) {
    assert_s(false, "impl not finished");
    //find(st, ins_ei.key, index);
    int index = st.size() - 1;
    
    if (ins_elinfo(ins_ei, st[index].node.childr)) {
	return; //done
    }

    split_insert(st, index, ins_ei);
}

/*  We check the change by rerunning the insertion
    on st_before. It turns out this is much simpler
    than trying to check if every operation the server
    did in the deletion was correct. */
bool UpdateMerkleProof::check_ins_change(std::string ins_target) const {

    assert_s(false, "check ins change not implemented fully");
    State st_sim = st_before;

    ElInfo ei = ElInfo(ins_target, "");
    // simulate insertion
    sim_insert(st_sim, ei);
    // keys and childr are up to date and
    // hashes of subtrees not involved in the delete;
    // other hashes in pos in parent may be stale

    // recompute all hashes
    recomp_hash_pos(st_sim);

    // compare new state to the one from the server
    check_equals(true, st_sim, st_after);
    
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
    check_equals(false, st_sim, st_after);
    
    return true;
}

string UpdateMerkleProof::new_hash() const {

    if (st_after.size() == 0) {
	return hash_empty_node;
    }
    UpInfo root = st_after[0];
    if (!root.this_was_del) {
	return root.node.hash();
    } else {
	assert_s(st_after.size() >= 2, "inconsistency");
	return st_after[1].node.hash();
    }
}

ostream&
UpdateMerkleProof::operator>>(std::ostream &out){
    out << st_before << " " << st_after;
    return out;
}

istream&
UpdateMerkleProof::operator<<(std::istream &is) {
    is >> st_before >> st_after;
    return is;
}

string
UpdateMerkleProof::pretty() const {
    string res = "Update Proof: \n -- old " + PrettyPrinter::pretty(st_before) + "\n -- new  " + PrettyPrinter::pretty(st_after);
    return res;
}
  
