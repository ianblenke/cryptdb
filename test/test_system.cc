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
#include <crypto/aes_cbc.hh>

using namespace std;


static blowfish * bc = new blowfish(passwd);
static aes_cbc * bc_aes = new aes_cbc(passwd, true);

static uint ciph_size = 0;
static uint num_tests = 0;
vector<uint64_t> vals;
bool is_malicious;



template<class V, class BC>
static void
check_order(OPETable<string> * ot, list<string> & vals, BC * bc) {
    
    string prev;
    bool first = true;
    for (auto it : vals) {
	if (!first) {
	    assert_s(bc->decrypt(Cnv<V>::TypeFromStr(it)) >
		     bc->decrypt(Cnv<V>::TypeFromStr(prev)), "values in tree are not ordered properly: prev " + prev + " it " + it);
	    //also check ope table
	    assert_s(ot->get(it).ope > ot->get(prev).ope, "ope table does not satisfy order");
	}
	first = false;
	prev = it;
    }   
}

template<class V, class BC>
static void
check_order_and_values(OPETable<string> * ot, vector<uint64_t> & input_vals,
		       list<string> & tree_vals, BC * bc) {

    cerr << "-- check values\n";
    for (string tree_val : tree_vals) {
	V val = bc->decrypt(Cnv<V>::TypeFromStr(tree_val));
	uint64_t val2 = Cnv<uint64_t>::TypeFromStr(Cnv<V>::StrFromType(val)); //ugly..
	assert_s(mcontains(input_vals, val2), "val  not found in tree");
    }

    cerr << "-- check order in tree and ope table\n";
    // check tree vals are in order
    check_order<V, BC>(ot, tree_vals, bc);
}


template<class V, class BC>
static void
check_correct(Server * s, vector<uint64_t> & vals, BC * bc) {

    cerr << "checking sever state is correct: \n";
    //s->tracker->get_root()->check_merkle_tree();

    BTree * bt = (BTree *) s->ope_tree;

    list<string> treeorder;
    bt->tracker->get_root()->in_order_traverse(treeorder); 
    check_order_and_values<V, BC>(bt->opetable, vals, treeorder, bc);

    cerr <<"-- check height\n";
    uint max_height = bt->tracker->get_root()->max_height();
    cout << " -- max height of tree is " << max_height << " fanout " << b_max_keys << "\n";
    check_good_height(max_height, treeorder.size(), b_min_keys);

}

static void
client64() {
    cerr << "creating uint64, blowfish client...";
    ope_client<uint64_t, blowfish> * ope_cl = new ope_client<uint64_t, blowfish>(bc, is_malicious);
    cerr << "client created \n";
    
    cerr << "num tests " << num_tests << "\n";
    
    for (uint i = 0; i < vals.size(); i++) {
	cerr << "ENCRYPT " <<  i << "-th val: " << vals[i] << "\n";
	ope_cl->encrypt(vals[i], true);
    }
    cerr << "DONE!\n";
}

static void
client128() {
    cerr << "creating string, aes cbc client...";
    ope_client<string, aes_cbc> * ope_cl = new ope_client<string, aes_cbc>(bc_aes, is_malicious);
    cerr << "client created \n";	
    
    for (uint i = 0; i < vals.size(); i++) {
	cerr << "ENCRYPT " <<  i << "-th val: " << vals[i] << "\n";
	stringstream ss;
	ss << vals[i];
	cerr << "ope is " << ope_cl->encrypt(ss.str(), true) <<"\n";
    }
    cerr << "DONE!\n";	
}

static void
client_thread() {

    if (ciph_size == 64) {
	client64();
	return;
    }

    if (ciph_size >= 128) {
	client128();
	return;
    }

    assert_s(false, "not recognized ciphertext size; accepting 64, >= 128\n");
    return;
}

static void *
server_thread(void *s) {

    Server * serv = (Server *)s;
      
    serv->work();

    return NULL;
}

static void
runtest() {
           
    pid_t pid = fork();
    if (pid == 0) {
	// child
	client_thread();
	exit(EXIT_SUCCESS);
    }
    // parent
    
    sleep(2);
    
    pthread_t server_thd;
    cerr << "creating server ... ";
    Server * s = new Server(is_malicious);
    cerr << "server created \n";
    
    int r = pthread_create(&server_thd, NULL, server_thread, (void *)s);
    if (r) {
	cerr << "problem with thread create for server\n";
    }

    int status;

    pid_t pid2 = wait(&status);

    assert_s(WIFEXITED(status),"client terminated abnormally");
    assert_s(pid == pid2, "incorrect pid");

    if (ciph_size == 64) {
	check_correct<uint64_t, blowfish>(s, vals, bc);
    } else {
	assert_s(ciph_size >= 128, "invalid ciph size");
	check_correct<string, aes_cbc>(s, vals, bc_aes);
    }

    cerr << "OK!\n";
    delete s;
}
/*
static void
run_client(int num_tests) {
    
  cerr << "creating client...";
  ope_client<uint64_t, blowfish> * ope_cl = new ope_client<uint64_t, blowfish>(bc);
  cerr << " done \n";
  
  for (int i = 0; i < num_tests; i++) {
      ope_cl->encrypt(i, true);
  }

  cerr << "DONE!\n";
  
  delete ope_cl;

}
*/
static void
run_server() {
    Server * serv = new Server(is_malicious);
    
    serv->work();

    delete serv;
}
enum workload {INCREASING, RANDOM};

struct our_conf {
    vector<uint> num_elems;
    workload w;
    uint ciph_size;
    bool is_malicious;
};

struct bclo_conf {
    vector<uint> num_elems;
    workload w;
    uint ciph_size;
    bool use_cache;
};

static
void measure_ours(our_conf c) {
    // start client

    // fork and start server

    // prepare values to enc

    // encrypt and measure
}

static
void measure_bclo(bclo_conf c) {
    cerr << "unimplemented\n";
}


vector<our_conf> our_confs =
{// num_elems              workload       ciph_size    is_malicious
    {{10,100,1000},       INCREASING,      64,           false},
    {{10, 100, 1000},       RANDOM,        64,           false},
    {{10,100,1000},       INCREASING,      64,           true},
    {{10,100,1000},       RANDOM,          64,           true},
    {{10,100,1000},       INCREASING,      128,          false},
    {{10, 100, 1000},       RANDOM,        128,          false},
    {{10,100,1000},       INCREASING,      128,          true},
    {{10,100,1000},       RANDOM,          128,          true},
};


vector<bclo_conf> BCLO_confs =
{// num_elems               type          ciph_size       cache
    {{10,100,1000, 10000},
                            INCREASING,      32,           false},
    {{10,100,1000, 10000},
                            INCREASING,      64,          false},
    {{10,100,1000, 3000},
                            INCREASING,      128,          false},
    {{10,100,1000},
                            INCREASING,      256,          false},
};

static void
test_bench() {
    cerr << "Our scheme:\n";
    for (auto c : our_confs) {
	measure_ours(c);
    }
    
    cerr << "BCLO'09 scheme: \n";
    for (auto c: BCLO_confs) {
	measure_bclo(c);
    }
    cerr << "DONE\n";
}


int main(int argc, char ** argv)
{
    assert_s(OPE_MODE, "code must be in OPE_MODE to run this test");

    if (argc > 5 || argc < 2) {
	cerr << "usage ./test client ciph_size(64,>=128) num_tests is_malicious(0/1)\n \
                 OR ./test server is_malicious \
                 OR ./test sys ciph_size num_tests is_malicious \
                 OR ./test bench \n";
	return 0;
    }
    
    if (argc == 5 && string(argv[1]) == "client") {
	
	ciph_size = atoi(argv[2]);
	num_tests = atoi(argv[3]);
	is_malicious = atoi(argv[4]);
	vals = get_random(num_tests);

	client_thread();
	return 0;
    }
    if (argc == 3 && string(argv[1]) == "server") {
	is_malicious = atoi(argv[2]);
	run_server();
	return 0;
    }

    if (argc==5 && string(argv[1]) == "sys") {
	
	ciph_size = atoi(argv[2]);
	num_tests = atoi(argv[3]);
	is_malicious = atoi(argv[4]);
	vals = get_random(num_tests);

	runtest();
	return 0;
    }

    if (argc == 2 && string(argv[1]) == "bench") {
	test_bench();
	return 0;
    }
    
    cerr << "invalid test\n";
    cerr << "usage ./test client ciph_size(64,>=128) num_tests is_malicious(0/1)\n \
                 OR ./test server is_malicious \
                 OR ./test sys ciph_size num_tests is_malicious \
                 OR ./test bench \n";
    

    return 0;
}

 
