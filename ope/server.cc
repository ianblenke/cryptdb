#include "server.hh"
#include <exception>
#include <utility>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>
#include <sys/types.h>

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::vector;
using std::cerr;
using std::map;

template<class EncT>
void
Server<EncT>::interaction(EncT ciph,
			  uint & rindex, uint & nbits,
			  uint64_t & ope_path,
			  bool & equals) {

    assert_s(ope_tree.root != NULL, "root is null");
    tree_node<EncT> * curr = ope_tree.root;

    ope_path = 0;
    nbits = 0;

    while (true) {
	cerr << "LOOP! \n";
    	// create message and send it to client
    	stringstream msg;
    	msg.clear();
    	msg << MsgType::INTERACT_FOR_LOOKUP << " ";
    	msg << ciph << " ";

    	uint len = curr->keys.size();
    	msg << len << " ";

    	for (uint i = 0; i < len; i++) {
    	    msg << curr->keys[i] << " ";
    	}

    	string _reply = send_receive(sock_cl, msg.str());
    	istringstream reply(_reply);

    	uint index;
    	bool equals = false;
    	reply >> index >> equals;

    	assert_s(len >= index, "index returned by client is incorrect");

	if (equals) {
	    rindex = index;
	    return;
	}
	
	EncT next_ciph = (index == 0) ? 0 : curr->keys[index-1];

	if (!curr->key_in_map(next_ciph)) {
	    rindex = index;
	    return;
	}
	
	curr = curr->right[next_ciph];
    	ope_path = (ope_path << num_bits) | index;
    	nbits += num_bits;
    }
    
}

template<class EncT>
void 
Server<EncT>::handle_enc(int csock, istringstream & iss, bool do_ins) {
   
    uint64_t ciph;
    iss >> ciph;
    if(DEBUG_COMM) cout<< "ciph: "<< ciph << endl;

    string ope;
    
    auto ts = ope_table.find(ciph);
    if (ts != ope_table.end()) { // found in OPE Table
    	if (DEBUG) {cerr << "found in ope table"; }
    	// if this is an insert, increase ref count
    	if (do_ins) {
    	    ts->second.refcount++;
    	}
	ope = opeToStr(ts->second.ope);
   	
    } else { // not found in OPE Table
    	if (DEBUG) {cerr << "not in ope table \n"; }

    	uint index = 0;
    	uint64_t ope_path = 0;
    	bool equals = false;
    	uint nbits = 0;

    	interaction(ciph, index, nbits, ope_path, equals);

        assert_s(!equals, "equals should always be false");
    	
    	if (DEBUG) {cerr << "new ope_enc has "<< " path " << ope_path
    			 << " index " << index << "\n";}
    	if (do_ins) {
    	    // insert in OPE Tree
            // ope_insert also updates ope_table
    	    ope_tree.insert(ope_path, nbits, index, ciph, ope_table);

            ts = ope_table.find( ciph);
            assert_s( ts != ope_table.end() , "after insert, value must be in ope_table");
            ope = opeToStr(ts->second.ope);            
        }else{

            uint64_t ope_enc = compute_ope<EncT>(ope_path, nbits, index);

            //Subtract 1 b/c if ope points to existing value in tree, want this ope value
            //to be less than 
            ope = opeToStr(ope_enc-1);            
        }
    }
    assert_s(send(csock, ope.c_str(), ope.size(), 0) == (int)ope.size(),
	     "problem with send");
    
}

template<class EncT>
void
Server<EncT>::dispatch( int csock, istringstream & iss) {
   
    MsgType msgtype;
    iss >> msgtype;

    if (DEBUG_COMM) {cerr << "message type is " << mtnames[(int) msgtype] << endl;}

    switch (msgtype) {
    case MsgType::ENC_INS: {
	return handle_enc(csock, iss, true);
    }
    case MsgType::QUERY: {
	return handle_enc(csock, iss, false);
    }
    default: {}
    }

    assert_s(false, "invalid message type in dispatch");
    
}

template<class EncT>
Server<EncT>::Server() {
    sock_cl = create_and_connect(OPE_CLIENT_HOST, OPE_CLIENT_PORT);
    sock_udf = create_and_bind(OPE_SERVER_PORT);
}

template<class EncT>
Server<EncT>::~Server() {
    close(sock_cl);
    close(sock_udf);
}


int main(int argc, char **argv){

    cerr<<"Starting tree server \n";
    Server<uint64_t> server;
    
    //Start listening
    int listen_rtn = listen(server.sock_udf, 10);
    if (listen_rtn < 0) {
	cerr<<"Error listening to socket"<<endl;
    }
    cerr<<"Listening \n";
    
    socklen_t addr_size = sizeof(sockaddr_in);
    struct sockaddr_in sadr;
    
    int csock = accept(server.sock_udf, (struct sockaddr*) &sadr, &addr_size);
    assert_s(csock >= 0, "Server accept failed!");
    cerr << "Server accepted connection with udf \n";

    uint buflen = 10240;
    char buffer[buflen];

    while (true) {
    	cerr<<"Listening..."<<endl;

        memset(buffer, 0, buflen);

	uint len;
        assert_s((len = recv(csock, buffer, buflen, 0)) > 0, "received 0 bytes");

        if (DEBUG_COMM) {cerr << "received msg " << buffer << endl;}
        istringstream iss(buffer);
	
	//Pass connection and messages received to handle_client
	server.dispatch(csock, iss);

    }

    close(csock);
    cerr << "Server exits!\n";
}
