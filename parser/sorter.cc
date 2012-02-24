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

static const size_t MAX_INMEM_ROWS = 1000000;

static bool merge_sort = false;

static size_t run_ids = 0;

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

static inline string tmp_file_name(size_t id) {
    ostringstream s;
    s << "/tmp/sorted_run_" << id;
    return s.str();
}

static inline string next_tmp_file_name() {
    return tmp_file_name(run_ids++);
}

static void emit_row(ostream& o, const vector<string>& row) {
    vector<string> n;
    n.reserve(row.size());
    for (size_t i = 0; i < row.size(); i++) {
        n.push_back(to_mysql_escape_varbin(row[i]));
    }
    o << join(n, "|") << endl;
}

struct process_row {
    void operator()(const vector<string>& row) const {
        rows.push_back(row);
        if (rows.size() == MAX_INMEM_ROWS) {
            merge_sort = true;
            FlushAndClearRows();
        }
    }
    static void FlushAndClearRows() {
        // write this file as a sorted run to disk
        if (rows.empty() || !merge_sort) return;

        string tmpname = next_tmp_file_name();
        ofstream f;
        f.open(tmpname.c_str());
        assert(f.is_open());

        sort(rows.begin(), rows.end(), sorter_functor());
        for (auto r : rows) emit_row(f, r);
        f.flush();
        rows.clear();
    }
};

struct appender {
    appender(vector<vector<string> >& buf) : buf(buf) {}
    void operator()(const vector<string>& elem) { buf.push_back(elem); }
    vector<vector<string> >& buf;
};

template <typename T>
size_t read_elems_from_stream(
    istream& input, ssize_t nelems, T functor) {
    vector<string> elems;
    string cur_elem;
    bool onescape = false;
    size_t read = 0;
    assert(nelems == -1 || nelems >= 0);
    while (input.good() && (nelems == -1 || read < ((size_t)nelems))) {
        char c = input.get();
        if (input.eof()) break;
        if (onescape) {
            cur_elem.push_back(c);
            onescape = false;
            continue;
        }
        switch (c) {
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
            //assert(elems.size() > maxSortKey);
            functor(elems);
            read++;
            elems.clear();
            break;
        default:
            cur_elem.push_back(c);
            break;
        }
    }
    assert(!onescape);
    assert(nelems == -1 || read <= ((size_t)nelems));
    return read;
}

static void sort_and_output() {
    if (merge_sort) {
        size_t n_ways = run_ids;
        //cerr << n_ways << endl;
        assert(n_ways >= 1);

        typedef vector<string> elem;
        typedef vector<elem> buffer;

        vector<buffer> buffers(n_ways);
        vector<ssize_t> positions(n_ways);
        vector<ifstream> streams(n_ways);
        for (size_t i = 0; i < n_ways; i++) {
            streams[i].open(tmp_file_name(i));
            assert(!streams[i].fail());

            appender app(buffers[i]);
            size_t read = read_elems_from_stream(streams[i], 4096, app);
            assert(read > 0);
            positions[i] = 0;
        }

        sorter_functor f;
        while (true) {
            // take min from buffer
            elem* minSoFar = NULL;
            ssize_t idxSoFar = -1;
            for (size_t i = 0; i < n_ways; i++) {
                if (positions[i] == -1) continue;
                elem& test = buffers[i][positions[i]];
                //cerr << "buffer " << i << " has pos " << positions[i] << endl;
                if (idxSoFar == -1 || f(test, *minSoFar)) {
                    minSoFar = &test;
                    idxSoFar = i;
                }
            }

            if (idxSoFar == -1) break; // done!
            emit_row(cout, *minSoFar);

            positions[idxSoFar]++; // advance cursor
            if (((size_t)positions[idxSoFar]) == buffers[idxSoFar].size()) {
                // need to load more stuff
                buffers[idxSoFar].clear();
                appender app(buffers[idxSoFar]);
                size_t read = read_elems_from_stream(streams[idxSoFar], 4096, app);
                if (read == 0) { // file is all processed
                    positions[idxSoFar] = -1;
                } else { // reset cursor
                    positions[idxSoFar] = 0;
                }
            }
        }
    } else {
        sort(rows.begin(), rows.end(), sorter_functor());
        for (auto r : rows) emit_row(cout, r);
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
    /*size_t r = */read_elems_from_stream(cin, -1, process_row());
    //cerr << r << endl;
    process_row::FlushAndClearRows();
    sort_and_output();
    return 0;
}
