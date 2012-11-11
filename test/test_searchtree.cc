/*
 * Tests search trees.
 */


#include <sstream>
#include <vector>
#include <algorithm>
#include <util/util.hh>
#include <ope/btree.hh>
#include <ope/transform.hh>
#include <crypto/blowfish.hh>


using namespace std;

static void
check_order(list<string> & vals) {
    string prev;
    bool first = true;
    for (auto it : vals) {
	if (!first) {
	    assert_s(it > prev, "values in tree are not ordered properly: prev " + prev + " it " + it);
	}
	first = false;
	prev = it;
    }   
}

static void
check_order_and_values(vector<string> & input_vals,
		       list<string> & tree_vals) {

    for (string input_val: input_vals) {
	assert_s(mcontains(tree_vals, input_val), "val  not found in tree");
    }

    // check tree vals are in order
    check_order(tree_vals);
}

class Test {

public:

    static void
    check_good_tree(RootTracker * tracker) {
	
	tracker->get_root()->check_merkle_tree();
	
	// Check tree is in right order and has good height
	list<string> treeorder;
	tracker->get_root()->in_order_traverse(treeorder); 
	check_order(treeorder);
	
	uint max_height = tracker->get_root()->max_height();
	cout << " -- max height of tree is " << max_height << " fanout " << b_max_keys << "\n";
	check_good_height(max_height, treeorder.size(), b_min_keys);
	
	treeorder.clear();
    }

    static void
    check_good_tree(RootTracker * tracker, vector<string> & vals, uint no_elems) {
	
	tracker->get_root()->check_merkle_tree();
	cerr << "merkle tree verifies after insert\n";
	
	// Check tree is in right order and has good height
	list<string> treeorder;
	tracker->get_root()->in_order_traverse(treeorder); 
	check_order_and_values(vals, treeorder);
	
	uint max_height = tracker->get_root()->max_height();
	cout << " -- max height of tree is " << max_height << " fanout " << b_max_keys << "\n";
	check_good_height(max_height, no_elems, b_min_keys);
	
	treeorder.clear();
    }

    static void
    check_equals(State & s1, State & s2) {
	assert_s(s1.size() == s2.size(), "serialization error: states differ in size");

	for (uint i = 0; i < s1.size(); i++) {
	    assert_s(s1[i].equals(s2[i]), "states are not equal");
	}
    }
    static void
    test_serialize(UpdateMerkleProof & p) {

	stringstream ss;
	ss << "hello" << " ";
	p >> ss;
	string res = ss.str();
	stringstream ss2 (res);
	UpdateMerkleProof p2;
	string greeting;
	ss2 >> greeting;
	assert_s(greeting == "hello", "incorrect behavior");
	p2 << ss2;

	check_equals(p.st_before, p2.st_before);
	check_equals(p.st_after, p2.st_after);


    }

    static void
    check_equals(list<NodeInfo> & s1, list<NodeInfo> & s2) {
	assert_s(s1.size() == s2.size(), "serialization error: states differ in size");

	auto i1 = s1.begin();
	auto i2 = s2.begin();
	
	for (; i1 != s1.end(); i1++, i2++) {
	    if (!(*i1 == *i2)) {
		cerr << "i1 is ";
		*i1 >> cerr;
		cerr << "\n i2 is\n";
		*i2 >> cerr;
		cerr << "\n";
	    }
		    
	    assert_s(*i1==*i2, "NodeInfo are not equal");
	}
    }
  

    static void
    test_serialize(MerkleProof p) {

	stringstream ss;
	ss << "hello" << " ";
	p >> ss;
	string res = ss.str();
	stringstream ss2 (res);
	MerkleProof p2;
	string greeting;
	ss2 >> greeting;
	assert_s(greeting == "hello", "incorrect behavior");
	p2 << ss2;

	check_equals(p.path, p2.path);

    }
    
    static void
    test_help(vector<string> & vals, uint no_elems) {

	cerr << "Testing B tree.. \n";

	// Frequency of testing certain aspects
	uint no_inserted_checks =  int(sqrt(no_elems)) + 1;
	uint no_not_in_tree = 10; //no_inserted_checks;
	//uint delete_freq = 2; //one in deletes_freq will be deleted
	uint period_Merkle_check = no_inserted_checks;
	//uint period_Merkle_del_check = 1;
	uint period_Merkle_insert_check = no_inserted_checks;

	// Create tree
	Node::m_failure.invalidate();
	Node::m_failure.m_key = "";
	RootTracker * tracker = new RootTracker();  // maintains a pointer to the current root of the b-tree
	Node* root_ptr = new Node(tracker);
	tracker->set_root(null_ptr, root_ptr);

	// Insert into tree
	Elem elem;
	for (uint i=0; i< no_elems; i++) {
	    elem.m_key = vals[i];
	    UpdateMerkleProof p;
	    string merkle_root_before = tracker->get_root()->merkle_hash;
	    cerr << "merkle root before " << merkle_root_before << "\n";
	    bool inserted = tracker->get_root()->tree_insert(elem, p);
	    
	    assert_s(inserted, "element was not inserted");
	    
	    // check Merkle hash tree integrity
	    if (i % period_Merkle_check == 0) {
		//cerr << "checking Merkle for tree so far \n";
		tracker->get_root()->check_merkle_tree();
		//cerr << "passed check\n";
	    }
	    if (i % period_Merkle_insert_check == 0) {
		test_serialize(p);
		string merkle_after;
		assert_s(verify_ins_merkle_proof<uint64_t, blowfish>(p, elem.m_key, merkle_root_before, merkle_after),
			 "insert merkle proof not verified");
		assert_s(merkle_after == tracker->get_root()->merkle_hash, "verify insert does not give correct merkle proof");
	    }
	}

	// Check tree is correct
	check_good_tree(tracker, vals, no_elems);
	
	// Check if some inserted values are indeed in the tree
	for (uint i = 0 ; i < no_inserted_checks; i++) {
	    string test_val = vals[rand() % no_elems];
	    Elem desired;
	    desired.m_key = test_val;
	    Node * last;
	    Elem& result = tracker->get_root()->search(desired, last);
	    assert_s(desired.m_key == result.m_key, "could not find val that should be there");

	    MerkleProof p = node_merkle_proof(last);
	    test_serialize(p);
	    assert_s(verify_merkle_proof(p, tracker->get_root()->merkle_hash), "merkle proof for get does not verify");
	}
	
	// Check tree is correct
	check_good_tree(tracker, vals, no_elems);

	//Check that some values are indeed not in the tree
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
	    Elem& result = tracker->get_root()->search(desired, last);
	    assert_s(result == Node::m_failure, "found elem that does not exist");

	}

	// Check tree is correct
	check_good_tree(tracker, vals, no_elems);
/*
		
	//Check that deletion works correctly
	for (uint i = 0 ; i < no_elems/delete_freq; i++) {

	    string old_merkle_hash = tracker.get_root()->merkle_hash;

	    // value to delete
	    uint index_to_del = i * delete_freq;
	    if (index_to_del >= no_elems) {
		break;
	    }
	    string test_val = vals[index_to_del];

	    // delete the value
	    Elem desired;
	    desired.m_key = test_val;
	    //delete it
	    UpdateMerkleProof delproof;
	    bool deleted = tracker.get_root()->tree_delete(desired, delproof);
	    assert_s(deleted, "tree delete does not delete");
	    
	    // check it is indeed deleted
	    Node * last;
	    Elem result = tracker.get_root()->search(desired, last);
	    assert_s(result == Node::m_failure, "found element that should have been deleted");

	    // check Merkle hash tree integrity
	    if (i % period_Merkle_del_check == 0) {
		//cerr << "check Merkle \n";
		tracker.get_root()->check_merkle_tree();

		//check deletion proof only if deleted
		string new_merkle_root;
		bool r = verify_del_merkle_proof(delproof, test_val, old_merkle_hash, new_merkle_root);
		assert_s(r, "deletion proof does not verify");
		assert_s(new_merkle_root == tracker.get_root()->merkle_hash,
		    	 "verify_del gives incorrect new merkle root");
		cerr << "deletion proof verified\n";
		
	    }

	}


	// Check tree is still correct
	check_good_tree(tracker);
*/	
    }

    static string
    inserted_here(UpInfo before, UpInfo after, int & pos) {
	NodeInfo node1 = before.node;
	NodeInfo node2 = after.node;

	if (node1.childr.size() == node2.childr.size()) {
	    return "";
	}

	assert_s(node1.childr.size() == (node2.childr.size() - 1), "error in logic");
	
	for (uint i = 0; i < node1.childr.size(); i++) {
	    if (node1.childr[i].key != node2.childr[i].key) {
		pos = i;
		return node2.childr[i].key;
	    }
	}

	pos = node2.childr.size() - 1;
	return node2.childr[pos].key;
    }

    static int
    cost(Node * node, int pos) {
	assert_s(pos >=0 , "error somewhere in logic");
	assert_s(pos < (int)node->m_count , "error somewhere in logic");
	
	int total_cost= 0;
	for (uint i = (uint)pos; i < node->m_count; i++) {
	    total_cost = 1 + total_cost; // add keys at current node
	    if (node->m_vector[i].mp_subtree != NULL) {
		total_cost = total_cost + cost(node->m_vector[i].mp_subtree, 0);
	    }
	}

	return total_cost;
    }

    // compute the cost of an insert in terms of elements that changed
    // their ope encoding
    static int
    cost(UpdateMerkleProof & p, RootTracker * tracker) {

	//check first if root grew
	if (p.st_before.size() == p.st_after.size() -1 ) {
	    return cost(tracker->get_root(), 0);
	}

	assert_s(p.st_before.size() == p.st_after.size(), "should be same height!");
	
	for (uint i = 0; i < p.st_after.size(); i++) {
	    string new_key;
	    int pos = -1;
	    new_key = inserted_here(p.st_before[i], p.st_after[i], pos);
	    if (new_key != "") {
		Elem desired;
		desired.m_key = new_key;
		Node * visited;
		tracker->get_root()->search(desired, visited);
		return cost(visited, pos);
	    }
	    

	}

	assert_s(false, "should have found a new key");
	return 0;    
    }

    static void
    test_B_complexity(vector<string> & vals, uint no_elems) {

	Node::m_failure.invalidate();

	Node::m_failure.m_key = "";

	RootTracker * tracker = new RootTracker();  // maintains a pointer to the current root of the b-tree
	Node* root_ptr = new Node(tracker);
	tracker->set_root(null_ptr, root_ptr);

	Elem elem;
	extern uint Merklecost;
	Merklecost = 0;
	uint ope_cost = 0;
	uint eff_elems = 0;
	
	//insert values in tree
	for (uint i=0; i< no_elems; i++) {
	    elem.m_key = vals[i];
	    UpdateMerkleProof p;
	    bool b = tracker->get_root()->tree_insert(elem, p);
	    if (b) {
		eff_elems++;
		ope_cost += cost(p, tracker);
	    }
	}
	
	cerr << "ope change cost " << ope_cost << " for " << no_elems << " inserts from zero " <<
	    " and effective elements " << eff_elems << "\n";
	cerr << "RATIO " << ope_cost*1.0/eff_elems << "\n";
	cerr << "total Merkle cost is " << Merklecost*1.0/eff_elems << "\n";
	cerr << "max height was " << tracker->get_root()->max_height() << "\n";
	cerr << "minimum keys " << b_min_keys << " max keys " << b_max_keys << "\n";
	

    }

// test the b-tree.
// -- inserts many elements in random order, increasing order, decreasing order,
// and checks that the tree is built correctly -- correct order and max height
// -- searches for existing elements, searches for nonexistin elements
// -- tests deletion
// -- checks that Merkle hash tree is overall correct with some period
    static void
    testBMerkleTree(int argc, char ** argv) {

	if (argc != 2) {
	    cerr << "usage: ./test_searchtree num_nodes_to_insert \n";
	    return;
	}

	assert_s(!OPE_MODE, "this test is for the B tree and must not be in ope mode");
	
	uint no_elems = atoi(argv[1]);


	vector<string> vals;
	srand( time(NULL));

	cerr << "\n\n- " << no_elems << " values in random order: \n";
	// test on random values
	for(uint i=0; i< no_elems; i++){
	    stringstream s;
	    s << rand();
	    vals.push_back(s.str());
	}

	test_help(vals, no_elems);

	// test on values in increasing order
	sort(vals.begin(), vals.end());
	cerr << "\n\n- " << no_elems << " values in increasing order: \n";
	test_help(vals, no_elems); 

	// test on values in decreasing order
	reverse(vals.begin(), vals.end());
	cerr << "\n\n- " << no_elems << " values in decreasing order: \n";
	test_help(vals, no_elems);

	cerr << "test B complexity finished OK.\n";
 
    }



    static void
    evalBMerkleTree(int argc, char ** argv) {

	if (argc != 2) {
	    cerr << "usage: ./test_searchtree num_nodes_to_insert \n";
	    return;
	}
	
	uint no_elems = atoi(argv[1]);


	vector<string> vals;
	srand( time(NULL));

	cerr << "\n\n- " << no_elems << " values in random order: \n";
	// test on random values
	for(uint i=0; i< no_elems; i++){
	    stringstream s;
	    s << rand();
	    vals.push_back(s.str());
	}

	test_B_complexity(vals, no_elems);

	// test on values in increasing order
	sort(vals.begin(), vals.end());
	cerr << "\n\n- " << no_elems << " values in increasing order: \n";
	test_B_complexity(vals, no_elems); 

	// test on values in decreasing order
	reverse(vals.begin(), vals.end());
	cerr << "\n\n- " << no_elems << " values in decreasing order: \n";
	test_B_complexity(vals, no_elems);

	cerr << "test B complexity finished OK.\n";
 
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

	RootTracker * tracker =  new RootTracker();  // maintains a pointer to the current root of the b-tree

	Node* root_ptr = new Node(tracker);

	tracker->set_root(null_ptr, root_ptr);

	Elem elem;
	//insert values in tree
	for (uint i=0; i< no_elems; i++) {
	    elem.m_key = vals[i];
	    UpdateMerkleProof p;
	    tracker->get_root()->tree_insert(elem, p);
	}

	string merkle_root = tracker->get_root()->merkle_hash;

	// Correct proof: pick elements at random and check proofs for them
	
	cerr << "checking " << no_proof_checks << " correct proofs... ";
	
	for (uint i = 0 ; i < no_proof_checks; i++) {
	    string test_val = vals[rand() % no_elems];
	    Elem desired;
	    desired.m_key = test_val;
	    Node * last;
	    Elem& result = tracker->get_root()->search(desired, last);
	    assert_s(desired.m_key == result.m_key, "could not find val that should be there");

	    MerkleProof proof = node_merkle_proof(last);
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
	    Elem& result = tracker->get_root()->search(desired, last);
	    assert_s(desired.m_key == result.m_key, "could not find val that should be there");

	    MerkleProof proof = node_merkle_proof(last);
	    assert_s(!verify_merkle_proof(proof, bad_merkle_root), "verified bad merkle proof");

	}

	for (uint i = 0 ; i < no_proof_checks/2+1; i++) {
	    string test_val = vals[rand() % no_elems];
	    Elem desired;
	    desired.m_key = test_val;
	    Node * last;
	    Elem& result = tracker->get_root()->search(desired, last);
	    assert_s(desired.m_key == result.m_key, "could not find val that should be there");

	    //change an element in last
	    change(last); 
	    MerkleProof proof = node_merkle_proof(last);
	    assert_s(!verify_merkle_proof(proof, bad_merkle_root), "verified bad merkle proof");

	}
	cerr << "ok.\n";

	cerr << "Test MerkleProof OK.\n";

    }

    static bool
    contains(const vector<string> & v, const string & x) {
	for (auto i : v) {
	    if (i == x){
		return true;
	    }
	}
	return false;
    }

    static string
    ope_t_print(OPETransform t, vector<uint> path) {
	OPEType val = vec_to_enc(path);
	vector<uint> str = enc_to_vec(t.transform(val));
	return pretty_path(str);
    }

    static bool
    check_equals(vector<uint> v1, vector<uint> v2) {
	if (v1.size() != v2.size()) {
	    return false;
	}

	for (uint i = 0; i < v1.size(); i++) {
	    if (v1[i] != v2[i]) {
		return false;
	    }
	}

	return true;
    }
    
    static void
    ope_transform_check(OPETransform & t, vector<uint> ope_path, vector<uint> new_path){
	cerr << "test on " << pretty_path(ope_path) << " vec to enc " << vec_to_enc(ope_path) << " "
	     << "enc to vec back " << pretty_path(enc_to_vec(vec_to_enc(ope_path))) << "\n";
	vector<uint> compute_path = enc_to_vec(t.transform(vec_to_enc(ope_path)));
	if (!check_equals(compute_path, new_path)) {
	    cerr << "expected " << pretty_path(new_path) << " computed " << pretty_path(compute_path) << "\n";
	    assert_s(false, "");
	}
    }

    static void
    test_transform() {
	OPETransform t;
	t.push_change(vec_to_path({2, 1, 5}), 3, 1, 1);
	t.push_change(vec_to_path({2, 1}), 2,    6, 3);
	t.push_change(vec_to_path({2}), 1,       2, 3);
	t.push_change(vec_to_path({}), 0,        3, -1);

	ope_transform_check(t, {0}, {0});
	ope_transform_check(t, {3,5, 6} ,{4, 5, 6} );
	ope_transform_check(t, {2, 2}, {2, 3});
	ope_transform_check(t, {0, 5, 6}, {0, 5, 6});
	cerr << " 2,1,5,0 " << ope_t_print(t, {2, 1, 5, 0}) << "\n";
	cerr << "2, 8  " << ope_t_print(t, {2, 8}) << "\n";
	cerr << "2, 1, 1, 5  " << ope_t_print(t, {2, 1, 1, 5}) << "\n";

	cerr << "Ok -- not root case and test interval!\n";
	
    }
    
    // Inputs: the number of values to test the tree on and the
    // frequency of testing (check tree every test_freq insertions)
    //
    // Tests the tree on three sequences of values:
    //   -- random
    //   -- sorted increasing
    //   -- sorted decreasing
    static void
    test_search_tree() {

	uint no_test_vals = 10000;
	
	vector<string> vals;

	//to make debugging deterministic
	srand(time(NULL));

	uint elems_inserted = 0;
	// test on random values
	for(uint i=0; i< no_test_vals; i++){
	    stringstream ss;
	    ss << rand();
	    string val = ss.str();
	    ss.clear();
	    // only insert unique elements
	    if (!contains(vals, val)) {
		vals.push_back(val);
		elems_inserted++;
	    }
	}

	
	test_help(vals, elems_inserted);

	//
	cerr << "UNCOMMENT STUFF NOW\n";
	
	// test on values in increasing order
	sort(vals.begin(), vals.end());
	test_help(vals, elems_inserted);
	
	// test on values in decreasing order
	reverse(vals.begin(), vals.end());
	test_help(vals, elems_inserted);
	
    }


    
}; // end of class Test

int main(int argc, char ** argv)
{
    //Test::test_search_tree();
    Test::testBMerkleTree(argc, argv);
    //Test::testMerkleProof();
    //Test::test_transform();
}

 
