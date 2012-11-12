
/****************************
 *  Merkle data structures
 *  and related functions.
 ****************************/

#pragma once

#include <string>
#include <vector>
#include <list>
#include <util/util.hh>
#include <crypto/sha.hh>
#include <util/ope-util.hh>


using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::vector;
using std::cerr;
using std::max;


std::ostream &
marshall_binary(std::ostream & o, const string & hash);

std::string
unmarshall_binary(std::istream & o);

// Represents one element: its key and the hash of its subtree
struct ElInfo {
    std::string key;
    std::string hash;

    static const uint repr_size = sha256::hashsize + b_key_size;
    
    ElInfo(std::string key_, std::string hash_): key(key_), hash(hash_) {}
    ElInfo() {}

    bool operator==(const ElInfo & el) const;
    
    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &out);
    
    std::string pretty() const;
    
};

// Records information at a B tree node; key of node, children, etc.
struct NodeInfo {
    std::string key; // the key (in parent) whose subtree is this node
    int pos_in_parent;
    std::vector<ElInfo> childr;

    NodeInfo();
    
    std::string hash() const;
    uint key_count();

    bool operator==(const NodeInfo & ni) const;
    
    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);
    
    std::string pretty() const;

};

// Merkle proof for one node 
struct MerkleProof {
    std::list<NodeInfo> path;

    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);
    
    std::string pretty() const;

};

    
// Information needed about a delete or insert on a level
struct UpInfo  {
    NodeInfo node;
    
    bool has_left_sib;
    NodeInfo left_sib;

    bool has_right_sib;
    NodeInfo right_sib;

    // info added by client during verification for del
    bool this_was_del;
    bool right_was_del;

    //info added by client during verif for insert
    bool node_moved_right;
    bool left_moved_right;
    bool right_moved_right;
    bool path_moved_right;
    
    UpInfo() {
	has_left_sib = false;
	has_right_sib = false;
	this_was_del = false;
	right_was_del = false;
	node_moved_right = left_moved_right = right_moved_right = false;
	path_moved_right = false;
    }

    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);

    //returns true if ui has the same contents (other than contents marked as
    //added by client above)
    bool equals(UpInfo ui);
    
    std::string pretty() const;
    
};

// checks if two UpInfo are equivalent
bool
equiv(const UpInfo & sim, const UpInfo & given);

typedef std::vector<UpInfo> State;

struct PrettyPrinter {
    static std::string pretty(const State & st);
};

/*
 * Merkle Proofs for updates
 *
 * 1. Deletion Merkle Proof
 *
 * - delete changes at most the nodes on a path from a node (``deletion path'')
 *   up to the root and one additional node, sibling of one node on this path
 *
 * - there are a few transformations deletion may perform on the tree and we
 * - need to convey these in the proofs
 *
 * 2. Insertion Merkle Proof
 *
 * - Changes only the nodes on a path from a node up to root
 * - UpdateMerkleProof st_before and after start from root and go
 * down to the lowest level affected by insert
 *
 * UpdateMerkleProof:
 * ``before'' information in UpInfo is before delete/insert
 * ``after'' is after delete/insert
 */
struct UpdateMerkleProof {
    // vector is from root to leaf
    State st_before; 
    State st_after; 
    
    std::string old_hash() const;

    template<class V, class BC>
    bool check_del_change(std::string key_to_del, BC * bc = NULL) const;
    template<class V, class BC>
    bool check_ins_change(std::string key_to_ins, BC * bc = NULL) const;

    std::string new_hash() const;

    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);

    std::string pretty() const;
   
};

// Two special values of the hash,
// it is unlikely a real hash will take them
const std::string hash_empty_node = "";
const std::string hash_not_extracted = "1";


// formats (key, hash) pair for hashing
// arranges key in key_size bytes and hash in hash_size bytes
// and concatenates them
std::string
format_concat(const std::string & key, uint key_size, const std::string & hash, uint hash_size);


/************ Functions need to be declared in this file mostly because of templating ****/

/******************* Checking proofs **********/

template<class V, class BC>
V get_dec(string a, BC * bc) {
    assert_s(a != "", "cannot decrypt and compare to empty string");
    assert_s(bc, "null blockcipher!\n");
    return bc->decrypt(Cnv<V>::TypeFromStr(a));
}

// greater than
template<class V, class BC>
bool
gt(string a, string b, BC * bc) {
    if (OPE_MODE) {
	if (a == "" && b == "") {
	    return false;
	}  
	if (a == "") {
	    return false;
	} 
	if (b == "") {
	    return true;
	}
	
	V aa = get_dec<V,BC>(a, bc);
	V bb = get_dec<V,BC>(b, bc);

	return aa > bb;
    } else {
	return (a>b);
    }
}


// greater or equal than
template<class V, class BC>
bool
ge(string a, string b, BC * bc) {
    if (OPE_MODE) {

	if (b == "") {
	    return true;
	} 
	if (a == "") {
	    return false;
	}
	
	V aa = get_dec<V,BC>(a, bc);
	V bb = get_dec<V,BC>(b, bc);

	return aa>=bb;
    } else {
	return (a>=b);
    }
}

// equals
template<class V, class BC>
bool
eq(string a, string b, BC * bc) {
    if (OPE_MODE) {
	if (a == "" && b == "") {
	    return true;
	}
	if (a == "" || b == "") {
	    return false;
	}
	
	V aa = get_dec<V,BC>(a, bc);
	V bb = get_dec<V,BC>(b, bc);

	return aa==bb;
    } else {
	return (a==b);
    }
}

 

// Should return the position pos where v[pos] <= target < v[pos+1]
template<class V, class BC>
static ElInfo &
binary_search(vector<ElInfo> & v, string target, bool & matched, int & pos, BC * bc = NULL) {
    
    int first = 0;
    int last = v.size() -1;
    while (last - first > 1) {
	int mid = first + (last - first)/2;

	if (ge<V, BC>(target,v[mid].key,bc)) {
	    first = mid;
	} else {
	    last = mid;
	}
    }

    matched = false;
    
    if (eq<V,BC>(v[first].key,target, bc)) {
	matched = true;
	pos = first;
	return v[first];
    }

    if (eq<V,BC>(v[last].key,target, bc)) {
	matched = true;
	pos = last;
	return v[last];
    }

    if (gt<V,BC>(v[last].key, target, bc)) {
	pos = first;
	return v[first];
    } else {
	pos = last;
	return v[last];
    }

}

template<class V, class BC>
static void
update_node_parent(NodeInfo & node, NodeInfo & parent, BC * bc = NULL) {
    if (node.pos_in_parent >= 0 &&
	parent.childr.size() > (uint)node.pos_in_parent &&
	parent.childr[node.pos_in_parent].key == node.key) {
	// pos is correct
    } else {
	bool matched = false;
	binary_search<V, BC>(parent.childr, node.key, matched, node.pos_in_parent, bc);
	assert_s(matched, "could not find child in parent");
    }

    // now pos_in_parent is up to date
    parent.childr[node.pos_in_parent].hash = node.hash();
}



// recomputes hashes and positions
template<class V, class BC>
static void
recomp_hash_pos_del(State & st, BC * bc = NULL) {

    // compute hashes for nodes in a level and update the parent
    
    for (int i = st.size()-1; i > 0; i--) {
	UpInfo & sl = st[i];
	UpInfo & sl_parent = st[i-1];
      
	if (!sl.this_was_del) {
	    if (sl_parent.this_was_del) {
		update_node_parent<V, BC>(sl.node, sl_parent.left_sib, bc);
	    } else {
		update_node_parent<V, BC>(sl.node, sl_parent.node, bc);
	    }
	}

	if (sl.has_left_sib) {
	    if (sl_parent.this_was_del) {
		update_node_parent<V, BC>(sl.left_sib, sl_parent.left_sib, bc);
	    } else {
		update_node_parent<V, BC>(sl.left_sib, sl_parent.node, bc);
	    }
	}

	if (sl.has_right_sib && !sl.right_was_del) {
	      if (sl_parent.this_was_del) {
		  update_node_parent<V, BC>(sl.right_sib, sl_parent.left_sib, bc);
	    } else {
		  update_node_parent<V, BC>(sl.right_sib, sl_parent.node, bc);
	    }
	}
    }
}



// recomputes hashes and positions
template<class V, class BC>
static void
recomp_hash_pos_ins(State & st, BC * bc = NULL) {

    // compute hashes for nodes in a level and update the parent
    
    for (int i = st.size()-1; i > 0; i--) {
	UpInfo & sl = st[i];
	UpInfo & sl_parent = st[i-1];

	if (sl.node_moved_right) {
	    update_node_parent<V, BC>(sl.node, sl_parent.right_sib, bc);
	} else {
	    update_node_parent<V, BC>(sl.node, sl_parent.node, bc);
	}

	if (sl.has_left_sib) {
	    if (sl.left_moved_right) {
		update_node_parent<V, BC>(sl.left_sib, sl_parent.right_sib, bc);
	    } else {
		update_node_parent<V, BC>(sl.left_sib, sl_parent.node, bc);
	    }
	}
	
	if (sl.has_right_sib) {
	    if (sl.right_moved_right) {
		update_node_parent<V, BC>(sl.right_sib, sl_parent.right_sib, bc);
	    } else {
		update_node_parent<V, BC>(sl.right_sib, sl_parent.node, bc);
	    }
	}
    }
}


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



static void
check_equals_del(const State & st_sim, const State & st_real) {

    assert_s(st_sim.size() == st_real.size(), "real and simulated states differ in height");
    
    for (uint i = 0; i < st_real.size(); i++) {
	if (!equiv_del(st_sim[i],st_real[i])) {
	    cerr << "difference in level " << i  << "\n";
	    cerr << " sim is\n" << st_sim[i].pretty() << "\n real is\n" << st_real[i].pretty() << "\n";
	    assert_s(false, "difference in level");
	}	
    }
}



static void
check_equals_ins(const State & st_sim, const State & st_real) {

    assert_s(st_sim.size() == st_real.size(), "real and simulated states differ in height");
    
    for (uint i = 0; i < st_real.size(); i++) {
	UpInfo sim = st_sim[i];
	UpInfo given = st_real[i];
	
	if (sim.path_moved_right) {
	    assert_s(sim.right_sib  == given.node, "node mismatch");
	} else {
	    assert_s(sim.node == given.node, "node mismatch");
	}
    }
}


template<class V, class BC>
static ElInfo &
find(State & st, string target, int & index, BC * bc = NULL) {

    vector<ElInfo> & childr = st[index].node.childr;
    bool matched;
    int pos;
    ElInfo & ei = binary_search<V, BC>(childr, target, matched, pos, bc);
    if (matched) {
	return ei;
    }
    assert_s(index+1 <= (int)st.size(), "not found!");
    assert_s(st[index+1].node.key == ei.key, " deletion path is not the correct one");

    index = index + 1;
    return find<V, BC>(st, target, index, bc);  
}

static void
del_elinfo_pos(int pos, vector<ElInfo> & v) {
    for (int i = pos; i < (int)v.size()-1; i++) {
	v[i] = v[i+1];
    }
    v.resize(v.size() - 1);
}

template<class V, class BC>
static void
del_elinfo(string del_target, vector<ElInfo> & v, BC * bc = NULL) {
    bool matched;
    int pos;
    if (OPE_MODE) {
	assert_s(false, "del_elinfo still uses keys for comparison");
    }
    binary_search<V, BC>(v, del_target, matched, pos, bc);
    assert_s(matched, "del_elinfo received key to remove that does not exist in vector");
    del_elinfo_pos(pos, v);
}

template<class V, class BC>
static bool
ins_elinfo(ElInfo ei, vector<ElInfo> & v, BC * bc = NULL) {

    if ((int)v.size() >= (int)b_max_keys - 1) {
	return false;
    }

    bool matched = false;
    int pos;

    if (v.size() == 0) {
	pos = -1;
	matched = false;
    } else {
	binary_search<V, BC>(v, ei.key, matched, pos, bc);
    }
    
    assert_s(!matched, "ins_elinfo received key to insert that already exists in vector");
    v.resize(v.size()+1);
    
    // must insert on pos + 1
    for (int i = (int)v.size()-2; i> pos ; i-- ) {
	v[i+1] = v[i];
    }
    v[pos+1] = ei;
    
    return true;
}
template<class V, class BC>
static void
rotate_from_right(State & st, int index, BC * bc = NULL) {

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
    
    ins_elinfo<V, BC>(filler, node.childr, bc);
    del_elinfo<V, BC>(0, sib.childr, bc);
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

template<class V, class BC>
static void
rotate_from_left(State & st, int index, BC * bc = NULL) {
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
    
    ins_elinfo<V, BC>(filler, node.childr, bc);
    del_elinfo<V, BC>(sib.childr.size() - 1, sib.childr, bc);

    node.key = parent.childr[parent_index_this].key;
    // if node has child with key ""
    change_key(st, index+1, "", filler.key);
 
}

template<class V, class BC>
static bool
merge_right(State & st, int index, BC * bc = NULL) {
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
    ins_elinfo<V, BC>(parent_elem, node.childr, bc);
    
    for (uint i = 1; i < sib.childr.size(); i++) {
	ins_elinfo<V, BC>(sib.childr[i],node.childr, bc);
    }
    
    del_elinfo<V, BC>(parent_index_this+1, parent.childr, bc);
    
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

template <class V, class BC>
static bool
merge_left(State & st, int index, BC * bc = NULL) {
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
    ins_elinfo<V, BC>(parent_elem, sib.childr, bc);
    
    for (uint i = 1; i < node.childr.size(); i++) {
	ins_elinfo<V, BC>(node.childr[i], sib.childr, bc);
    }
    
    del_elinfo<V, BC>(parent_index_this, parent.childr, bc);
    
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

template <class V, class BC>
static void
sim_delete(State & st, string del_target, int index, BC * bc = NULL) {

    if (OPE_MODE) {
	assert_s(false, "sim_delete compares ciphertexts -- needs to be templatized");	
    }
    
    ElInfo &  found = find<V, BC>(st, del_target, index, bc);

  
    NodeInfo & node = st[index].node;
    
    if (index == (int)st.size()-1) { // leaf
	
	if ((uint)node.key_count() > b_min_keys) {
	    if (DEBUG_PROOF) { cerr << "basic del enough elems; my key count " << node.key_count()
			      << " min " << b_min_keys << "\n"; }
	    del_elinfo<V, BC>(del_target, node.childr, bc);
	    
	} else {
	    del_elinfo<V, BC>(del_target, node.childr, bc);

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
		    rotate_from_right<V, BC>(st, index, bc);
		    break;
		}

		if (sl.has_left_sib && sl.left_sib.key_count() > b_min_keys) {
		    rotate_from_left<V, BC>(st, index, bc);
		    break;
		}

		if (sl.has_right_sib) {
		    done = merge_right<V, BC>(st, index, bc);
		} else {
		    assert_s(sl.has_left_sib, "incorrect state tree, must have right or left sib");
		    done = merge_left<V, BC>(st, index, bc);
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
	sim_delete<V, BC>(st, sm_in_subtree.key, index+1, bc); 
    }
    
    if (DEBUG_PROOF) {cerr << "done sim delete\n";}

}



template<class V, class BC>
static void
vector_insert_for_split(NodeInfo & node, ElInfo & ins_ei, BC * bc = NULL) {

    int i = node.childr.size();
    node.childr.resize(node.childr.size() + 1);

    while ((i > 0) && (gt<V,BC>(node.childr[i-1].key, ins_ei.key, bc))) {
	node.childr[i] = node.childr[i-1];
	i--;
    }
        
    node.childr[i] = ins_ei;
    
}

// assumes node is newly created and contains nothing
static void
insert_zeroth_subtree(NodeInfo & node, string hash) {
    assert_s(node.childr.size() == 0, "expecting empty node");
    ElInfo ei = ElInfo("", hash);
    node.childr.push_back(ei);
}

static void
update_child(NodeInfo & ni, int split_point) {
    if (ni.pos_in_parent == split_point) {
	ni.key = "";
    }

    ni.pos_in_parent = ni.pos_in_parent - split_point;
}

template<class V, class BC>
static void
split_insert(State & st, int level, ElInfo & ins_ei, BC * bc = NULL) {
    UpInfo & ui = st[level];
    NodeInfo & node = st[level].node;
    
    int keys_at_node = node.childr.size();
    
    assert_s(keys_at_node == b_max_keys-1, "split insert should not have been called");

    vector_insert_for_split<V, BC>(node, ins_ei, bc);
    keys_at_node++;

    unsigned int split_point = keys_at_node/2;

    if (2 * split_point < (uint) keys_at_node) {
	split_point++;
    }

    NodeInfo new_node = NodeInfo();

    ElInfo upward_elm = node.childr[split_point];
 
    insert_zeroth_subtree(new_node, upward_elm.hash);
 
    upward_elm.hash = hash_not_extracted; 
    new_node.key = upward_elm.key;
    new_node.pos_in_parent = node.pos_in_parent+1;

    for (int i = 1; i < (int) (keys_at_node - split_point); i++) {
	ins_elinfo<V, BC>(node.childr[split_point+i], new_node.childr, bc);
    }

    node.childr.resize(split_point);

    ui.has_right_sib = 1;
    ui.right_sib = new_node;


    // these changes may affect nodes on level level+1
    if (level +1 <  (int)st.size()) {
	UpInfo & ui_child = st[level+1];

	if (ui_child.node.pos_in_parent >= (int)split_point) {
	    ui_child.node_moved_right = true;
	    update_child(ui_child.node, split_point);
	}

	if (ui_child.has_left_sib && ui_child.left_sib.pos_in_parent >= (int)split_point) {
	    ui_child.left_moved_right = true;
	    update_child(ui_child.left_sib, split_point);
	}
	if (ui_child.has_right_sib && ui_child.right_sib.pos_in_parent >= (int)split_point) {
	    ui_child.right_moved_right = true;
	    update_child(ui_child.right_sib, split_point);
	}

	// determine if main path moves to right node
	if (ui_child.node_moved_right || (ui_child.path_moved_right && ui_child.right_moved_right)) {
	    ui.path_moved_right = true;
	}
    }
    
    if (level > 0 && ins_elinfo<V,BC>(upward_elm, st[level-1].node.childr, bc)) { // this node has a parent
	return;
    }
    else if (level > 0) {
	split_insert<V, BC>(st, level-1, upward_elm, bc);
	return;
    } 
    else if (level == 0) { // this node was the root
	NodeInfo new_root = NodeInfo();
	insert_zeroth_subtree(new_root, hash_not_extracted);
	node.key = "";
	node.pos_in_parent = 0;
	new_node.pos_in_parent = 1;
	ins_elinfo<V,BC>(upward_elm, new_root.childr, bc);
	
	UpInfo new_di = UpInfo();
	new_di.node = new_root;
	st.insert(st.begin(), new_di);
    }
   
}

template<class V, class BC>
static void
sim_insert(State & st, ElInfo & ins_ei, BC * bc = NULL) {

    // use find first here?
    int level = st.size() - 1;
    
    if (ins_elinfo<V, BC>(ins_ei, st[level].node.childr, bc)) {
	return; //done
    }

    split_insert<V, BC>(st, level, ins_ei, bc);
}

/*  We check the change by rerunning the insertion
    on st_before. It turns out this is much simpler
    than trying to check if every operation the server
    did in the deletion was correct. */
template<class V, class BC>
bool UpdateMerkleProof::check_ins_change(std::string ins_target, BC * bc) const {

    State st_sim = st_before;

    ElInfo ei = ElInfo(ins_target, "");
    // simulate insertion
    sim_insert<V, BC>(st_sim, ei, bc);
    // keys and childr are up to date and
    // hashes of subtrees not involved in the delete;
    // other hashes in pos in parent may be stale
  
    // recompute all hashes - this makes all hashes fresh now
    recomp_hash_pos_ins<V, BC>(st_sim, bc);

    // compare new state to the one from the server
    check_equals_ins(st_sim, st_after);
    
    return true;
}


/*  We check the change by rerunning the deletion
    on st_before. It turns out this is much simpler
    than trying to check if every operation the server
    did in the deletion was correct. */
template<class V, class BC>
bool UpdateMerkleProof::check_del_change(std::string del_target, BC * bc) const {
   
    State st_sim = st_before;
    
    // simulate deletion
    sim_delete<V, BC>(st_sim, del_target, 0, bc);
    // keys and childr are up to date and
    // hashes of subtrees not involved in the delete
    // other hashes in pos in parent may be stale

    // recompute all hashes - this makes all hashes fresh now
    recomp_hash_pos_del<V, BC>(st_sim, bc);

    // compare new state to the one from the server
    check_equals_del(st_sim, st_after);
    
    return true;
}
