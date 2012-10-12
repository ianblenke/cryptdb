#include <util/ope-util.hh>

#include <sstream>

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
