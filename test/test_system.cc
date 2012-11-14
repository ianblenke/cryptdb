/*
 * Tests search trees.
 */

#include <stdlib.h>
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
#include <sys/utsname.h>
#include <crypto/aes_cbc.hh>
#include <crypto/ope.hh>
#include <NTL/ZZ.h>
#include <csignal>
#include <unistd.h>

using namespace std;


static blowfish * bc = new blowfish(passwd);
static aes_cbc * bc_aes = new aes_cbc(passwd, true);

static uint plain_size = 0;
static uint num_tests = 0;
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

    BTree * bt = (BTree *) s->ope_tree;

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
	    string s = strFromVal(index);
	    string r;
	    for (uint i = 0; i < plain_size/8 - s.size(); i++) {
		r = r + " ";
	    }
	    s = r + s;
	    assert_s(s.size() == plain_size/8, "logic error");
	    return s;
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


template<class A, class BC>
static void
clientgeneric(BC * bc) {

    ope_client<A, BC> * ope_cl = new ope_client<A, BC>(bc, is_malicious);
    cerr << "client created \n";

    vector<A> vs = *((vector<A> * )vals);

    for (uint i = 0; i < vs.size(); i++) {
	cerr << "ope is " << ope_cl->encrypt(vs[i], true) <<"\n";
    }
    cerr << "DONE!\n";	
}

static void
client_thread() {
    
    if (plain_size == 64) {
	cerr << "creating uint64, blowfish client\n";
	clientgeneric<uint64_t, blowfish>(bc);
	return;
    }

    if (plain_size >= 128) {
	cerr << "creating string, aes cbc client\n";
	clientgeneric<string, aes_cbc>(bc_aes);
	return;
    }

    assert_s(false, "not recognized ciphertext size; accepting 64, >= 128\n");
    return;
}

/*** Parallel client/server setup ***/

int rv;
int completed_enc = 0;
fstream clientfile;

struct timeval start_time;

template<class A, class BC>
static void
clientnetgeneric(BC * bc, int c_port, int s_port) {

    ope_client<A, BC> * ope_cl = new ope_client<A, BC>(bc, is_malicious, c_port, s_port);
    clientfile << "client created \n";

    vector<A> vs = *((vector<A> * )vals);
    pause();

    gettimeofday(&start_time, 0);
    for (uint i = 0; i < vs.size(); i++) {
        ope_cl->encrypt(vs[i], true);
        completed_enc++;
    }
    clientfile << "DONE with workload!\n";  
}

static void
clientnet_thread(int c_port, int s_port) {
    
    if (plain_size == 64) {
    //cerr << "creating uint64, blowfish client\n";
    clientnetgeneric<uint64_t, blowfish>(bc, c_port, s_port);
    return;
    }

    if (plain_size >= 128) {
    //cerr << "creating string, aes cbc client\n";
    clientnetgeneric<string, aes_cbc>(bc_aes, c_port, s_port);
    return;
    }

    assert_s(false, "not recognized ciphertext size; accepting 64, >= 128\n");
    return;
}

static void signalHandlerStart(int signum){
    clientfile <<"signalHandler"<<endl;
}

static void signalHandlerEnd(int signum){
    struct timeval end_time;
    gettimeofday(&end_time, 0);    

    float time_passed = end_time.tv_sec+end_time.tv_usec/((long) 1000000.0) - 
        start_time.tv_sec+start_time.tv_usec/((long) 1000.0);
//    clientfile << "completed: " << completed_enc << " in " << time_passed << endl;
    clientfile << "throughput: " << ( completed_enc*1.0 ) / (time_passed * 1.0) << endl;
    //clientfile << "start-end: "<< start_time.tv_sec+start_time.tv_usec/((long) 1000.0) << " " << end_time.tv_sec+end_time.tv_usec/((long) 1000000.0) <<endl;
    clientfile <<"ending"<<endl;
    clientfile.close();
    exit(rv);
}

static void parse_client_files(int num_clients){
    float total_throughput = 0;
    fstream throughput_f;
    throughput_f.open ("throughput.txt", std::ios::out | std::ios::app);
    for (int i=0; i< num_clients; i++) {
            
            stringstream ss;
            ss<<i;
            string filename = "client"+ss.str()+".txt";
            ifstream readfile (filename);
            if (readfile.is_open()) {
                string line;
                while (readfile.good() ){
                    getline(readfile, line);
                    ss.clear();
                    ss.str("");
                    ss << line;
                    string first;
                    ss >> first;
                    if (first == "throughput:") {
                        float indiv_throughput = 0;
                        ss >> indiv_throughput;
                        total_throughput += indiv_throughput;
                        break;
                    }

                }
                readfile.close();
            }

    }  
    throughput_f << num_clients << " " << total_throughput << endl;
    throughput_f.close();
}

static void
client_net(int num_clients){
    signal(SIGALRM, signalHandlerStart);
    signal(SIGINT, signalHandlerEnd);

    vector<pid_t> pid_list;
    for (int i=0; i< num_clients; i++) {
            pid_t pid = fork();
            if(pid == 0){
                    stringstream ss;
                    ss<<i;
                    string filename = "client"+ss.str()+".txt";
                    clientfile.open (filename.c_str(), std::ios::in | std::ios::out | std::ios::trunc);
                    clientnet_thread(1110+i, 1110+i);
                    assert_s(false, "Should have exited with signalEnd");
                    exit(rv);
            }else{
//                    cout<<"pid: "<<pid<<endl;
                    pid_list.push_back(pid);
            }
    }
    cout << pid_list.size() << " clients ready, starting servers " << endl;
    stringstream parse_num;
    parse_num << num_clients;
    string cmd = "cd /; ./home/frankli/cryptdb/obj/test/test servernet "+parse_num.str();
    string ssh = "ssh root@ud0.csail.mit.edu '" + cmd + "'";
    sleep(2);
    pid_t pid = fork();
    if(pid == 0) {
        exit( system(ssh.c_str()) );    
    }
    sleep(2);
    for(uint i = 0; i < pid_list.size(); i++){
            kill(pid_list[i], SIGALRM);
    }
    sleep(10);
    for(uint i = 0; i < pid_list.size(); i++){
            kill(pid_list[i], SIGINT);
    }

    int t= pid_list.size();
    while(t>0){
            wait(&rv);
            t--;
            cout<<"Client closed, " << t << " remaining threads" <<endl;
    }
    cout << "All clients closed" << endl;

    parse_client_files(num_clients);

}

static void
servernet_thread(int c_port, int s_port){
    Server * serv = new Server(is_malicious, c_port, s_port);
    
    serv->work();
}

static void signalHandlerTerminate(int signum){
    exit(rv);
}

static void
server_net(int num_servers){
    signal(SIGTERM, signalHandlerTerminate);

    vector<pid_t> pid_list;
    for (int i=0; i< num_servers; i++) {
            pid_t pid = fork();
            if(pid == 0){
                    servernet_thread(1110+i, 1110+i);
                    assert_s(false, "Should have exited with signalEnd");
                    exit(rv);
            }else{
                    //cout<<"pid: "<<pid<<endl;
                    pid_list.push_back(pid);
            }
    }
    cout << pid_list.size() << " servers started. " << endl;

    int t= pid_list.size();
    while(t>0){
            wait(&rv);
            t--;
            cout<<"Server closed, " << t << " remaining threads" <<endl;            
    }

}


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


struct our_conf {
    vector<uint> num_elems;
    workload w;
    uint plain_size;
    bool is_malicious;
};

struct bclo_conf {
    vector<uint> num_elems;
    workload w;
    uint plain_size;
    bool use_cache;
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

     if (DEBUG_EXP) cerr << "DONE!\n";
}

static const char* datadelim = "\n";

template<class A, class BC>
static void
measure_ours_instance(uint n, our_conf c, BC * bc) {

    cout << datadelim
         << "{ \"iv:scheme\": " << "\"stOPE\"" << ",\n"
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

template <class A>
static void
measure_bclo_instance(uint n, bclo_conf c) {

    cout << datadelim
         << "{ \"iv:scheme\": " << "\"bclo\"" << ",\n"
         << "  \"iv:nelem\": " << n << ",\n"
         << "  \"iv:ptsize\": " << c.plain_size << ",\n"
         << "  \"iv:ctsize\": " << c.plain_size*2 << ",\n"
         << "  \"iv:cached\": " << c.use_cache << ",\n"
         << "  \"iv:workload\": " << ((c.w == INCREASING) ? "\"increasing\""
                                                          : "\"random\"") << ",\n";

    OPE* ope = new OPE(passwd, c.plain_size, c.plain_size * 2, c.use_cache);
    NTL::ZZ ope_enc, pt;

    Timer t;
    for (uint i = 0; i < n; i++) {
        A plaintext = WorkloadGen<A>::get_query(i, c.plain_size, c.w);
        pt = plaintext;
        ope_enc = ope->encrypt(pt);

        //Do not want this assert during actual timing. Comment out.
        //assert_s(ope->decrypt(ope_enc) == pt, "encryption for int not right");

        //Chaining to ensure compile doesn't optimize out some loops
        if (ope_enc == NTL::to_ZZ(0) ) {
            cerr << "ope_enc is 0 in bclo";
            assert_s(ope->decrypt(ope_enc) == pt, "encryption for int not right");
        }
    }
    uint64_t time_interval = t.lap();
    cout << "  \"dv:enctime_ms\": " << (time_interval*1.0/(n *1000.0)) << "\n"
         << "}";
    datadelim = ",\n";
    cout.flush();
}

template <class A>
static
void measure_bclo(bclo_conf c) {
    for(uint i = 0; i < c.num_elems.size(); i++){
        measure_bclo_instance<A>(c.num_elems[i], c);
    }
}


vector<our_conf> our_confs =
{// num_elems              workload       plain_size    is_malicious
    {{10,100,1000,10000},
                            INCREASING,     32,         false},
    {{10,100,1000,10000},
                            RANDOM,         32,         false},
    {{10,100,1000,10000},
                            INCREASING,     32,         true},
    {{10,100,1000,10000},
                            RANDOM,         32,         true},
    {{10,100,1000,10000},
                            INCREASING,     64,         false},
    {{10,100,1000,10000},
                            RANDOM,         64,         false},
    {{10,100,1000,10000},
                            INCREASING,     64,         true},
    {{10,100,1000,10000},
                            RANDOM,         64,         true},
    {{10,100,1000,10000},
                            INCREASING,     128,        false},
    {{10,100,1000,10000},
                            RANDOM,         128,        false},
    {{10,100,1000,10000},
                            INCREASING,     128,        true},
    {{10,100,1000,10000},
                            RANDOM,         128,        true},
    {{10,100,1000,10000},
                            INCREASING,     256,        false},
    {{10,100,1000,10000},
                            RANDOM,         256,        false},
    {{10,100,1000,10000},
                            INCREASING,     256,        true},
    {{10,100,1000,10000},
                            RANDOM,         256,        true},
};


vector<bclo_conf> BCLO_confs =
{// num_elems               type          plain_size       cache
    {{10,100,1000},
                            INCREASING,      32,           false},
    {{10,100,1000,10000,100000},
                            INCREASING,      32,           true},
    {{10,100,1000},
                            INCREASING,      64,           false},
    {{10,100,1000,10000,100000},
                            INCREASING,      64,           true},
    {{10,100,1000},
                            INCREASING,      128,          false},
    {{10,100,1000,10000},
                            INCREASING,      128,          true},
    {{10,100,1000},
                            INCREASING,      256,          false},
    {{10,100,1000,10000},
                            INCREASING,      256,          true},
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
    
    for (auto c: BCLO_confs) {
        if (c.plain_size == 32) {
            measure_bclo<uint32_t>(c);
        } else if (c.plain_size == 64) {
            measure_bclo<uint64_t>(c);
        } else {
            measure_bclo<NTL::ZZ>(c);
        }	   
    }

    curtime = time(0);
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z", localtime(&curtime));

    cout << "],\n"
         << "  \"end_time\": " << curtime << ",\n"
         << "  \"end_asctime\": \"" << timebuf << "\"\n"
         << "}\n";
}

static void
set_workload(uint num_tests, uint plain_size, workload w) {
    // generate workload
    cerr << "preparing workload: \n";
    if (plain_size == 64) {
	vals = (void *) get_uniq_wload<string>(num_tests, plain_size, w);
    } else {
	vals = (void *) get_uniq_wload<string>(num_tests, plain_size, w);
    }
    cerr << "set workload\n";
}

int client_load []= {1, 10, 20};

int main(int argc, char ** argv)
{
    assert_s(OPE_MODE, "code must be in OPE_MODE to run this test");

    if (argc > 5 || argc < 2) {
	cerr << "usage ./test client plain_size(64,>=128) num_to_enc is_malicious(0/1)\n \
                 OR ./test server is_malicious\n \
                 OR ./test sys plain_size num_tests is_malicious\n \
                 OR ./test bench \n \
                 OR ./test net plain_size\n \
                 OR ./test clientnet plain_size(64,>=128) num_clients \n \
                 OR ./test servernet num_servers";
	return 0;
    }

    if (argc == 3 && string(argv[1]) == "net") {
        plain_size = atoi(argv[2]);
        is_malicious = 0;
        set_workload(100000, plain_size, INCREASING);

        for (int i=0; i < sizeof(client_load)/sizeof(int); i++) {
            client_net(client_load[i]);
        }
    }

    if (argc == 4 && string(argv[1]) == "clientnet") {
    
    plain_size = atoi(argv[2]);
    int num_clients = atoi(argv[3]);
    is_malicious = 0;
    set_workload(100000, plain_size, INCREASING);

    client_net(num_clients);

    return 0;
    }

    if (argc == 3 && string(argv[1]) == "servernet") {
    
    int num_servers = atoi(argv[2]);
    is_malicious = 0;

    server_net(num_servers);

    return 0;
    }    
    
    if (argc == 5 && string(argv[1]) == "client") {
	
	plain_size = atoi(argv[2]);
	num_tests = atoi(argv[3]);
	is_malicious = atoi(argv[4]);
	set_workload(num_tests, plain_size, RANDOM);

	client_thread();

	return 0;
    }
    if (argc == 3 && string(argv[1]) == "server") {
	is_malicious = atoi(argv[2]);
	run_server();
	return 0;
    }

    if (argc==5 && string(argv[1]) == "sys") {
	
	plain_size = atoi(argv[2]);
	num_tests = atoi(argv[3]);
	is_malicious = atoi(argv[4]);
	set_workload(num_tests, plain_size, RANDOM);
	
	runtest();

	return 0;
    }

    if (argc == 2 && string(argv[1]) == "bench") {
	test_bench();
	return 0;
    }
    
    cerr << "invalid test\n";
    cerr << "usage ./test client plain_size(64,>=128) num_to_enc is_malicious(0/1)\n \
                 OR ./test server is_malicious\n \
                 OR ./test sys plain_size num_tests is_malicious\n \
                 OR ./test bench \n \
                 OR ./test net \n \
                 OR ./test clientnet plain_size(64,>=128) num_clients \n \
                 OR ./test servernet num_servers";
    

    return 0;
}

 
