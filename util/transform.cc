#include <util/transform.hh>

#include <util/util.hh>
#include <util/ope-util.hh>

using namespace std;

OPEiTransform::OPEiTransform() {
    new_root = false;
}

OPEiTransform::~OPEiTransform() {
    ts.clear();
}
void
OPEiTransform::push_change(OPEType ope_path, int num, int index_inserted, int split_point) {

    itransf t;
    t.ope_path = path_to_vec(ope_path, num);
    if (DEBUG_TRANSF) {
	cerr << "push change: ope_path " << ope_path << ": " << pretty_path(t.ope_path) << "  "
	     << " num " << num << " index_inserted " << index_inserted << " split_point " << split_point << "\n";
    }
    t.index_inserted = index_inserted;
    if (ts.size()) {
	itransf last_t = ts.back();
	assert_s(index_inserted == (int)(1 + last_t.ope_path[last_t.ope_path.size() - 1]), " inconsistent insert");
    }
    t.split_point = split_point;

    ts.push_back(t);
}

void
OPEiTransform::add_root() {
    assert_s(ts.back().split_point >= 0, "cannot add root if old root was not split");
    new_root = true;

    // all transformations get an index of zero, as if the new root already
    // existed
    for (auto t = ts.begin(); t != ts.end(); t++) {
	t->ope_path.insert(t->ope_path.begin(), 0);
    }

    // add a new transformation for the new root
    push_change(0, 0, 1, -1);
}

void
OPEiTransform::get_interval(OPEType & omin, OPEType & omax) {
    /*
     * Interval depends on the highest node affected in which there was just an
     * insert and no split.
     */

    if (new_root) {
	//everything
	omin = 0;
	omax = compute_ope({}, 0);
	return;
    }

    assert_s(ts.size() > 0, "there is no transformation");
    itransf t = ts.back();
    assert_s(t.split_point < 0, "last transformation should have no split point");

    OPEType common_path = vec_to_path(t.ope_path);
    OPEType common_bits = t.ope_path.size() * num_bits;
    
    omax = compute_ope(common_path, common_bits);

    /* Computing omin is less straightforward: */
    
    auto ts2 = ts;
    ts2.reverse();
    OPEType v = common_path;
    uint nbits = common_bits;

    // loop starting from highest node
    for (auto t : ts2) {
	if (t.split_point == -1) {
	    v  = (v << num_bits) + t.index_inserted - 1;
	    nbits = nbits + num_bits;
	    continue;
	}
	if (t.index_inserted -1 <= t.split_point) {
	    v = (v << num_bits) + t.index_inserted -1;
	    nbits = nbits+ num_bits;
	    continue;
	}
	// remains the case: t.index-1 > t.split_point
	v = (v << num_bits) + t.split_point;
	nbits = nbits + num_bits;
	break;
    }

    // ope representations have index one less than B tree index
    assert_s(nbits > 0, "logic error with omin");
   
    omin = compute_ope(v-1, nbits);
	
}

OPEType
OPEiTransform::transform(OPEType val) {

    if (DEBUG_TRANSF) std::cerr << "val " << val << " \n";
  
    // repr consists of ope path followed by index
    vector<uint> repr = enc_to_vec(val);
    // ope encodings have the last index one less than the one in the btree
    repr[repr.size()-1]++;

    if (new_root) {
	// we can think of nodes in the old tree as having index 0 in the new
	// root
	repr.insert(repr.begin(), 0);
    }

    // now repr is the path in B tree
    
     if (DEBUG_TRANSF) cerr << "its path " << pretty_path(repr) << "\n";
    
    bool met_transformation = false;

    // loop thru each transf
    for (auto t :  ts) {
	if (match(t.ope_path, repr)) { // need to transform
	    if (DEBUG_TRANSF) cerr << "match with ope path " << pretty_path(t.ope_path) << "\n";
	    uint & cur_index = repr[t.ope_path.size()];
	    if (!met_transformation && ((uint)t.index_inserted <= cur_index)) {
		cur_index++;
		if (DEBUG_TRANSF) {cerr << "first transf met, index is " << cur_index << "\n";}
				
	    }
	    met_transformation = true;
	    if (t.split_point == (int)cur_index) {
		if (DEBUG_TRANSF) cerr << "right at split point \n";
		if (repr.size() == t.ope_path.size()+1) {
		    //remove the last element
		    repr.resize(repr.size()-1);
		} else {
		    cur_index = 0;
		}
		repr[t.ope_path.size()-1]++;
		if (DEBUG_TRANSF) cerr << "path is now " << pretty_path(repr) << "\n";
		continue;
	    }
	    if (t.split_point >=0 && t.split_point < (int)cur_index) {
		if (DEBUG_TRANSF) cerr << "split point before " << t.split_point << "\n";
		cur_index = cur_index - t.split_point;
		assert_s(t.ope_path.size() > 0, "split point at root\n");
		repr[t.ope_path.size()-1]++;
		if (DEBUG_TRANSF) cerr << "becomes " << pretty_path(repr) << "\n";
	    }

	}
    }

    // ope encodings have the last index one less than the one in the btree
    assert_s(repr.size() > 0 && repr[repr.size() -1 ] > 0, "index must be > 0");
    repr[repr.size()-1]--;
    return vec_to_enc(repr);
}


static ostream&
operator>>(itransf t, ostream & out) {
    out <<  " " << t.ope_path.size() << " ";
    for (uint i = 0; i < t.ope_path.size(); i++) {
	out << t.ope_path[i] << " ";
    }

    out << t.index_inserted << " " << t.split_point << " ";

    return out;
}

static istream&
operator<<(itransf & t, istream & is) {
    uint size;
    is >> size;
    t.ope_path.resize(size);

    for (uint i = 0; i < size; i++) {
	is >> t.ope_path[i];
    }

    is >> t.index_inserted >> t.split_point;
  
    return is;
}

ostream &
OPEiTransform::operator>>(ostream &out) {
    out << " " << ts.size();

    for (auto t: ts) {
	t >> out;
	out << " ";
    }

    out << new_root << " ";

    return out;
}

istream &
OPEiTransform::operator<<(istream & is) {
    from_stream(is);
    return is;
}

void
OPEiTransform::from_stream(istream & is){
    uint size;
    is >> size;

    for (uint i = 0; i < size; i++) {
	itransf t;
	t << is;
	ts.push_back(t);
    }

    is >> new_root;

}

static bool
tr_equals(itransf t1, itransf t2) {
    if (t1.index_inserted != t2.index_inserted) {
	return false;
    }
    
    if (t1.split_point != t2.split_point) {
	return false;
    }

    if (t1.ope_path.size() != t2.ope_path.size()) {
	return false;
    }

    for (uint i = 0; i < t1.ope_path.size(); i++) {
	if (t1.ope_path[i] != t2.ope_path[i]) {
	    return false;
	}
    }

    return true; 
 }

bool
OPEiTransform::equals(OPEiTransform t) {
    if (new_root != t.new_root) {
	return false;
    }

    if (ts.size() != t.ts.size()) {
	return false;
    }

    auto it1 = ts.begin();
    auto it2 = t.ts.begin();

    for (; it1 != ts.end(); it1++) {
	if (!tr_equals(*it1, *it2)) {
	    return false;
	}
	it2++;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////

OPEdTransform::OPEdTransform() {
    ts.clear();
}

OPEdTransform::~OPEdTransform() {
    ts.clear();
}
void
OPEdTransform::push_change(OPEType ope_path, int num, int parent_index, int left_mcount, TransType optype) {

    dtransf t;
    t.ope_path = path_to_vec(ope_path, num);
    if (DEBUG_TRANSF) {
	cerr << "push change: ope_path " << ope_path << ": " << pretty_path(t.ope_path) << "  "
	     << " num " << num << " parent_index " << parent_index << " left_mcount " << left_mcount << "\n";
    }
    t.parent_index = parent_index;
    t.left_mcount = left_mcount;
    t.optype = optype;
/*    if (ts.size()) {
	dtransf last_t = ts.back();
	assert_s(last_t.optype != TransType::SIMPLE &&
		 last_t.optype != TransType::ROTATE_FROM_RIGHT &&
		 last_t.optype != TransType::ROTATE_FROM_LEFT,
		 "pushing a transformation after a termina transformation!");

    }
*/
    ts.push_back(t);
}


//Assume's t.parent_index is the index in the BTree not the OPE tree (BTree index is 1 larger)
void
OPEdTransform::get_interval(OPEType & omin, OPEType & omax) {
    /*
     * See OPEiTransform::get_interval for example
     * 
     */

    //TODO
    omin = -1;
    omax = 0;

    for (auto t: ts) {
	OPEType omin_tmp, omax_tmp;
	OPEType common_path = vec_to_path(t.ope_path);
	OPEType common_bits = t.ope_path.size() * num_bits;

	switch (t.optype) {
	case TransType::SIMPLE: {
		OPEType min_path = (common_path << num_bits) | (t.parent_index - 1); // -1 b/c OPE index at leaf is 1 less than BTree leaf index
		omin = compute_ope(min_path, common_bits + num_bits);
		omax = compute_ope(common_path, common_bits);
		break;
	}
	case TransType::ROTATE_FROM_RIGHT: {
		//left sibling has left_mcount - 1 real keys
		OPEType parent_elem_path = (common_path << num_bits) | (t.parent_index - 1);
		omin_tmp = compute_ope(parent_elem_path, common_bits + num_bits);
		//This is ope enc for elem next to the elem at parent_index
		OPEType parent_next_elem_path = (common_path << num_bits) | t.parent_index;
		omax_tmp = compute_ope(parent_next_elem_path, common_bits + num_bits);
		if (omin_tmp < omin) {
			omin=omin_tmp;
			cout <<"Hmm why is omin_tmp<omin in ROTATE RIGHT?" << endl;
		}
		if (omax_tmp > omax) omax=omax_tmp;	
		break;
	}
	case TransType::ROTATE_FROM_LEFT: {
		OPEType left_max_elem_path = (common_path << num_bits) | (t.parent_index - 1);
		left_max_elem_path = (left_max_elem_path << num_bits) | (t.left_mcount - 1);
		omin_tmp = compute_ope(left_max_elem_path, common_bits + 2*num_bits);
		//This is ope enc for elem next to the elem at parent_index
		OPEType parent_next_elem_path = (common_path << num_bits) | t.parent_index;
		omax_tmp = compute_ope(parent_next_elem_path, common_bits + num_bits);
		if (omin_tmp < omin) omin=omin_tmp;
		if (omax_tmp > omax) {
			omax=omax_tmp;
			cout << "Hmm why is omax_tmp > omax in ROTATE LEFT?" << endl;
		}
		break;
	}
	case TransType::MERGE_WITH_RIGHT:
	case TransType::MERGE_WITH_LEFT: {
		OPEType parent_del_path = (common_path << num_bits) | (t.parent_index - 1);
		omin_tmp = compute_ope(parent_del_path, common_bits + num_bits);
		omax_tmp = compute_ope(common_path, common_bits); 
		if (omin_tmp < omin) omin=omin_tmp;
		if (omax_tmp > omax) omax=omax_tmp;
		break;
	
	}
	// I assume NONLEAF type has ope_path to non-leaf node, and parent_index is index of elem deleted
	case TransType::NONLEAF: {
		OPEType nonleaf_elem_path = (common_path << num_bits) | (t.parent_index - 1);
		omin_tmp = compute_ope(nonleaf_elem_path, common_bits + num_bits);
		omax_tmp = omin_tmp;
		if (omin_tmp < omin) omin=omin_tmp;
		if (omax_tmp > omax) omax=omax_tmp;
		break;
	}

	}//end of switch
	
    }//end of for loop


	
}


static void
handle_rotate_from_right(dtransf t, vector<uint> & repr) {
    uint tsize = t.ope_path.size();
    uint & cur_index = repr[tsize];
    
    // Case: key is in parent
    if (repr.size() - 1 == tsize) { // key is in parent
	if ((int)cur_index == t.parent_index) {
	    cur_index--;
	    repr.push_back(t.left_mcount + 1);
	}
	return;
    }

    // Case: key is lower than parent

    uint & child_index = repr[tsize + 1];

    // key is in right subtree
    if ((int)cur_index == t.parent_index) {

        // Case: it is a key

	if (repr.size() == tsize + 2) {
	    if (child_index == 1) {// it is key which gets moved in parent
		repr.resize(tsize+1);
	    } else {
		child_index--;
	    }
	    return;
	}

	// Case: it is in a subtree
	
	if (child_index == 0) {//it is in zeroth subtree
	    child_index = t.left_mcount+1;
	    cur_index--;
	    return;
	} else {
	    child_index--;
	}
	return;
    }
    
    // Case: key is in left subtree: nothing changes
    
}

/* Merging from right node into this node */
static void
handle_merge(dtransf t, vector<uint> & repr) {
    uint tsize = t.ope_path.size();

    uint & cur_index = repr[tsize];

    if ((int)cur_index < t.parent_index) {
	//nothing happens
	return;
    }

    if ((int)cur_index > t.parent_index) {
	cur_index--;
	return;
    }

    // Now cur_index == t.parent_index

    if (tsize == repr.size() - 1) {
	// this is the parent elem that moves to left sib
	cur_index--;
	repr.push_back(t.left_mcount+1);
    } else {
	// in the right subtree
	cur_index--;
	uint & child_index = repr[tsize + 1];
	child_index = child_index + 1 + t.left_mcount;
    }
}



static void
handle_rotate_from_left(dtransf t, vector<uint> & repr) {
    uint tsize = t.ope_path.size();
    uint & cur_index = repr[tsize]; // index in parent node

    // Case: key in parent
    if (repr.size() - 1 == tsize) {
	if ((int)cur_index == t.parent_index) {
	    repr.push_back(1);
	}
	return;
    }


    // Case: key in children or their subtrees

    uint & child_index = repr[tsize + 1];

    if ((int)cur_index == t.parent_index - 1) { // in left subtree
	if ((int)child_index == t.left_mcount) { // only last key or subtree change
	    if (repr.size() - 2  == tsize) {//key is in left sibling
		// move to parent
		cur_index++;
		repr.resize(tsize + 1);
	    } else { // key is in some left subtree
		cur_index++;
		child_index = 0;
	    }
	}
	return;   
    }

    if ((int)cur_index == t.parent_index) { // in right subtree, all moves right
	child_index++;
    }
    
}

static void
handle_simple(dtransf t, vector<uint> & repr) {

    uint tsize = t.ope_path.size();
    uint & cur_index = repr[tsize]; // index in node

    if (tsize + 1 != repr.size()){
	assert_s(false,"Simple dtransf doesn't have right length ope_path");
    }

    if (cur_index > (uint) t.parent_index) {
	cur_index--;
    }
    return;
}

static void
handle_nonleaf(dtransf t, vector<uint> & repr) {

    if (match(repr, t.smallest_elem_path)){
	repr = t.ope_path;
	repr.push_back(t.parent_index);
	return;	
    }

}

OPEType
OPEdTransform::transform(OPEType val) {

    if (DEBUG_TRANSF) std::cerr << "val " << val << " \n";
  
    // repr consists of ope path followed by index
    vector<uint> repr = enc_to_vec(val);
    // ope encodings have the last index one less than the one in the btree
    repr[repr.size()-1]++;

    // now repr is the path in B tree
    
     if (DEBUG_TRANSF) cerr << "its path " << pretty_path(repr) << "\n";
    
    // loop thru each transf
    for (auto t :  ts) {
	if (match(t.ope_path, repr)) { // need to transform
	    if (DEBUG_TRANSF) cerr << "match with ope path " << pretty_path(t.ope_path) << "\n";
	    
	    switch (t.optype) {
	    case TransType::NONLEAF: {
		handle_nonleaf(t, repr);
		break;
	    }
	    case TransType::SIMPLE: {
		handle_simple(t, repr);
		break;
	    }
	    case TransType::ROTATE_FROM_RIGHT: {
		handle_rotate_from_right(t, repr);
		break;
	    }
	    case TransType::ROTATE_FROM_LEFT: {
		handle_rotate_from_left(t, repr);
		break;
	    }
	    case TransType::MERGE_WITH_LEFT:
	    case TransType::MERGE_WITH_RIGHT: {
		handle_merge(t, repr);
		break;
	    }
	    default: assert_s(false, "invalid OpType");
	    }

	}
    }

    // ope encodings have the last index one less than the one in the btree

    assert_s(repr.size() > 0 && repr[repr.size() -1 ] > 0, "index must be > 0");
    repr[repr.size()-1]--;
    return vec_to_enc(repr);
}

/* TODO
static ostream&
operator>>(dtransf t, ostream & out) {
    //TODO: see the same operator for insert
    return out;
}

static istream&
operator<<(dtransf & t, istream & is) {
    //TODO  
    return is;
}

ostream &
OPEdTransform::operator>>(ostream &out) {
    //TODO

    return out;
}
*/

istream &
OPEdTransform::operator<<(istream & is) {
    from_stream(is);
    return is;
}

void
OPEdTransform::from_stream(istream & is){
    //TODO 

}
/*
static bool
tr_equals(dtransf t1, dtransf t2) {
    //TODO

    assert_s(false, "unimplemented");
    return true; 
 }
*/
bool
OPEdTransform::equals(OPEdTransform t) {
    //TODO
    assert_s(false, "unimplemented");
    return false;
}
