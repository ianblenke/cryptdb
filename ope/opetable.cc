#include <ope/opetable.hh>


using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::ceil;
using std::sort;
using std::vector;
using std::cerr;
using std::map;
using std::max;


template <class EncT>
table_entry *
OPETable<EncT>::find(EncT encval) {
    auto it = table.find(encval);
    if (it == table.end()) {
	return NULL;
    } else {
	return &it->second;
    }
}

template <class EncT>
table_entry *
OPETable<EncT>::get(EncT encval) {
    auto p = find(encval);
    assert_s(p, "key for ope_table.get not found");
    return p;
}

template <class EncT>
bool
OPETable<EncT>::insert(EncT encval, uint64_t ope) {
    auto it = table.find(encval);

    if (it == table.end()) {
	table_entry te;
	te.ope = ope;
	te.refcount = 1;
	table[encval] = ope;

	return true;
    }

    it->second.refcount++;
    return false;
}

template <class EncT>
bool
OPETable<EncT>::update(EncT encval, uint64_t newope) {
    auto it = table.find(encval);

    if (it == table.end()) {
	return false;
    } else {
	it->second.ope = newope;
	return true;
    }
}
