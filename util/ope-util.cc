

#include <sstream>

#include <util/ope-util.hh>
#include <util/util.hh>

using namespace std;

string
opeToStr(OPEType ope) {
    stringstream ss;
    ss.str("");
    ss.clear();
    ss << ope;
    return ss.str();
}

uint64_t
path_append(uint64_t v, uint index) {
    return (v << num_bits | index);
}



std::vector<uint>
path_to_vec(OPEType val, int num) {
    vector<uint> res = std::vector<uint>(num);
    for (int i = num-1; i>=0; i--) {
	res[i] = val % UNIT_VAL;
	val = val >> num_bits;
    }

    assert_s(val == 0, "val should have gotten zero");
    return res;
   
}

std::vector<uint>
enc_to_vec(OPEType val) {
    uint64_t v = 0, nbits = 0, index = 0;
    
    my_parse_ope(val, v, nbits, index);

    v = (v << num_bits) + index;

    uint num = 1 + nbits/num_bits;

    return path_to_vec(v, num);
 }


OPEType
vec_to_path(const std::vector<uint> & vec) {
    OPEType val = 0;
    for (uint i = 0; i < vec.size(); i++) {
	val = (val << num_bits) + vec[i];
    }
    return val;
}


OPEType
vec_to_enc(const std::vector<uint> & vec) {
    assert_s(vec.size() >= 1, "vec must have at least one element -- the index");
    return compute_ope(vec_to_path(vec), vec.size() * num_bits);
}


string
pretty_path(vector<uint> v) {
    stringstream ss;
    for (uint i : v) {
	ss << i << " ";
    }
    return ss.str();
}




bool
match(const std::vector<uint> & ope_path, const std::vector<uint> & repr) {

    // opepath needs to be strictly shorter than repr size
    if (ope_path.size() >= repr.size()) {
	return false;
    }
    
    for (uint i = 0; i < ope_path.size();i++){
	if (ope_path[i] != repr[i]) {
	    return false;
	}
    }
    return true;
}

uint
minimum_keys (uint max_keys) {
    // max keys contains empty slot, key "" and the normal keys

    // minus 1 for the empty slot left for splitting the node

    int size = max_keys;

    int ceiling_func = (size-1)/2;

    if (ceiling_func*2 < size-1) {
        ceiling_func++;
    }

    int res = ceiling_func-1;  // for clarity, may increment then decrement

    assert_s(res >= 0, "cannot have negative min keys");
    if (res <= 1){
	res++;
	assert_s((uint)res < max_keys-1, "cannot have min keys be the same as max_keys-1");
    }
    
    return res;
} 


string
short_string(string s) {
    if (s.size() <= 10) {
	return s;
    }
    return s.substr(s.size() - 11, 10);
}
string
read_short(string s) {
    stringstream r;
    for (uint i = 0; i < s.size(); i++) {
	r << (((int)s[i] - '0') % 10);
    }
    return r.str();
}
