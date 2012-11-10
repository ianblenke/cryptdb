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
#include <ope/server.hh>
#include <ope/client.hh>
#include <ope/tree.hh>
#include <ope/btree.hh>
#include <crypto/blowfish.hh>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;


static blowfish * bc = new blowfish(passwd);


static void
check_order(OPETable<string> * ot, list<string> & vals) {
    string prev;
    bool first = true;
    for (auto it : vals) {
	if (!first) {
	    assert_s(bc->decrypt(valFromStr(it)) >
		     bc->decrypt(valFromStr(prev)), "values in tree are not ordered properly: prev " + prev + " it " + it);
	    //also check ope table
	    assert_s(ot->get(it).ope > ot->get(prev).ope, "ope table does not satisfy order");
	}
	first = false;
	prev = it;
    }   
}


static void
check_order_and_values(OPETable<string> * ot, vector<uint64_t> & input_vals,
		       list<string> & tree_vals) {

    for (string tree_val : tree_vals) {
	int val = bc->decrypt(valFromStr(tree_val));
	assert_s(mcontains(input_vals, val), "val  not found in tree");
    }

    // check tree vals are in order
    check_order(ot, tree_vals);
}


static void
check_correct(Server * s, vector<uint64_t> & vals) {
    //s->tracker->get_root()->check_merkle_tree();

    BTree * bt = (BTree *) s->ope_tree;

    list<string> treeorder;
    bt->tracker->get_root()->in_order_traverse(treeorder); 
    check_order_and_values(bt->opetable, vals, treeorder);

    uint max_height = bt->tracker->get_root()->max_height();
    cout << " -- max height of tree is " << max_height << " fanout " << b_max_keys << "\n";
    check_good_height(max_height, treeorder.size(), b_min_keys);

}

static int num_vals = 0;
static vector<uint64_t> vals;

static void *
client_thread(void * ) {

    cerr << "creating client...";
    ope_client<uint64_t, blowfish> * ope_cl = new ope_client<uint64_t, blowfish>(bc);
    cerr << " done \n";
    
    for (int i = 0; i < num_vals; i++) {
	ope_cl->encrypt(vals[i], true);
    }
 
    return NULL;
}

static void *
server_thread(void *s) {
  
    Server * serv = (Server *)s;
      
    serv->work();

    return NULL;
}

static void
runtest(int argc, char ** argv) {
    if (argc != 2) {
	cerr << "usage test no_vals\n";
	exit(1);
    }
    uint num_tests = atoi(argv[1]);
    
    vals = get_random(num_tests);
    
    pid_t pid = fork();
    if (pid == 0) {
	// child
	client_thread(NULL);
	exit(EXIT_SUCCESS);
    }
    // parent
    
    sleep(5);
    
    pthread_t server_thd;
    cerr << "creating server ... ";
    Server * s = new Server();
    cerr << " done \n";
    
    int r = pthread_create(&server_thd, NULL, server_thread, (void *)s);
    if (r) {
	cerr << "problem with thread create for server\n";
    }

    int status;

    pid_t pid2 = wait(&status);

    assert_s(pid == pid2, "incorrect pid");
    
    check_correct(s, vals);

}

static void
run_client() {
    
  cerr << "creating client...";
  ope_client<uint64_t, blowfish> * ope_cl = new ope_client<uint64_t, blowfish>(bc);
  cerr << " done \n";
  
  for (int i = 0; i < 9; i++) {
      ope_cl->encrypt(i, true);
  }
 
}

static void
run_server() {
    Server * serv = new Server();
    
    serv->work();
}


int main(int argc, char ** argv)
{
    if (argc == 2 && string(argv[1]) == "client") {
	run_client();
	return 0;
    }
    if (argc == 2 && string(argv[1]) == "server") {
	run_server();
	return 0;
    }
    
    //Test::test_search_tree();
    runtest(argc, argv);
    //Test::testMerkleProof();
    //Test::test_transform();
}

 
