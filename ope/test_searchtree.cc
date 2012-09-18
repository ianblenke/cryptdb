/*
 * Tests search trees, AVL, scapegoat, binary, nway.
 */

#include "btree.hh"
#include <sstream>
#include <vector>
#include <algorithm>

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

template<class T>
static bool
mcontains(const T & vals, const string & test_val) {
    for (auto it : vals) {
	if (it == test_val) {
	    return true;
	}
    }
    
    return false;
}


static void
check_order(list<string> & vals) {
    string prev;
    bool first = true;
    for (auto it : vals) {
	if (!first) {
	    assert_s(it >= prev, "values in tree are not ordered properly: prev " + prev + " it " + it);
	}
	first = false;
	prev = it;
    }   
}

static void
check_order_and_values(vector<string> & input_vals,
		       list<string> & tree_vals) {

    for (string input_val: input_vals) {
	assert_s(mcontains(tree_vals, input_val), "val " + input_val + " not found in tree");
    }

    // check tree vals are in order
    check_order(tree_vals);
}

static void
check_good_height(uint maxheight, uint no_elems, uint breadth) {
    assert_s(maxheight <= (log(no_elems*1.0)/log(breadth*1.0)) + 1, "tree is too high");
}

/*static void
  progress_info(string mess, uint total, uint i) {
  if (total > 1000) {
  if (i % (total / 10) == 0) {
  cerr << (i*100.0/total) << "% " + mess + " \n";
  }
  }
  }*/

class Test {

public:

    static void
    test_help(vector<string> & vals, uint no_elems) {
    
	uint no_inserted_checks = int(sqrt(no_elems)) + 1;
	uint no_not_in_tree = no_inserted_checks;
	uint no_deletes = no_inserted_checks;
	uint period_Merkle_check = no_inserted_checks/5 + 1;

	Node::m_failure.invalidate();

	Node::m_failure.m_key = "";

	RootTracker tracker;  // maintains a pointer to the current root of the b-tree

	Node* root_ptr = new Node(tracker);

	tracker.set_root(null_ptr, root_ptr);

	Elem elem;
	//insert values in tree
	for (uint i=0; i< no_elems; i++) {
	    elem.m_key = vals[i];
	    elem.m_payload = vals[i]+" hi you";
	    InsMerkleProof p;
	    tracker.get_root()->tree_insert(elem, p);
	
	    // check Merkle hash tree integrity
	    if (i % period_Merkle_check == 0) {
		//cerr << "checking Merkle for tree so far \n";
		tracker.get_root()->check_merkle_tree();
		//cerr << "passed check\n";
	    }
	}


	//check tree structure is correct
	list<string> treeorder;
	tracker.get_root()->in_order_traverse(treeorder); 
	check_order_and_values(vals, treeorder);

	uint max_height = tracker.get_root()->max_height();
	cout << " -- max height of tree is " << max_height << " fanout " << max_elements << "\n";
	check_good_height(max_height, no_elems, max_elements);

	tracker.get_root()->check_merkle_tree();


//check if no_inserted random elements are indeed in the tree
	for (uint i = 0 ; i < no_inserted_checks; i++) {
	    string test_val = vals[rand() % no_elems];
	    Elem desired;
	    desired.m_key = test_val;
	    Node * last;
	    Elem& result = tracker.get_root()->search(desired, last);
	    assert_s(desired.m_key == result.m_key, "could not find val that should be there");

	}

	//check if no_not_in_tree elements are indeed not in the tree
	for (uint i = 0; i < no_not_in_tree; i++) {
	    string test_val;
	    uint counter = 100;
	    while (true) {
		stringstream s;
		s << rand();
		test_val =  s.str();
		if (!mcontains(vals, test_val)) {
		    break;
		}
		counter--;
		if (counter > 0) {
		    cerr << "could not find item not in vals for testing";
		}
	    }
	    Elem desired;
	    desired.m_key = test_val;
	    Node * last;
	    Elem& result = tracker.get_root()->search(desired, last);
	    assert_s(result == Node::m_failure, "found elem that does not exist");

	}


	//check deletion
	for (uint i = 0 ; i < no_deletes; i++) {

	    string old_merkle_hash = tracker.get_root()->merkle_hash;
	    
	    string test_val = vals[rand() % no_elems];
	    Elem desired;
	    desired.m_key = test_val;
	    //delete it
	    DelMerkleProof delproof;
	    bool deleted = tracker.get_root()->tree_delete(desired, delproof);

	    Node * last;
	    Elem result = tracker.get_root()->search(desired, last);
	    assert_s(result == Node::m_failure, "found element that should have been deleted");
	
	    // check Merkle hash tree integrity
	    if (i % period_Merkle_check == 0) {
		//cerr << "check Merkle \n";
		tracker.get_root()->check_merkle_tree();

		//check deletion proof only if deleted
		if (deleted) {
		    string new_merkle_root;
		    bool r = verify_del_merkle_proof(delproof, old_merkle_hash, new_merkle_root);
		    assert_s(r, "deletion proof does not verify");
		    assert_s(new_merkle_root == tracker.get_root()->merkle_hash,
		    	 "verify_del gives incorrect new merkle root");
		    //cerr << "check success\n";
		}
	    }

	}
    
	//check tree structure is correct after deletion
	treeorder.clear();
	tracker.get_root()->in_order_traverse(treeorder); 
	check_order(treeorder);
	check_good_height(tracker.get_root()->max_height(), no_elems, max_elements);

    }
    


// test the b-tree.
// -- inserts many elements in random order, increasing order, decreasing order,
// and checks that the tree is built correctly -- correct order and max height
// -- searches for existing elements, searches for nonexistin elements
// -- tests deletion
// -- checks that Merkle hash tree is overall correct with some period
    static void
    testBMerkleTree() {
    
	uint no_elems = 3000;

	vector<string> vals;
	srand( time(NULL));

	cerr << "- " << no_elems << " values in random order: \n";
	// test on random values
	for(uint i=0; i< no_elems; i++){
	    stringstream s;
	    s << rand();
	    vals.push_back(s.str());
	}
	test_help(vals, no_elems); 
   
	// test on values in increasing order
	sort(vals.begin(), vals.end());
	cerr << "- " << no_elems << " values in increasing order: \n";
	test_help(vals, no_elems); 
   
	// test on values in decreasing order
	reverse(vals.begin(), vals.end());
	cerr << "- " << no_elems << " values in decreasing order: \n";
	test_help(vals, no_elems);

	cerr << "test B + Merkle tree OK.\n";
   
    }


    static void change(string & s) {
	s[0] = (char)((int)s[0] + 1 % 200);
    }
    static void change(Node * last) {
	change(last->m_vector[0].m_key); 
    }
    
    static void
    testMerkleProof() {
	uint no_elems = 10000;
	uint no_proof_checks = 100;

	// Insert no_elems in a tree
	cerr << "creating tree with " << no_elems << " elements "
	     << " to check Merkle proofs \n";
	vector<string> vals;
	srand( time(NULL));

	for(uint i=0; i< no_elems; i++){
	    stringstream s;
	    s << rand();
	    vals.push_back(s.str());
	}

	Node::m_failure.invalidate();

	Node::m_failure.m_key = "";

	RootTracker tracker;  // maintains a pointer to the current root of the b-tree

	Node* root_ptr = new Node(tracker);

	tracker.set_root(null_ptr, root_ptr);

	Elem elem;
	//insert values in tree
	for (uint i=0; i< no_elems; i++) {
	    elem.m_key = vals[i];
	    elem.m_payload = vals[i]+" hi you";
	    InsMerkleProof p;
	    tracker.get_root()->tree_insert(elem, p);
	}

	string merkle_root = tracker.get_root()->merkle_hash;

	// Correct proof: pick elements at random and check proofs for them
	
	cerr << "checking " << no_proof_checks << " correct proofs... ";
	
	for (uint i = 0 ; i < no_proof_checks; i++) {
	    string test_val = vals[rand() % no_elems];
	    Elem desired;
	    desired.m_key = test_val;
	    Node * last;
	    Elem& result = tracker.get_root()->search(desired, last);
	    assert_s(desired.m_key == result.m_key, "could not find val that should be there");

	    MerkleProof proof = get_search_merkle_proof(last);
	    assert_s(verify_merkle_proof(proof, merkle_root), "failed to verify merkle proof");

	}
	cerr << "ok.\n";

	// Incorrect proofs
	
	cerr << "checking " << no_proof_checks << " incorrect proofs... ";

	string bad_merkle_root = merkle_root;
	bad_merkle_root[1] = (bad_merkle_root[1] + 1) % 200;
	
	for (uint i = 0 ; i < no_proof_checks/2+1; i++) {
	    string test_val = vals[rand() % no_elems];
	    Elem desired;
	    desired.m_key = test_val;
	    Node * last;
	    Elem& result = tracker.get_root()->search(desired, last);
	    assert_s(desired.m_key == result.m_key, "could not find val that should be there");

	    MerkleProof proof = get_search_merkle_proof(last);
	    assert_s(!verify_merkle_proof(proof, bad_merkle_root), "verified bad merkle proof");

	}

	for (uint i = 0 ; i < no_proof_checks/2+1; i++) {
	    string test_val = vals[rand() % no_elems];
	    Elem desired;
	    desired.m_key = test_val;
	    Node * last;
	    Elem& result = tracker.get_root()->search(desired, last);
	    assert_s(desired.m_key == result.m_key, "could not find val that should be there");

	    //change an element in last
	    change(last); 
	    MerkleProof proof = get_search_merkle_proof(last);
	    assert_s(!verify_merkle_proof(proof, bad_merkle_root), "verified bad merkle proof");

	}
	cerr << "ok.\n";

	cerr << "Test MerkleProof OK.\n";

    }
    
}; // end of class Test

int main(int argc, char ** argv)
{
    Test::testBMerkleTree();
    Test::testMerkleProof();
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
