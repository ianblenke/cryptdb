#include <ope/transform.hh>

#include <util/util.hh>

using namespace std;

OPETransform::OPETransform() {
    new_root = false;
}

void
OPETransform::push_change(OPEType ope_path, int num, int index_inserted, int split_point) {

    transf t;
    t.ope_path = path_to_vec(ope_path, num);
    if (DEBUG_TRANSF) cerr << "given ope path " << ope_path << " path is " << pretty_path(t.ope_path) << "\n";
    t.index_inserted = index_inserted;
    if (ts.size()) {
	transf last_t = ts.back();
	assert_s(index_inserted == (int)(1 + last_t.ope_path[last_t.ope_path.size() - 1]), " inconsistent insert");
    }
    t.split_point = split_point;

    ts.push_back(t);
}

void
OPETransform::add_root() {
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
OPETransform::get_interval(OPEType & omin, OPEType & omax) {
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
    
    transf t = ts.back();
    assert_s(t.split_point < 0, "last transformation should have no split point");

    omax = compute_ope(vec_to_path(t.ope_path), t.ope_path.size() * num_bits);

    /* Computing omin is less straightforward: */
    
    auto ts2 = ts;
    ts2.reverse();
    OPEType v = 0;
    uint nbits = 0;
    uint count = 0;
    // loop starting from highest node
    for (auto t : ts2) {
	count++;
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
    assert_s(v > 0, "logic error with omin");
   
    omin = compute_ope(v-1, nbits);
	
}

OPEType
OPETransform::transform(OPEType val) {

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
    assert_s(repr.size() > 0 && repr[repr.size() -1 ] > 0, "index must be >0");
    repr[repr.size()-1]--;
    return vec_to_enc(repr);
}


static ostream&
operator>>(transf t, ostream & out) {
    out <<  " " << t.ope_path.size() << " ";
    for (uint i = 0; i < t.ope_path.size(); i++) {
	out << t.ope_path[i] << " ";
    }

    out << t.index_inserted << " " << t.split_point << " ";

    return out;
}

static istream&
operator<<(transf t, istream & is) {
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
OPETransform::operator>>(ostream &out) {
    out << " " << ts.size();

    for (auto t: ts) {
	t >> out;
	out << " ";
    }

    out << new_root << " ";

    return out;
}

istream &
OPETransform::operator<<(istream & is) {
    uint size;
    is >> size;

    for (uint i = 0; i < size; i++) {
	transf t;
	t << is;
	ts.push_back(t);
    }

    is >> new_root;

    return is;
}

static bool
tr_equals(transf t1, transf t2) {
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
OPETransform::equals(OPETransform t) {
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
