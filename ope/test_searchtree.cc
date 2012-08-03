/*
 * Tests search trees, AVL, scapegoat, binary, nway.
 */

#include "btree.hh"
#include <sstream>

using namespace std;

/*
static void
test_help(vector<uint64_t> & vals, uint no_test_vals, uint test_freq) {

    AVLClass tree;
    
    uint counter = 0;
    
    for (auto val : vals) {
	tree.insert(val);
    }
}
*/

int main(int argc, char ** argv)
{

// test the b-tree. it inserts 100,000 elements,
// then searches for each of them, then deletes them in reverse order (also tested in
// forward order) and searches for all 100,000 elements after each deletion to
// ensure that all remaining elements remain accessible.


    int notests = 200;
    
    Node::m_failure.invalidate();

    Node::m_failure.m_key = "";

    Elem elem;


    RootTracker tracker;  // maintains a pointer to the current root of the b-tree

    Node* root_ptr = new Node(tracker);

    tracker.set_root (null_ptr, root_ptr);

    vector<string> search_vect;

    // prepare the key strings

    search_vect.resize(notests);

    int search_count = 0;

    for (int i=0; i<notests; i++) {

        stringstream stm;
        stm << i;
        stm >> search_vect[search_count++];

    }

    string s;

    cout << "finished preparing key strings\n";

    for (int i=0; i<notests; i++) {
        elem.m_key = search_vect[i];
        elem.m_payload = search_vect[i]+" hi you";
        tracker.get_root()->tree_insert(elem);

    }

    cout << "finished inserting elements\n";
    tracker.get_root()->dump();
    
    Node * last;

    for (int i=0; i<notests; i++) {
        Elem desired;
        desired.m_key = search_vect[i];
        Elem& result = tracker.get_root()->search (desired, last);
	assert_s(desired.m_key == result.m_key, "key not found in tree");

    }
    cout << "finished searching for elements\n";

    
    for (int i=notests-1; i>=0; i--) {

        Elem target;
        target.m_key = search_vect[i];
        tracker.get_root()->delete_element (target);

    }
    
    tracker.get_root()->dump();

}

 


// Inputs: the number of values to test the tree on and the
// frequency of testing (check tree every test_freq insertions)
//
// Tests the tree on three sequences of values:
//   -- random
//   -- sorted increasing
//   -- sorted decreasing
/*static void
test_search_tree(uint no_test_vals, uint test_freq) {

    vector<uint64_t> vals;

    srand( time(NULL));

    // test on random values
    for(int i=0; i< no_test_vals; i++){
	vals.push_back((uint64_t) rand());
    }
    test_help(vals, no_test_vals, test_freq);
    
    // test on values in increasing order
    sort(vals.begin(), vals.end());
    test_help(vals, no_test_vals, test_freq);
    
    // test on values in decreasing order
    reverse(vals.begin(), vals.end());
    test_help(vals, no_test_vals, test_freq);
    
}
*/
