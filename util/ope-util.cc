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
