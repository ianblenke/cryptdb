#include <ope/transform.hh>

#include <util/util.hh>

using namespace std;

void
OPETransform::push_change(OPEType ope_path, uint num, uint index_inserted, int split_point) {
    transf t;
    t.ope_path = path_to_vec(ope_path, num);
    cerr << "given ope path " << ope_path << " path is " << pretty_path(t.ope_path) << "\n";
    t.num = num;
    t.index_inserted = index_inserted;
    if (ts.size()) {
	assert_s(index_inserted == 1 + ts.back().ope_path[ts.back().num - 1], " inconsistent insert");
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
    
    omin = vec_to_enc(t.ope_path);
    OPEType x = (vec_to_path(t.ope_path) << num_bits) + t.index_inserted;
    uint nbits = (t.ope_path.size() + 1) * num_bits;

    omax = x << (64-nbits);
}



OPEType
OPETransform::transform(OPEType val) {
  
    std::cerr << "val " << val << " \n";
  
    vector<uint> path = enc_to_vec(val);

    cerr << "its path " << pretty_path(path) << "\n";
    
    bool leaf = true;
    
    // loop thru each transf
    for (auto t :  ts) {
	if (match(t.ope_path, path)) { // need to transform
	    cerr << "match with ope path " << pretty_path(t.ope_path) << "\n";
	    assert_s(path.size() > t.ope_path.size(), "logic error");
	    uint & index = path[t.ope_path.size()];
	    if (leaf && (t.index_inserted <= index)) {
		index++;
	    }
	    if (t.split_point < (int)index) {
		index = index - t.split_point - 1;
		assert_s(t.ope_path.size() > 0, "split point at root\n");
		path[t.ope_path.size()-1]++;
	    }
	}
	leaf = false;
    }

    return vec_to_enc(path);
}
