

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

uint64_t compute_ope(uint64_t ope_path, uint nbits, uint index) {
    ope_path = (ope_path << num_bits) | index;
    nbits+=num_bits;

    return (ope_path << (64-nbits)) | (s_mask << (64-num_bits-nbits));
}


uint
minimum_keys (uint max_keys) {

    // minus 1 for the empty slot left for splitting the node

    int size = max_keys;

    int ceiling_func = (size-1)/2;

    if (ceiling_func*2 < size-1) {
        ceiling_func++;
    }

    int res = ceiling_func-1;  // for clarity, may increment then decrement
    assert_s(res > 0, "cannot have min nodes be 0 or less");

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
