#include <ope/opetable.hh>


using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
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
table_entry 
OPETable<EncT>::get(EncT encval) {
    auto p = find(encval);
    if (!p) {
	stringstream ss;
	ss << encval;
	assert_s(false, "key " + ss.str() + "for ope_table.get not found");	
    }
    return *p;
}

template <class EncT>
bool
OPETable<EncT>::insert(EncT encval, uint64_t ope, TreeNode * n) {
    auto it = table.find(encval);

    if (it == table.end()) {
	table_entry te;
	te.ope = ope;
	te.refcount = 1;
	te.n = n;
	table[encval] = te;

	return true;
    }

    it->second.refcount++;
    return false;
}

template <class EncT>
bool
OPETable<EncT>::update(EncT encval, uint64_t newope, TreeNode * n) {
    auto it = table.find(encval);

    if (it == table.end()) {
	return false;
    } else {
	it->second.n = n;
	it->second.ope = newope;
	return true;
    }
}

template<class EncT>
bool
OPETable<EncT>::remove(EncT encval) {
    return (table.erase(encval) > 0);
}

template<class EncT>
void
OPETable<EncT>::dump() {
    cerr << "OPE table dump:\n";
    for (auto e : table) {
	cerr << "ciph " << niceciph(Cnv<EncT>::StrFromType(e.first)) << " ope " << e.second.ope << " ref count " << e.second.refcount << "\n";
    }
}

/*
 * Explicitly instantiate the tree template for various ciphertext types.
 */
template struct OPETable<uint64_t>;
template struct OPETable<uint32_t>;
template struct OPETable<uint16_t>;
template struct OPETable<std::string>;
