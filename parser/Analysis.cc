#include <parser/Analysis.hh>

using namespace std;

string ItemMeta::stringify() {
    stringstream res;
    res << "(ItemMeta: onion " << o << " uptolevel " << (int)uptolevel << " basefield " << stringify_ptr(basefield) << ") ";
    return res.str();
}

EncSet::EncSet() : osl(FULL_EncSet.osl) {}

EncSet
EncSet::intersect(const EncSet & es2) const
{
    OnionLevelFieldMap m;
    for (auto it2 = es2.osl.begin();
         it2 != es2.osl.end(); ++it2) {
        auto it = osl.find(it2->first);
        if (it != osl.end()) {
            SECLEVEL sl = (SECLEVEL)min((int)it->second.first,
                                        (int)it2->second.first);
            if (it->second.second == NULL) {
                m[it->first] = LevelFieldPair(
                        sl, it2->second.second);
            } else if (it2->second.second == NULL) {
                m[it->first] = LevelFieldPair(
                        sl, it->second.second);
            } else if (it->second.second == it2->second.second) {
                m[it->first] = LevelFieldPair(
                        sl, it->second.second);
            }
        }
    }
    return EncSet(m);
}

EncSet
EncSet::chooseOne() const
{
    // Order of selection is encoded in this array.
    // The onions appearing earlier are the more preferred ones.
    static const onion onion_order[] = {
        oDET,
        oOPE,
        oAGG,
        oSWP,
    };
    static size_t onion_size = sizeof(onion_order) / sizeof(onion_order[0]);
    for (size_t i = 0; i < onion_size; i++) {
        auto it = osl.find(onion_order[i]);
        if (it != osl.end()) {
            OnionLevelFieldMap m;
            m[onion_order[i]] = it->second;
            return EncSet(m);
        }
    }
    return EncSet(OnionLevelFieldMap());
}
