#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <list>
#include <algorithm>
#include <type_traits>

#include <parser/cdb_rewrite.hh>
#include <parser/cdb_helpers.hh>

#include <unistd.h>

using namespace std;

static vector< vector<string> > rows;

static vector<size_t> sort_keys;

static bool interpret_as_int = false;

static void process_row(const vector<string>& row) {
    rows.push_back(row);
}

struct sorter_functor {
    inline bool
        operator()(const vector<string>& lhs, const vector<string>& rhs)
        const {
        size_t nkeys = sort_keys.size();
        for (size_t i = 0; i < nkeys; i++) {
            size_t pos = sort_keys[i];
            const string& l = lhs[pos];
            const string& r = rhs[pos];
            if (interpret_as_int) {
                long ll = strtol(l.c_str(), NULL, 10);
                long rl = strtol(r.c_str(), NULL, 10);
                if (ll < rl) return true;
                if (ll > rl) return false;
            } else {
                if (l < r) return true;
                if (l > r) return false;
            }
        }
        return false;
    }
};

static void sort_and_output() {
    sort(rows.begin(), rows.end(), sorter_functor());
    for (auto r : rows) {
        vector<string> n;
        n.reserve(r.size());
        for (size_t i = 0; i < r.size(); i++) {
            n.push_back(to_mysql_escape_varbin(r[i]));
        }
        cout << join(n, "|") << endl;
    }
}

int main(int argc, char** argv) {
    size_t maxSortKey = 0;
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "--interpret-as-int") {
            interpret_as_int = true;
            continue;
        }
        int arg = atoi(argv[i]);
        assert(arg >= 0);
        sort_keys.push_back((size_t) arg);
        if ((size_t) arg > maxSortKey) maxSortKey = (size_t) arg;
    }
    assert(!sort_keys.empty());

    char buf[4096];
    vector<string> elems;
    string cur_elem;
    bool onescape = false;
    while (cin.good()) {
        cin.read(buf, sizeof(buf));
        for (streamsize i = 0; i < cin.gcount(); i++) {
            if (onescape) {
                cur_elem.push_back(buf[i]);
                onescape = false;
                continue;
            }
            switch (buf[i]) {
            case '\\':
                onescape = true;
                break;
            case '|':
                elems.push_back(cur_elem);
                cur_elem.clear();
                break;
            case '\n':
                elems.push_back(cur_elem);
                cur_elem.clear();
                assert(elems.size() > maxSortKey);
                process_row(elems);
                elems.clear();
                break;
            default:
                cur_elem.push_back(buf[i]);
                break;
            }
        }
    }
    assert(!onescape);
    sort_and_output();
    return 0;
}
