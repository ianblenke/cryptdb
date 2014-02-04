/*
 * Tests search trees.
 */

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include <util/net.hh>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <util/util.hh>
#include <ope/btree.hh>
#include <util/transform.hh>
#include <crypto/blowfish.hh>
#include <ope/server.hh>
#include <ope/client.hh>
#include <ope/tree.hh>
#include <ope/btree.hh>
#include <crypto/blowfish.hh>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <crypto/aes_cbc.hh>
#include <NTL/ZZ.h>
#include <csignal>
#include <utility>
#include <time.h>
#include <unistd.h>

using namespace std;
using boost::asio::ip::tcp;

static blowfish * bc = new blowfish(passwd);
static aes_cbc * bc_aes = new aes_cbc(passwd, true);

static uint plain_size = 0;
static uint num_tests = 0;
static bool db_rewrites = 1;

static const char* datadelim = "\n";

void * vals;
bool is_malicious;

enum workload {INCREASING, RANDOM};


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
check_order_and_values(OPETable<string> * ot, vector<V> & input_vals,
		       list<string> & tree_vals, BC * bc) {

    if (DEBUG_EXP) cerr << "-- check values\n";
    for (string tree_val : tree_vals) {
	V val = bc->decrypt(Cnv<V>::TypeFromStr(tree_val));
	assert_s(mcontains(input_vals, val), "val  not found in tree");
    }

    if (DEBUG_EXP) cerr << "-- check order in tree and ope table\n";
    // check tree vals are in order
    check_order<V, BC>(ot, tree_vals, bc);
}


template<class V, class BC>
static void
check_correct(Server * s, void * vals, BC * bc) {

    vector<V> vs = *((vector<V> *) vals);
    
    if (DEBUG_EXP) cerr << "checking sever state is correct: \n";
    //s->tracker->get_root()->check_merkle_tree();

    
    BTree * bt = (BTree *) s->ope_tree();

    list<string> treeorder;
    bt->tracker->get_root()->in_order_traverse(treeorder); 
    check_order_and_values<V, BC>(bt->opetable, vs, treeorder, bc);

     if (DEBUG_EXP) cerr <<"-- check height\n";
    uint max_height = bt->tracker->get_root()->max_height();
    cout << " -- max height of tree is " << max_height << " fanout " << b_max_keys << "\n";
    check_good_height(max_height, treeorder.size(), b_min_keys);

}


template<class A>
class WorkloadGen {
public:
    static A get_query(uint index, uint plain_size, workload w) {
	return (A) NULL;
    }
};

template<>
class WorkloadGen<string> {
public:
    static string get_query(uint index, uint plain_size, workload w) {
	if (w == INCREASING) {

        string s = "";
        if(plain_size > 64) {
	       s = strFromVal(index);
    	    string r;
    	    for (uint i = 0; i < plain_size/8 - s.size(); i++) {
    		r = r + " ";
    	    }
    	    s = r + s;
    	    assert_s(s.size() == plain_size/8, "logic error");
    	    return s;
        } else if (plain_size == 32) {
            uint num = index;
            while ( num > 0) {
                s = s + (char) ((int) num % 256);
                num = num/256;
            }

            assert_s(s.size() <= 4, "get_query32 bit oversized");
            string r;
            for (uint i = 0; i < plain_size/8 - s.size(); i++) {
            r = r + " ";
            }
            s = r + s;
        }

	}
	// random
	return randomBytes(plain_size/8);
    }
};

template<>
class WorkloadGen<uint64_t> {
public:
    static uint64_t get_query(uint index, uint plain_size, workload w) {
	assert_s(plain_size == 64,"logic error");
	if (w == INCREASING) {
	    return (uint64_t)index;
	} else {
	    return (uint64_t) rand();
	}
    }
};

template<>
class WorkloadGen<uint32_t> {
public:
    static uint32_t get_query(uint index, uint plain_size, workload w) {
	assert_s(plain_size == 32,"logic error");
	if (w == INCREASING) {
	    return (uint32_t)index;
	} else {
	    return (uint32_t) rand();
	}
    }
};

 
template<>
class WorkloadGen<NTL::ZZ> {
public:
    static NTL::ZZ get_query(uint index, uint plain_size, workload w) {
    string s;
    if (w == INCREASING) {
        s = strFromVal(index);
        string r;
        for (uint i = 0; i < plain_size/8 - s.size(); i++) {
	    r = r + " ";
        }
        s = r + s;
        assert_s(s.size() == plain_size/8, "logic error");
    } else {
        s = randomBytes(plain_size);
    }
    NTL::ZZ return_zz;
    for(uint i = 0; i < (uint) s.size(); i++){
            return_zz = return_zz * 256 + (int) s[i];
    }  

    return return_zz;
    }
};



template<class A>
static vector<A> *
get_uniq_wload(uint num, uint plain_size, workload w) {
    //   uint fails = 0;
    vector<A> * res = new vector<A>();
    for (uint i = 0; i < num; i++) {	    
	A q = WorkloadGen<A>::get_query(i, plain_size, w);
/*	while (mcontains(*res, q)) {
	    fails++;
	    if (fails > num/2) {
		    assert_s(false, "hard to get a workload of unique values, abandoning");
		    }
	    q = WorkloadGen<A>::get_query(i, plain_size, w);
	    }*/
	res->push_back(q);
    }
    return res;
}


static void
set_workload(uint num_tests, uint plain_size, workload w) {
    // generate workload
    if (plain_size == 64) {
    vals = (void *) get_uniq_wload<uint64_t>(num_tests, plain_size, w);
    
    } else {
    vals = (void *) get_uniq_wload<string>(num_tests, plain_size, w);
    }
    if (DEBUG_EXP) cerr << "set workload\n";
}


template<class A, class BC>
static void
clientgeneric(BC * bc) {


    if (DEBUG_EXP) cerr << "rewrites is " << db_rewrites << "\n";

    ope_client<A, BC> * ope_cl = new ope_client<A, BC>(bc, is_malicious);
    if (DEBUG_EXP) cerr << "client created \n";

    vector<A> * vs = (vector<A> * )vals;
    if (DEBUG_EXP) cerr << "SIZE of workload is " << vs->size() << "\n";

    cout << datadelim
         << "{ \"iv:scheme\": " << "\"mOPE\"" << ",\n"
         << "  \"iv:nelem\": " << vs->size() << ",\n"
         << "  \"iv:ptsize\": " << plain_size << ",\n"
         << "  \"iv:malicious\": " << is_malicious << ",\n"
         << "  \"iv:workload\": " <<  "\"sql_incr\"" << ",\n"
    	 << "  \"iv:dbrewrites\": " << "\"" << (db_rewrites==1 ? "yes" : "no") << "\"" << ",\n";
    
    Timer t;
    for (uint i = 0; i < vs->size(); i++) {
	ope_cl->encrypt(vs->at(i), true, "testope");
	// cerr << "ope is " << ope << " \n";
    }
    uint64_t time_interval = t.lap();
    cout << "  \"dv:enctime_ms\": " << (time_interval*1.0/(vs->size() *1000.0)) << "\n" << "}";
    delete ope_cl;
    if (DEBUG_EXP) cerr << "DONE!\n";	
}

static void
client_thread() {

    if (DEBUG_EXP) {cerr << "increasing\n";}
    time_t curtime = time(0);
    char timebuf[128];
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z", localtime(&curtime));

    struct utsname uts;
    uname(&uts);

      cout << "{ \"hostname\": \"" << uts.nodename << "\",\n"
         << "  \"username\": \"" << getenv("USER") << "\",\n"
         << "  \"start_time\": " << curtime << ",\n"
         << "  \"start_asctime\": \"" << timebuf << "\",\n"
         << "  \"data\": [";
  
    if (plain_size == 64) {
	if (DEBUG_EXP) cerr << "creating uint64, blowfish client\n";
	clientgeneric<uint64_t, blowfish>(bc);
	curtime = time(0);
	strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z", localtime(&curtime));
   
	cout << "],\n"
	     << "  \"end_time\": " << curtime << ",\n"
	     << "  \"end_asctime\": \"" << timebuf << "\"\n"
	     << "}\n";
	return;
	
    }

    //assert_s(false, "only 64 in this experiment");

    if (plain_size >= 128) {
	cerr << "creating string, aes cbc client\n";
	clientgeneric<string, aes_cbc>(bc_aes);
  return;	
    }

    
    
    
    assert_s(false, "not recognized ciphertext size; accepting 64, >= 128\n");
    return;
}

/*** Parallel client/server setup ***/


static void *
server_thread(void *s) {

    Server * serv = (Server *)s;
      
    serv->work();

    return NULL;
}

static void
runtest() {
    cout.flush();
    pid_t pid = fork();
    if (pid == 0) {
	// child
	client_thread();
	exit(EXIT_SUCCESS);
    }
    // parent

    cerr << "client pid is " << pid << "\n";
    
    sleep(1);
    
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

    cerr << "checking if server state is correct\n";
    if (plain_size == 64) {
	check_correct<uint64_t, blowfish>(s, vals, bc);
    } else {
	assert_s(plain_size >= 128, "invalid ciph size");
	check_correct<string, aes_cbc>(s, vals, bc_aes);
    }

    cerr << "number of rewrites is " << s->num_rewrites() << "\n";

    cerr << "OK!\n";
    delete s;
}

static void
run_server(bool db_updates) {

    if (DEBUG_EXP) cerr << "updates " << db_updates << "\n";

    list<string> * tables = new list<string>();
	if (DEBUG_EXP) cerr << "increasing\n";
	tables->push_back("testope");
    Server * serv = new Server(is_malicious, OPE_CLIENT_PORT, OPE_SERVER_PORT, tables, db_updates);
    
    serv->work();

    delete serv;
    delete tables;
}


struct our_conf {
    vector<uint> num_elems;
    workload w;
    uint plain_size;
    bool is_malicious;
    bool storage;
};

template<class A, class BC>
static void
client_work(uint n, our_conf c, BC * bc) {
    if (DEBUG_EXP) cerr << "creating client...";
    ope_client<A, BC> * ope_cl = new ope_client<A, BC>(bc, c.is_malicious);
    if (DEBUG_EXP) cerr << "client created \n";

    Timer t;
    for (uint i = 0; i < n; i++) {
	ope_cl->encrypt(WorkloadGen<A>::get_query(i, c.plain_size, c.w), true);
    }
    uint64_t time_interval = t.lap();
    cout << "  \"dv:enctime_ms\": " << (time_interval*1.0/(n *1000.0)) << ",\n";
    delete ope_cl;
     if (DEBUG_EXP) cerr << "DONE!\n";
}


static void
update_entries(OPETable<string> & ope_table, Node* curr_node, uint64_t v, uint64_t nbits){

    assert_s(nbits < (uint64_t) 64 - num_bits, "ciphertext too long!");

    uint64_t ope_enc;
    for (unsigned int i=1; i<curr_node->m_count; i++) {
        ope_enc = compute_ope(v, nbits, i-1);
        assert_s(ope_table.insert( curr_node->m_vector[i].m_key, ope_enc, curr_node), "inserted table value already existing!");

    }

    if(curr_node->is_leaf()){
      return;
    }

    for (unsigned int i=0; i<curr_node->m_count; i++) {
        if (curr_node->m_vector[i].has_subtree()) {  
            update_entries(ope_table, curr_node->m_vector[i].mp_subtree, 
              (v << num_bits) | i, 
              nbits+num_bits);
        }
    }

}

template<class A, class BC>
static void
measure_ours_instance(uint n, our_conf c, BC * bc) {

    cout << datadelim
         << "{ \"iv:scheme\": " << "\"mOPE\"" << ",\n"
         << "  \"iv:nelem\": " << n << ",\n"
         << "  \"iv:ptsize\": " << c.plain_size << ",\n"
         << "  \"iv:malicious\": " << c.is_malicious << ",\n"
         << "  \"iv:workload\": " << ((c.w == INCREASING) ? "\"increasing\""
                                                          : "\"random\"") << ",\n";

    // start client
    cout.flush();
    pid_t pid_client = fork();
    if (pid_client == 0) {
	//client
	client_work<A, BC>(n, c, bc);
	exit(EXIT_SUCCESS);
    }
    assert_s(pid_client > 0, "issue starting client");

    // cerr << "client pid is " << pid_client << "\n";
    sleep(1);

    pthread_t server_thd;
    if (DEBUG_EXP) cerr << "creating server ... ";
    Server * s = new Server(c.is_malicious);
     if (DEBUG_EXP) cerr << "server created \n";
    
    int r = pthread_create(&server_thd, NULL, server_thread, (void *)s);
    if (r) {
	cerr << "problem with thread create for server\n";
    }

    int status;
    pid_t pid2 = wait(&status);

    assert_s(WIFEXITED(status),"client terminated abnormally");
    assert_s(pid_client == pid2, "incorrect pid");

    if(c.storage){
        uint64_t dump_size = s->store_tree();
        cout << "  \"dv:storage_size\": " << dump_size << ",\n";        
    }
    cout << "  \"dv:rewrites_per_enc\": " << (s->num_rewrites() * 1.0)/(n*1.0) << "\n"
         << "}";
    datadelim = ",\n";

//    cerr << "checking server state..";
//    check_correct<A, BC>(s, vals, bc);
//    cerr << "OK.\n";

    delete s;
}

template <class A, class BC>
static
void measure_ours(our_conf c, BC * bc) {
    for (uint i = 0; i < c.num_elems.size(); i++) {
	measure_ours_instance<A, BC>(c.num_elems[i], c, bc);
    }

}

vector<our_conf> our_confs =
{// num_elems              workload       plain_size    is_malicious   storage
    {{10,100,1000,10000},
                            INCREASING,     64,         false,           true},
    {{10,100,1000,10000},
                            RANDOM,         64,         false,          true},
    {{10,100,1000,10000},
                            INCREASING,     64,         true,        true},
    {{10,100,1000,10000},
                            RANDOM,         64,         true,       true},
};

static void
test_bench() {
    time_t curtime = time(0);
    char timebuf[128];
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z", localtime(&curtime));

    struct utsname uts;
    uname(&uts);

    cout << "{ \"hostname\": \"" << uts.nodename << "\",\n"
         << "  \"username\": \"" << getenv("USER") << "\",\n"
         << "  \"start_time\": " << curtime << ",\n"
         << "  \"start_asctime\": \"" << timebuf << "\",\n"
         << "  \"data\": [";

    for (auto c : our_confs) {
    	if (c.plain_size == 64) {
    	    measure_ours<uint64_t, blowfish>(c, bc);
    	} else {
    	    measure_ours<string, aes_cbc>(c, bc_aes);
    	}
    }

    curtime = time(0);
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z", localtime(&curtime));

    cout << "],\n"
         << "  \"end_time\": " << curtime << ",\n"
         << "  \"end_asctime\": \"" << timebuf << "\"\n"
         << "}\n";
}

static bool
val_rewrites(string rewrites, string usage) {
    if (rewrites != "noup" && rewrites != "up") {
	cerr << "invalid rewrites option\n" << usage << "\n";
	exit(0);
    }
    if (rewrites == "up") {
	return true;
    } else {
	return false;
    }
}

int main(int argc, char ** argv)
{
  assert_s(OPE_MODE, "code must be in OPE_MODE to run this test");

  string usage = "usage ./test client plain_size(64,>=128) num_to_enc is_malicious(0/1) do_db_updates(up/noup)\n \
                 OR ./test server is_malicious do_db_updates(up/noup)\n			\
                 OR ./test sys plain_size num_tests is_malicious\n	\
                 OR ./test bench \n";

  if (argc < 2) {
    cerr << usage;
    return 0;
  }

  //This spins up just an mOPE client. Must be run before server starts!
  if (argc == 6 && string(argv[1]) == "client") {
	  plain_size = atoi(argv[2]);
  	num_tests = atoi(argv[3]);
  	if (DEBUG_EXP) cerr << "num_tests is " << num_tests << "\n";
  	is_malicious = atoi(argv[4]); // Are we in the mode where the server may be malicious?
  	db_rewrites = val_rewrites(string(argv[5]), usage); // Are we connected to a MySQL db? If so, we need updates.

    // Create a workload of num_tests elements of size plain_size, in increasing
    // order.
  	set_workload(num_tests, plain_size, INCREASING);
  	client_thread();
  	return 0;
  }
  
  //This spins up just an mOPE server. Must be run after client starts,
  //otherwise we get a network connection failure.
  if (argc == 4 && string(argv[1]) == "server") {
	  is_malicious = atoi(argv[2]); // Are we in the mode where the server may be malicious?
  	bool db_updates = val_rewrites(argv[3], usage); // Are we connect to a MySQL db? If so, we need updates.
  	run_server(db_updates);
  	return 0;
  }

  //This runs an mOPE client and server locally and does a simple random
  //workload, checking for correctness. Useful for quick system checks.
  if (argc==5 && string(argv[1]) == "sys") {
	
  	plain_size = atoi(argv[2]);
  	num_tests = atoi(argv[3]);
  	is_malicious = atoi(argv[4]);
  	set_workload(num_tests, plain_size, RANDOM);
  	runtest();

	  return 0;
  }

  // Run the benchmark workloads specified in our_confs struct (see above). 
  // Useful for measuring performance.
  if (argc == 2 && string(argv[1]) == "bench") {
  	test_bench();
	  return 0;
  }
   
  cerr << "invalid test \n";
  for(int i = 0; i < argc; i++ ){
    cerr << string(argv[i]) << " ";
  }
  cerr << endl;

  cerr << usage << "\n";
  return 0;
}

 
