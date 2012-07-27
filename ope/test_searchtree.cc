/*
 * Tests search trees, AVL, scapegoat, binary, nway.
 */


static void
test_help(vector<uint64_t> & vals, uint no_test_vals, uint test_freq) {

    AVLClass tree;
    
    uint counter = 0;
    
    for (auto val : vals) {
	tree.insert(val);
    }
}

// Inputs: the number of values to test the tree on and the
// frequency of testing (check tree every test_freq insertions)
//
// Tests the tree on three sequences of values:
//   -- random
//   -- sorted increasing
//   -- sorted decreasing
static void
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
