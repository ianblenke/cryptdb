#include <ope/transform.hh>

#include <util/util.hh>

using namespace std;

void
OPETransform::push_change(OPEType ope_path, int num, int index_inserted, int split_point) {
    transf t;
    t.ope_path = path_to_vec(ope_path, num);
    cerr << "given ope path " << ope_path << " path is " << pretty_path(t.ope_path) << "\n";
    t.num = num;
    t.index_inserted = index_inserted;
    if (ts.size()) {
	assert_s(index_inserted == (int)(1 + ts.back().ope_path[ts.back().num - 1]), " inconsistent insert");
    }
    t.split_point = split_point;

    ts.push_back(t);
}

void
OPETransform::get_interval(OPEType & omin, OPEType & omax) {
    /*
     * Interval depends on the highest node affected in which there was just an
     * insert and no split.
     */

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
    }

    // ope representations have index one less than B tree index
    assert_s(v > 0, "logic error with omin");
    v--;
    omin = (v << (64 - nbits));
	
}

OPEType
OPETransform::transform(OPEType val) {
  
    if (DEBUG_TRANSF) std::cerr << "val " << val << " \n";
  
    // repr consists of ope path followed by index
    vector<uint> repr = enc_to_vec(val);
    // ope encodings have the last index one less than the one in the btree
    repr[repr.size()-1]++;

     if (DEBUG_TRANSF) cerr << "its path " << pretty_path(repr) << "\n";
    
    bool met_transformation = false;
    
    // loop thru each transf
    for (auto t :  ts) {
	if (match(t.ope_path, repr)) { // need to transform
	     if (DEBUG_TRANSF) cerr << "match with ope path " << pretty_path(t.ope_path) << "\n";
	    uint & index = repr[t.ope_path.size()];
	    if (!met_transformation && ((uint)t.index_inserted <= index)) {
		index++;
	    }
	    if (t.split_point >=0 && t.split_point < (int)index) {
		if (DEBUG_TRANSF) cerr << "met split point " << t.split_point << "\n";
		index = index - t.split_point - 1;
		assert_s(t.ope_path.size() > 0, "split point at root\n");
		repr[t.ope_path.size()-1]++;
		if (DEBUG_TRANSF) cerr << "becomes " << pretty_path(repr) << "\n";
	    }
	    met_transformation = true;
	}

    }

    // ope encodings have the last index one less than the one in the btree
    assert_s(repr.size() > 0 && repr[repr.size() -1 ] > 0, "index must be >0");
    repr[repr.size()-1]--;
    return vec_to_enc(repr);
}
