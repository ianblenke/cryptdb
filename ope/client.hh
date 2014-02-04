#pragma once

#include <algorithm>
#include <vector>
#include <cmath>
#include <stdint.h>
#include <iostream>
#include <exception>
#include <utility>
#include <time.h>
#include <stdlib.h>
#include <sstream>
#include <util/net.hh>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <edb/Connect.hh>
#include <crypto/blowfish.hh>
#include <util/static_assert.hh>
#include <util/ope-util.hh>
#include <util/msgsend.hh>
#include <ope/merkle.hh>
#include <ope/btree.hh>


#include <pthread.h>

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::vector;
using std::cerr;
using std::max;


Connect* dbconnect;

bool test_order(int num_vals, int sorted, bool deletes);

void handle_udf(void* lp);


template<class V, class BlockCipher>
class ope_client {
public:
  	
    ope_client(BlockCipher *bc, bool malicious, 
        int c_port = OPE_CLIENT_PORT, int s_port = OPE_SERVER_PORT);
    ~ope_client();
 
    /* Encryption is the path bits (aka v) bitwise left shifted by num_bits
     * so that the last num_bits can indicate the index of the value pt at
     * at the node found by the path
     * tname is for DB setting
     */
 
    uint64_t encrypt(V det, bool do_insert, string tname = "");


    //cipher
    BlockCipher *bc;

    // network
    pthread_t net_thread;
    int sock; //socked on which client listens for connections
    int csock; //socket to server on which client answers to interactions
    int sock_query; // socket to send queries to server
    struct sockaddr_in my_addr;
   
    // merkle
    bool MALICIOUS;
    string merkle_root;


    string handle_interaction(istringstream & iss);   
};

template<class V, class BC>
string
ope_client<V, BC>::handle_interaction(istringstream & iss){
    if (DEBUG_COMM) {cerr<<"Called handle_interaction"<<endl;}

    MsgType func_d;
    iss>>func_d;
  
    assert_s(func_d == MsgType::INTERACT_FOR_LOOKUP, "Incorrect function type in handle_server!");
    if (func_d == MsgType::INTERACT_FOR_LOOKUP){
        
        V det, pt, tmp_pt;
        int size, index;
        bool some_flag = false;

        det = Cnv<V>::TypeFromStr(unmarshall_binary(iss));

	iss >> size;

        pt = bc->decrypt(det);

        for(index=0; index<size; index++){
	    V tmp_key;
	    tmp_key = Cnv<V>::TypeFromStr(unmarshall_binary(iss));
            tmp_pt = bc->decrypt(tmp_key);
            some_flag = (tmp_pt==pt);

            if (tmp_pt >= pt){
                break;
            }
        }

        stringstream o;
        o << index << " " << some_flag;
        return o.str();
        
    }
    
    return "";
  
}

template<class V, class BlockCipher>
static void *
comm_thread(void * p) {

    ope_client<V, BlockCipher> * oc = (ope_client<V, BlockCipher> *) p;
    
    string interaction_rslt="";

    while (true) {

	if (DEBUG_COMM) { cerr << "comm_th: waiting to receive \n";}
        //Receive message to process
	uint len = 0;
	std::string buffer;
        assert_s((len = xmsg_recv(oc->csock, &buffer)) > 0,
		 "receive gets  <=0 bytes");
	if (DEBUG_COMM) {cerr << "bytes received during interaction is " << len << "\n";}
	
        istringstream iss(buffer);
	
        interaction_rslt = oc->handle_interaction(iss);

        assert_s(interaction_rslt!="", "interaction error");

	if (DEBUG_COMM) {cerr << "sending " << interaction_rslt << "\n";}
        assert_s(xmsg_send(oc->csock, interaction_rslt)
		 == (int)interaction_rslt.size(),
		 "send failed");

        interaction_rslt = "";
    }
  
    return NULL;

}



template<class V, class BlockCipher>
ope_client<V, BlockCipher>::ope_client(BlockCipher * bc, bool malicious, 
    int c_port, int s_port) {

    MALICIOUS = malicious;
    
    //Socket connection
    sock = create_and_bind(c_port);

    //Start listening
    int listen_rtn = listen(sock, 10);
    if (listen_rtn < 0) {
	cerr << "Error listening to socket" << endl;
    }
    if (DEBUG_BARE) cerr << "Listening \n";

    socklen_t addr_size = sizeof(sockaddr_in);
    struct sockaddr_in sadr;

    csock = accept(sock, (struct sockaddr*) &sadr, &addr_size);    
    assert_s(csock >= 0, "Client failed to accept connection");
    if (DEBUG_BARE) cerr << "accepted connection (from server hopefully)\n";

    while ((sock_query = create_and_connect(OPE_SERVER_HOST, s_port, false)) < 0) {
	if (DEBUG_BARE) cerr << "server still not up\n";
	sleep(1);
    }
    
    this->bc = bc;
    
    int rc = pthread_create(&net_thread, NULL, comm_thread<V, BlockCipher>, (void *)this);
    if (rc) {
	cerr << "error: cannot create thread: " << rc << "\n";
	exit(1);
    }
	 
}


template<class V, class BlockCipher>
ope_client<V, BlockCipher>::~ope_client(){
    close(sock_query);
    close(csock);    
    close(sock);
    if (DEBUG_BARE) cerr<<"Done with client, closing now\n";
    delete bc;
}

template<class V, class BC>
static void
check_proof(stringstream & ss, MsgType optype, V ins,
	    const string & old_mroot, string & new_mroot, BC * bc) {

    string inserted = Cnv<V>::StrFromType(ins);
    if (optype == MsgType::ENC_INS) {
	int pft;
	ss >> pft;
	if ((ProofType)pft == PF_INS) {
	    UpdateMerkleProof p;
	    p << ss;
	    if (DEBUG_COMM) {cerr << "proof i got " << p.pretty() << "\n";}
	    assert_s(verify_ins_merkle_proof<V, BC>(p, inserted, old_mroot, new_mroot, bc),
		     "client received incorrect insertion proof from server");
	    if (DEBUG_COMM) cerr << "proof verified\n";
	}
	else {
	    assert_s((ProofType)pft == PF_QUERY, "invalid proof type");
	    MerkleProof p;
	    p << ss;
	    if (DEBUG_COMM) {cerr << "proof i got " << p.pretty() << "\n";}
	    assert_s(verify_merkle_proof(p, old_mroot),
		     "client received incorrect insert-repeated proof from server");
	    new_mroot = old_mroot;
	    if (DEBUG_COMM) cerr << "proof verified\n";
	    
	}
	return;
    }
    if (optype == MsgType::QUERY) {
	int pft;
	ss >> pft;
	assert_s((ProofType)pft == PF_QUERY, "expected query proof");
	MerkleProof p;
	p << ss;
	assert_s(verify_merkle_proof(p, old_mroot), "client received incorrect node merkle proof");
	new_mroot = old_mroot;
	return;
    }
    assert_s(false, "invalid msg type when checking proof");
}

template<class V, class BlockCipher>
OPEType
ope_client<V, BlockCipher>::encrypt(V pt, bool imode, string tname) {

    if (imode==false) {
	std::cerr << "IMODE FALSE!" << std::endl;
    }
    
    MsgType optype;
    if (imode) {
	optype = MsgType::ENC_INS;
    } else {
	optype = MsgType::QUERY;
    }
    V ct = bc->encrypt(pt);

    ostringstream msg;
    msg << optype <<  " ";
    if (WITH_DB) {
	assert_s(tname.size(), "empty table name");
	msg << tname << " ";
    }
    marshall_binary(msg, Cnv<V>::StrFromType(ct));
    msg << " ";

    if (DEBUG_COMM) cerr << "cl: sending message to server " << msg.str() <<"\n";
    string res = send_receive(sock_query, msg.str());
    if (DEBUG_COMM) cerr << "response size " << res.size() << "\n";
    //cerr << "Result for " << pt << " is\n" << res << endl << endl;

    stringstream ss(res);
    OPEType ope;
    ss >> ope;
    
    if (MALICIOUS) { // check Merkle proofs
	string new_mroot;
	check_proof<V, BlockCipher>(ss, optype, ct, merkle_root, new_mroot, bc);
	merkle_root = new_mroot;
    }

    if (DEBUG_BARE) cerr << "ope val is " << ope <<"\n";
    
    return ope;
}

