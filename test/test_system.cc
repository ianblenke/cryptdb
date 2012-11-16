/*
 * Tests search trees.
 */

#include <boost/asio.hpp>
#include <boost/array.hpp>

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
#include <crypto/ope.hh>
#include <NTL/ZZ.h>
#include <csignal>
#include <utility>
#include <time.h>
#include <unistd.h>

#define port_start 25566

using namespace std;
using boost::asio::ip::tcp;

static blowfish * bc = new blowfish(passwd);
static aes_cbc * bc_aes = new aes_cbc(passwd, true);

static uint net_workload = 1000;

static uint plain_size = 0;
static uint num_tests = 0;

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
    cerr << "given num " << num << " result is " << res->size() << "\n";
    return res;
}

static void
set_workload(uint num_tests, uint plain_size, workload w) {
    // generate workload
    cerr << "preparing workload: \n";
    if (plain_size == 64) {
    vals = (void *) get_uniq_wload<uint64_t>(num_tests, plain_size, w);
    
    } else {
    vals = (void *) get_uniq_wload<string>(num_tests, plain_size, w);
    }
    cerr << "set workload\n";
}


template<class A, class BC>
static void
clientgeneric(BC * bc) {

    ope_client<A, BC> * ope_cl = new ope_client<A, BC>(bc, is_malicious);
    cerr << "client created \n";

    vector<A> * vs = (vector<A> * )vals;
    cerr << "SIZE of workload is " << vs->size() << "\n";

    for (uint i = 0; i < vs->size(); i++) {
	cerr << "ope is " << ope_cl->encrypt(vs->at(i), true) <<"\n";
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

template<class A, class BC>
static void
clientnetgeneric(BC * bc, int c_port, int s_port) {

    ope_client<A, BC> * ope_cl = new ope_client<A, BC>(bc, is_malicious, c_port, s_port);
    vector<A> * vs = (vector<A> * )vals;

    pause();
    for (uint i = 0; i < vs->size(); i++) {
        ope_cl->encrypt(vs->at(i), true);
    }
    delete ope_cl;
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


static void
signalHandlerStart(int signum) {
    return;
}

static void
client_net(int num_clients){
    signal(SIGALRM, signalHandlerStart);
    
    if(system("ssh -A root@ud1.csail.mit.edu 'killall -9 test' >> trash_client 2>&1") < 0)
            perror("ssh error on server");

    vector<pid_t> pid_list;
    for (int i=0; i< num_clients; i++) {
            pid_t pid = fork();
            if(pid == 0){
                    clientnet_thread(port_start+i, port_start+i);
                    exit(rv);
            }else{
                    assert_s(pid > 0, "error in client child fork");
//                    cout<<"pid: "<<pid<<endl;
                    pid_list.push_back(pid);
            }
    }
    cout << pid_list.size() << " clients ready, starting servers " << endl;
    stringstream parse_num;
    parse_num << num_clients;
    string cmd = "cd /; ./home/frankli/cryptdb/obj/test/test servernet "+parse_num.str();
    string ssh = "ssh -A root@ud1.csail.mit.edu '" + cmd + "' >> trash_client 2>&1";

    int sleep_rv;

    pid_t pid = fork();
    if(pid == 0) {
        if(system(ssh.c_str()) < 0)
            perror("ssh error on server");
        exit(sleep_rv);    
    }
    sleep(5);

    struct timeval begin_net_time;
    struct timeval end_net_time;

    gettimeofday(&begin_net_time, 0);    
    for(uint i = 0; i < pid_list.size(); i++){
            kill(pid_list[i], SIGALRM);
    }

    int t= pid_list.size();
    while(t>0){
            wait(&rv);
            t--;
    }
    gettimeofday(&end_net_time, 0);

    float time_passed = end_net_time.tv_sec - begin_net_time.tv_sec + \
        (end_net_time.tv_usec*1.0)/(1000000.0) - (begin_net_time.tv_usec*1.0)/(1000000.0);

    fstream throughput_f;
    throughput_f.open ("throughput.txt", ios::out | ios::app);
    throughput_f << "  \"dv:enctime_ms\": " << (time_passed*1000.0)/(num_clients * net_workload *1.0) << ",\n";
    throughput_f << "}";
    datadelim = ",\n";
    throughput_f.close();

    wait(&sleep_rv);
    if(DEBUG) cout << "All clients closed" << endl;
}

static void
servernet_thread(int c_port, int s_port){
    Server * serv = new Server(is_malicious, c_port, s_port);
    
    serv->work();
    exit(rv);
}

static void
server_net(int num_servers){

    vector<pid_t> pid_list;
    for (int i=0; i< num_servers; i++) {
            pid_t pid = fork();
            if(pid == 0){
                    servernet_thread(port_start+i, port_start+i);
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
            if(DEBUG) cout<<"Server closed, " << t << " remaining threads" <<endl;            
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

struct net_conf {
    uint trials;
    uint plain_size;
    workload w;
    vector<uint> num_threads;
    vector<uint> latencies;
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


static void
update_table(OPETable<string> & ope_table, Node* btree){

  update_entries(ope_table, btree, 0, 0);

}

static void 
parse_int_message(stringstream & ss, vector<string> & unique_data, vector<string> & bulk_data){

    string first;
    string last = "";

    while(true){
      ss >> first;
      if(first=="EOF") break;

      stringstream stoi;      

      bulk_data.push_back( first );
      if (first == last) {
        continue;
      } else{
        last = first;
        unique_data.push_back(first);
      }
    }    
}

static void 
parse_string_message(stringstream & ss, vector<string> & unique_data, vector<string> & bulk_data){

    string len_str;
    string last = "";

    int counter = 0;
    while(true){
      counter++;
      ss >> len_str;
      if(len_str=="EOF") break;

      stringstream stoi;
      int len;
      stoi << len_str;
      stoi >> len;

      ss.get();

      char data[len];
      for(int i=0 ; i < len; i++){
              ss.get(data[i]);
      }

      string str_data (data, len);

      bulk_data.push_back( str_data );
      if ( str_data == last) {
        continue;
      } else{
        last = str_data;
        unique_data.push_back(str_data);
       }

    }

}

template <class A>
static void
server_bulk_work(our_conf c, uint n)
{
      try
      {
        boost::asio::io_service io_service;

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 9913));

        tcp::socket socket(io_service);
        acceptor.accept(socket);
        
        string output = "";
        stringstream ss (output, stringstream::in | stringstream::out);
        for(;;)
        {

          boost::array<char, 128> buf;
          boost::system::error_code error;

          size_t len = socket.read_some(boost::asio::buffer(buf), error);

          if (error == boost::asio::error::eof){
            //cout<<"EOF signalled"<<endl;
            break; // Connection closed cleanly by peer.
          }else if (error)
            throw boost::system::system_error(error); // Some other error.

          ss.write(buf.data(), len);

        }
        ss << " EOF EOF";

        vector<string> unique_data;
        vector<string> bulk_data;

        if (c.plain_size == 32 || c.plain_size == 64)
            parse_int_message(ss, unique_data, bulk_data);
        else
            parse_string_message(ss, unique_data, bulk_data);

        RootTracker* root_tracker = new RootTracker(true);

        Node* b_tree = build_tree_wrapper(unique_data, root_tracker, 0, unique_data.size());

        /*vector<uint64_t> db_data;
        db_data.resize( bulk_data.size());*/

        OPETable<string >* ope_lookup_table = new OPETable<string>();
        update_table(*ope_lookup_table, b_tree);    
       /* for(int j = 0; j < (int) bulk_data.size(); j++){
            string key = bulk_data[j];
            db_data[j] = ope_lookup_table->get(key).ope;
        }*/
        struct timeval bulk_end_time;
        gettimeofday(&bulk_end_time, 0);

        if (!c.is_malicious) {
            sleep(2);
            ifstream f;
            f.open("bulk.txt");        
            double start;
            stringstream timess;
            if (f.is_open()) {
                string line;

                getline(f, line);
                timess << line;
                timess >> start;
               // cout << line << endl;
                f.close();
            }
            double total_runtime = bulk_end_time.tv_sec + bulk_end_time.tv_usec/1000000.0 - start;
            //cout << total_runtime << " : " << bulk_end_time.tv_sec <<"." << bulk_end_time.tv_usec/1000000.0<<endl;
            double enctime_ms = (total_runtime*1.0)/(n*1.0);
            cout<< "  \"dv:enctime_ms\": " << enctime_ms*1000.0 << endl;
        }else{
            fstream f;
            f.open("bulk.txt", ios::out | ios::trunc);
            stringstream timess;
            string seconds;
            timess <<  bulk_end_time.tv_usec/1000000.0;
            timess >> seconds;
            f << bulk_end_time.tv_sec << seconds.substr(1, seconds.size()-1)  << endl;
            f.close();        
        }


        delete ope_lookup_table;
        delete b_tree;
        delete root_tracker;

    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
    }
}


template<class A>
static void
client_bulk_work(uint n, our_conf c) {
  
    vector<uint32_t > values_in_db32;
    vector<uint64_t > values_in_db64;
    vector<string > strings_in_db;

    void* det_bc = NULL;

    if (c.plain_size == 32){
        det_bc = (blowfish *) new blowfish(passwd);
        values_in_db32.resize(n);
    } else if (c.plain_size == 64 ) {
        det_bc = (blowfish *) new blowfish(passwd);
        values_in_db64.resize(n);
    }else {
        strings_in_db.resize(n);
    }

    AES_KEY * aes_key = get_AES_enc_key(passwd);

    for ( uint rc=0; rc < n; rc++){
        std::stringstream ss;
        ss << WorkloadGen<A>::get_query(rc, c.plain_size, c.w);

        if(c.plain_size == 32 ){
            uint32_t cur_val = 0;
            ss >> cur_val;
            values_in_db32[rc] =cur_val;    
        }else if (c.plain_size == 64) {
            uint64_t cur_val = 0;
            ss >> cur_val;
            values_in_db64[rc] =cur_val;            
        } else {
            std::string cur_val = ss.str();
            std::transform( cur_val.begin(), cur_val.end(), cur_val.begin(), ::tolower);
            strings_in_db[rc] = cur_val;
        }

    }

    struct timeval bulk_start_time;
    gettimeofday(&bulk_start_time, 0);

    //cout << bulk_start_time.tv_sec <<"."<<bulk_start_time.tv_usec<<endl;

    if(c.plain_size == 32 ){
        std::stable_sort(values_in_db32.begin(), values_in_db32.end() );  
    }else if (c.plain_size == 64) {
        std::stable_sort(values_in_db64.begin(), values_in_db64.end() );         
    } else {
        std::stable_sort(strings_in_db.begin(), strings_in_db.end() );
    }

    std::string message = "";

    vector<string> unique_data;
    string last = "";

    for (uint rc = 0; rc < n; rc++) {
        std::stringstream ss;

        if(c.plain_size == 32 ){
            uint32_t det;
            ( (blowfish *) det_bc)->block_encrypt( &values_in_db32[rc] , &det);
            ss << det;   
            if (c.is_malicious && ss.str() != last) {
                last = ss.str();
                unique_data.push_back(last);
            }
        }else if (c.plain_size == 64) {
            uint64_t det;
            ( (blowfish *) det_bc)->block_encrypt( &values_in_db64[rc] , &det);
            ss << det;    
            if (c.is_malicious && ss.str() != last) {
                last = ss.str();
                unique_data.push_back(last);
            }              
        } else {
            std::string det = encrypt_AES_CBC(strings_in_db[rc], aes_key, "0");
            ss << det.size() << " " << det;
            if (c.is_malicious && det != last) {
                last = det;
                unique_data.push_back(det);
            }            
        }
        message += ss.str() + " ";
    }

    try
    {

        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query("localhost", "9913");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        tcp::socket socket(io_service);
        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpoint_iterator != end)
        {
          socket.close();
          socket.connect(*endpoint_iterator++, error);
        }
        if (error)
          throw boost::system::system_error(error);

        //cout <<"Sending message "<<message<<endl;
        boost::system::error_code ignored_error;
        boost::asio::write(socket, boost::asio::buffer(message),
            boost::asio::transfer_all(), ignored_error);

    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
    }
    if (!c.is_malicious) {
        fstream f;
        f.open("bulk.txt",ios::in | ios::out | ios::trunc);
        stringstream timess;
        string seconds;
        timess <<  bulk_start_time.tv_usec/1000000.0;
        timess >> seconds;
        f << bulk_start_time.tv_sec << seconds.substr(1, seconds.size()-1)  << endl;
        f.close();
        return;
    }

    RootTracker* root_tracker = new RootTracker(true);   
   
    Node* b_tree = build_tree_wrapper(unique_data, root_tracker, 0, unique_data.size());

    b_tree->compute_merkle_subtree();

    struct timeval bulk_end_time;
    gettimeofday(&bulk_end_time, 0);    

    sleep(5);
    ifstream f;
    f.open("bulk.txt");        
    double server_end;
    stringstream timess;
    if (f.is_open()) {
        string line;

        getline(f, line);
        timess << line;
        timess >> server_end;

        f.close();
    }
    double b_start_time = bulk_start_time.tv_sec + bulk_start_time.tv_usec/1000000.0;

    double client_runtime = bulk_end_time.tv_sec + bulk_end_time.tv_usec/1000000.0 - b_start_time;
    double server_runtime = server_end - b_start_time;
/*    if ( client_runtime > server_runtime) cout << "CLIENT" << endl;
    else cout << "SERVER" <<endl;*/
    double enctime_ms = (max(client_runtime, server_runtime)*1.0)/(n*1.0);
    cout<< "  \"dv:enctime_ms\": " << enctime_ms*1000.0 << endl;    

    delete b_tree;
}


template<class A>
static void
measure_bulk_instance(uint n, our_conf c) {

    cout << datadelim
         << "{ \"iv:scheme\": " << "\"stOPE\"" << ",\n"
         << "  \"iv:nelem\": " << n << ",\n"
         << "  \"iv:ptsize\": " << c.plain_size << ",\n"
         << "  \"iv:malicious\": " << c.is_malicious << ",\n"
         << "  \"iv:workload\": " << ((c.w == INCREASING) ? "\"bulk-increasing\""
                                                          : "\"bulk-random\"") << ",\n";

    int killsig;

    // start client
    cout.flush();
    pid_t pid_server = fork();
    if (pid_server == 0) {
    //client
        server_bulk_work<A>(c, n);
        exit(killsig);
    }
    assert_s(pid_server > 0, "issue starting bulk server");

    // cerr << "client pid is " << pid_client << "\n";
    sleep(1);

    pid_t pid_client = fork();
    if (pid_client == 0) {
        client_bulk_work<A>(n, c);
        exit(killsig);
    }
    int t= 2;
    while(t>0){
            wait(&killsig);
            t--;
    }
    cout << "}";
    datadelim = ",\n";
    cout.flush();

}

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
static
void measure_ours_bulk(our_conf c) {
    for (uint i = 0; i < c.num_elems.size(); i++) {
        measure_bulk_instance<A>(c.num_elems[i], c);
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

static void measure_net_instance(net_conf c) {
    fstream throughput_f;
    set_workload(net_workload, c.plain_size, c.w);
    plain_size = c.plain_size;
    is_malicious = 0;
    for(uint j = 0; j < c.latencies.size(); j++){
        stringstream ss;
        ss << "ssh root@ud1.csail.mit.edu ' tc qdisc add dev eth0 root netem delay " << c.latencies[j] << "ms'";

        if (system(ss.str().c_str()) < 0)
            perror("tc add failed");

        for(uint i = 0; i < c.num_threads.size(); i++) {
            uint num_clients = c.num_threads[i];           
            for(uint trial = 0; trial < c.trials; trial++) {

                throughput_f.open ("throughput.txt", ios::out | ios::app);
                throughput_f << datadelim
                     << "{ \"iv:scheme\": " << "\"network\"" << ",\n"
                     << "  \"iv:latency\": " << c.latencies[j] << ",\n"
                     << "  \"iv:ptsize\": " << c.plain_size << ",\n"
                     << "  \"iv:numthd\": " << num_clients << ",\n"
                     << "  \"iv:trialnum\": " << trial << ",\n"
                     << "  \"iv:workload\": " << ((c.w == INCREASING) ? "\"increasing\""
                                                                      : "\"random\"") << ",\n";
                throughput_f.close(); 

                client_net(num_clients);             
            }


        }
        if (system("ssh root@ud1.csail.mit.edu ' tc qdisc del dev eth0 root'") < 0)
            perror("tc del failed");   

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

vector<our_conf> bulk_confs =
{//    num_elems             workload  plain_size    is_malicious
    {{10, 100, 1000, 10000}, INCREASING,    32,         false},
    {{10, 100, 1000, 10000}, INCREASING,    64,         false},
    {{10, 100, 1000, 10000}, INCREASING,    128,        false},
    {{10, 100, 1000, 10000}, INCREASING,    256,        false},
    {{10, 100, 1000, 10000}, RANDOM,        32,         false},
    {{10, 100, 1000, 10000}, RANDOM,        64,         false},
    {{10, 100, 1000, 10000}, RANDOM,        128,        false},
    {{10, 100, 1000, 10000}, RANDOM,        256,        false},
    {{10, 100, 1000, 10000}, INCREASING,    32,         true},
    {{10, 100, 1000, 10000}, INCREASING,    64,         true},
    {{10, 100, 1000, 10000}, INCREASING,    128,        true},
    {{10, 100, 1000, 10000}, INCREASING,    256,        true},
    {{10, 100, 1000, 10000}, RANDOM,        32,         true},
    {{10, 100, 1000, 10000}, RANDOM,        64,         true},
    {{10, 100, 1000, 10000}, RANDOM,        128,        true},
    {{10, 100, 1000, 10000}, RANDOM,        256,        true},    
};

vector<bclo_conf> BCLO_confs =
{// num_elems               type          plain_size       cache
    {{10,100,1000,10000},
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

vector<net_conf> net_confs = 
{// trials  plain_size  workload            num_threads                             latencies
    {3,     64,         INCREASING, {1, 10, 50, 100, 200, 500, 750, 1000, 2000},     {0, 5, 10, 15, 20, 25}},
    {3,     64,         RANDOM,     {1, 10, 50, 100, 200, 500, 750, 1000, 2000},     {0, 5, 10, 15, 20, 25}}
};

static 
void measure_net () {
    time_t curtime = time(0);
    char timebuf[128];
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z", localtime(&curtime));

    struct utsname uts;
    uname(&uts);

    fstream throughput_f;
    throughput_f.open ("throughput.txt", ios::out | ios::trunc);
    throughput_f << "{ \"hostname\": \"" << uts.nodename << "\",\n"
         << "  \"username\": \"" << getenv("USER") << "\",\n"
         << "  \"start_time\": " << curtime << ",\n"
         << "  \"start_asctime\": \"" << timebuf << "\",\n"
         << "  \"data\": [";
    throughput_f.close(); 

    for (auto c: net_confs) {
        measure_net_instance(c);
    }

    curtime = time(0);
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %T %z", localtime(&curtime));
    throughput_f.open ("throughput.txt", ios::out | ios::app);
    throughput_f << "],\n"
         << "  \"end_time\": " << curtime << ",\n"
         << "  \"end_asctime\": \"" << timebuf << "\"\n"
         << "}\n";
    throughput_f.close();

    return;

}

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

    for (auto c: bulk_confs) {
        if (c.plain_size == 32) {
            measure_ours_bulk<uint32_t>(c);
        } else if (c.plain_size == 64) {
            measure_ours_bulk<uint64_t>(c);
        }else {
            measure_ours_bulk<string>(c);
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

int main(int argc, char ** argv)
{
    assert_s(OPE_MODE, "code must be in OPE_MODE to run this test");

    string usage = "usage ./test client plain_size(64,>=128) num_to_enc is_malicious(0/1)\n \
                 OR ./test server is_malicious\n			\
                 OR ./test sys plain_size num_tests is_malicious\n	\
                 OR ./test bench \n	\
                 OR ./test net \n \
                 OR ./test net plain_size [num_client1] ... \n		\
                 OR ./test clientnet plain_size(64,>=128) num_clients \n \
                 OR ./test servernet num_servers";
    
    if (argc < 2) {
	cerr << usage;
	return 0;
    }

    if (argc == 2 && string(argv[1]) == "net") {
        measure_net();
        return 0;

    }

    if (argc > 3 && string(argv[1]) == "net") {
        plain_size = atoi(argv[2]);
        is_malicious = 0;
        set_workload(1000000, plain_size, INCREASING);

        stringstream ss;

        for(int i = 0; i < argc - 3; i++) {
            int num_clients;
            ss << string(argv[3+i]);
            ss >> num_clients;
            client_net(num_clients);
            ss.clear();
            ss.str("");
            cout << "Done with " << num_clients << " clients." << endl;
        }
        return 0;
    }

    if (argc == 4 && string(argv[1]) == "clientnet") {
    
    plain_size = atoi(argv[2]);
    int num_clients = atoi(argv[3]);
    is_malicious = 0;
    set_workload(1000000, plain_size, INCREASING);

    client_net(num_clients);

    return 0;
    }

    if (argc == 3 && string(argv[1]) == "servernet") {
    
    int num_servers = atoi(argv[2]);
    is_malicious = 0;
    plain_size = 64;

    server_net(num_servers);

    return 0;
    }    
    
    if (argc == 5 && string(argv[1]) == "client") {
	
	plain_size = atoi(argv[2]);
	num_tests = atoi(argv[3]);
	cerr << "num_tests is " << num_tests << "\n";
	is_malicious = atoi(argv[4]);
	set_workload(num_tests, plain_size, INCREASING);
	
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
    
    cerr << "invalid test \n";

    for(int i = 0; i < argc; i++ ){
        cerr << string(argv[i]) << " ";
    }
    cerr << endl;

    cerr << "usage ./test client plain_size(64,>=128) num_to_enc is_malicious(0/1)\n \
                 OR ./test server is_malicious\n \
                 OR ./test sys plain_size num_tests is_malicious\n \
                 OR ./test bench \n \
                 OR ./test net \n   \
                 OR ./test net plain_size [num_client1] ... \n \
                 OR ./test clientnet plain_size(64,>=128) num_clients \n \
                 OR ./test servernet num_servers";


    return 0;
}

 
