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
  	
    ope_client(BlockCipher *bc);
    ~ope_client();
 
    /* Encryption is the path bits (aka v) bitwise left shifted by num_bits
     * so that the last num_bits can indicate the index of the value pt at
     * at the node found by the path
     */
 
    uint64_t encrypt(V det, bool do_insert);


    //cipher
    BlockCipher *bc;

    // network
    pthread_t net_thread;
    int sock; //socked on which client listens for connections
    int csock; //socket to server on which client answers to interactions
    int sock_query; // socket to send queries to server
    struct sockaddr_in my_addr;
   
    // merkle
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

        det = Cnv<V>::TypeFromStr(unmarshall_binary(iss));

	iss >> size;

        pt = bc->decrypt(det);

        for(index=0; index<size; index++){
	    V tmp_key;
	    tmp_key = Cnv<V>::TypeFromStr(unmarshall_binary(iss));
            tmp_pt = bc->decrypt(tmp_key);

            if (tmp_pt >= pt){
                break;
            }

        }

        stringstream o;
        o << index << " " << (tmp_pt==pt);
        return o.str();
        
    }
    
    return "";
  
}

template<class V, class BlockCipher>
static void *
comm_thread(void * p) {

    ope_client<V, BlockCipher> * oc = (ope_client<V, BlockCipher> *) p;
    
    int buflen = 1024;
    char buffer[buflen];

    string interaction_rslt="";

    while (true) {

        memset(buffer, 0, buflen);
	
	if (DEBUG_COMM) { cerr << "comm_th: waiting to receive \n";}
        //Receive message to process
	uint len = 0;
        assert_s((len = recv(oc->csock, buffer, buflen, 0)) > 0,
		 "receive gets  <=0 bytes");
	if (DEBUG_COMM) {cerr << "bytes received during interaction is " << len << "\n";}
	
        istringstream iss(buffer);
	
        interaction_rslt = oc->handle_interaction(iss);

        assert_s(interaction_rslt!="", "interaction error");

	if (DEBUG_COMM) {cerr << "sending " << interaction_rslt << "\n";}
        assert_s(send(oc->csock, interaction_rslt.c_str(), interaction_rslt.size(),0)
		 == (int)interaction_rslt.size(),
		 "send failed");

        interaction_rslt = "";
    }
  
    return NULL;

}
// must define here these few functions if we want to call them from other files..

template<class V, class BlockCipher>
ope_client<V, BlockCipher>::ope_client(BlockCipher * bc) {
    
    //Socket connection
    sock = create_and_bind(OPE_CLIENT_PORT);

    //Start listening
    int listen_rtn = listen(sock, 10);
    if (listen_rtn < 0) {
	cerr << "Error listening to socket" << endl;
    }
    cerr << "Listening \n";

    socklen_t addr_size = sizeof(sockaddr_in);
    struct sockaddr_in sadr;

    csock = accept(sock, (struct sockaddr*) &sadr, &addr_size);    
    assert_s(csock >= 0, "Client failed to accept connection");
    cerr << "accepted connection (from server hopefully)\n";

    while ((sock_query = create_and_connect(OPE_SERVER_HOST, OPE_SERVER_PORT, false)) < 0) {
	cerr << "server still not up\n";
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
    cerr<<"Done with client, closing now\n";
    delete bc;
}


/*OLD: Server protocol code:
 * lookup(encrypted_plaintext) = 1
 * lookup(v, nbits) = 2
 * insert(v, nbits, index, encrypted_laintext) = 3
 * delete(v, nbits, index) = 4
 */

template<class V, class BC>
static void
check_proof(stringstream & ss, MsgType optype, V ins,
	    const string & old_mroot, string & new_mroot, BC * bc) {

    string inserted = Cnv<V>::StrFromType(ins);
    if (optype == MsgType::ENC_INS) {
	UpdateMerkleProof p;
	p << ss;
	if (DEBUG_COMM) {cerr << "proof i got " << p.pretty() << "\n";}
	assert_s(verify_ins_merkle_proof<V, BC>(p, inserted, old_mroot, new_mroot, bc),
		 "client received incorrect insertion proof from server");
	return;
    }
    if (optype == MsgType::QUERY) {
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
ope_client<V, BlockCipher>::encrypt(V pt, bool imode) {

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
    marshall_binary(msg, Cnv<V>::StrFromType(ct));
    msg << " ";

    cerr << "cl: sending message to server " << msg.str() <<"\n";
    string res = send_receive(sock_query, msg.str(), 102400);
    cerr << "response size " << res.size() << "\n";
    //cerr << "Result for " << pt << " is\n" << res << endl << endl;

    stringstream ss(res);
    OPEType ope;
    ss >> ope;
    
    if (MALICIOUS) { // check Merkle proofs
	string new_mroot;
	check_proof<V, BlockCipher>(ss, optype, ct, merkle_root, new_mroot, bc);
	merkle_root = new_mroot;
    }

    cerr << "ope val is " << ope <<"\n";
    
    return ope;
}


/*
template<class V, class BlockCipher>
int
ope_client<V, BlockCipher>::predIndex(vector<V> vec, V pt){
    //Assumes vec is filled with decrypted non-DET values (original pt values)
    int tmp_index = 0;
    V tmp_pred_key =0;
    bool pred_found=false;
    for (int i = 0; i< (int) vec.size(); i++)
    {
	if(vec[i]==pt) return -1;
	if (vec[i] < pt && vec[i] > tmp_pred_key)
	{
	    pred_found=true;
	    tmp_pred_key = vec[i];
	    tmp_index = i;
	}
    }
    if(vec.size()<N-1) throw ope_lookup_failure();      
    if(pred_found) return tmp_index+1;
    else return 0;
}

template<class V, class BlockCipher>
V
ope_client<V, BlockCipher>::decrypt(uint64_t ct) const {
    char buffer[10240];

    uint64_t nbits = 64 - ffsl((uint64_t)ct);
    uint64_t v= ct>>(64-nbits); //Orig v
    uint64_t path = v>>num_bits; //Path part of v
    int index = (int) v & s_mask; //Index (last num_bits) of v
    if(DEBUG) cout<<"Decrypt lookup with path="<<path<<" nbits="<<nbits-num_bits<<" index="<<index<<endl;
    //vector<V> vec = s->lookup(path, nbits- num_bits);

    ostringstream o;
    o.str("");
    o.clear();
    o<<"2 "<<path<<" "<<(nbits-num_bits);
    string msg = o.str();
    if(DEBUG_COMM) cout<<"Sending decrypt msg: "<<msg<<endl;
    if( send(hsock, msg.c_str(), msg.size(),0) != msg.size()){
        assert_s(false, "decrypt send failed");
    }
    if( recv(hsock, buffer, 10240, 0) <= 0){
        assert_s(false, "decrypt recv failed");
    }

    if(DEBUG_BTREE) cout<<"Decrypt recv="<<buffer<<endl;
    istringstream iss_tmp(buffer);
    vector<V> xct_vec;
    V xct;
    string checker="";
    while(true){
        iss_tmp >> checker;
        if(checker==";") break;
        stringstream tmpss;
        tmpss<<checker; 
        tmpss>>xct;
        if(DEBUG_BTREE) cout<<"decrypt xct: "<<tmpss.str()<<endl;       
        

        xct_vec.push_back(xct);
    }        
    return block_decrypt(xct_vec[index]);

}

template<class V, class BlockCipher>
void
ope_client<V, BlockCipher>::delete_value(V pt){
    uint64_t ct = encrypt(pt);


    uint64_t nbits = 64 - ffsl((uint64_t)ct);
    uint64_t v= ct>>(64-nbits); //Orig v
    uint64_t path = v>>num_bits; //Path part of v
    int index = (int) v & s_mask; //Index (last num_bits) of v

    ostringstream o;
    o.str("");
    o.clear();
    o<<"4 "<<path<<" "<<nbits-num_bits<<" "<<index;
    string msg = o.str();
    if(DEBUG) cout<<"Deleting pt="<<pt<<" with msg "<<msg<<endl;

    char buffer[10240];
    memset(buffer, '\0', 10240);

    if( send(hsock, msg.c_str(), msg.size(), 0) != msg.size()){
        assert_s(false, "delete_value send failed");
    }
    if( recv(hsock, buffer, 10240, 0) <= 0){
        assert_s(false, "delete_value recv failed");
    }

#if MALICIOUS
    if(DEBUG_BTREE){
        cout<<"Delete_value buffer recv: "<<buffer<<endl;
    }
    const string tmp_merkle_hash = cur_merkle_hash;
    string new_merkle_hash;
    UpdateMerkleProof dmp;
    stringstream iss(buffer);
    iss>>new_merkle_hash;
    string tmp_hash_str (buffer, 10240);
    size_t hash_end = tmp_hash_str.find("hash_end");
    string new_hash_str (buffer, hash_end);
    cur_merkle_hash = new_hash_str;
    iss>>dmp;
    char tmp_hash[10240];
    strcpy(tmp_hash, buffer);
    size_t end = 
	*space='\0';
    string tmp_hash_str (tmp_hash);
    cur_merkle_hash=tmp_hash_str;  
    //cur_merkle_hash=new_merkle_hash;   

    if(DEBUG_BTREE){
        cout<<"Delete_value new mh="<<cur_merkle_hash<<endl;
        cout<<"DMP="<<dmp<<endl;
    }
    if(!verify_del_merkle_proof(dmp, "to_delete", tmp_merkle_hash, new_merkle_hash)){
        cout<<"Deletion merkle proof failed!"<<endl;
        exit(-1);
    }
    if(DEBUG_BTREE) cout<<"Deletion merkle proof succeeded"<<endl;

#endif

}


template<class V, class BlockCipher>
void
ope_client<V, BlockCipher>::insert(uint64_t v, uint64_t nbits, uint64_t index, V pt, V det) const{
    if(DEBUG) cout<<"pt: "<<pt<<" det: "<<det<<"  not in tree. "<<nbits<<": "<<" v: "<<v<<endl;
    
    //s->insert(v, nbits, index, pt);
    ostringstream o;
    o.str("");
    o.clear();
    o<<"3 "<<v<<" "<<nbits<<" "<<index<<" "<<det;
    string msg = o.str();
    if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;

    char buffer[10240];
    memset(buffer, '\0', 10240);
    if( send(hsock, msg.c_str(), msg.size(),0) != (int)msg.size() ){
        assert_s(false, "insert send failed");
    }
    if (recv(hsock, buffer, 10240, 0) <= 0 ){
        assert_s(false, "insert recv failed");
    }


#if MALICIOUS
    if(DEBUG_BTREE) cout<<"Insert buffer="<<buffer<<endl;
    const string tmp_merkle_hash = cur_merkle_hash;
    string new_merkle_hash;
    InsMerkleProof imp;
    stringstream iss(buffer);
    iss>>new_merkle_hash;
    string tmp_hash_str (buffer, 10240);
    size_t hash_end = tmp_hash_str.find("hash_end");
    string new_hash_str (buffer, hash_end-1);
    cur_merkle_hash = new_hash_str;
    iss>>imp;
    char tmp_hash[10240];
    strcpy(tmp_hash, buffer);
    char* space = strchr(tmp_hash,' ');
    *space='\0';
    string tmp_hash_str (tmp_hash);
    cur_merkle_hash=tmp_hash_str;
    
    if(DEBUG_BTREE){
        cout<<"Insert New mh="<<cur_merkle_hash<<endl;
        cout<<"Insert proof="<<imp<<endl;
    }
    if(!verify_ins_merkle_proof(imp, tmp_merkle_hash, new_merkle_hash)){
        cout<<"Insertion merkle proof failed!"<<endl;
        exit(-1);
    }
    if(DEBUG_BTREE) cout<<"Insert mp succeeded"<<endl;
#endif    

    //relabeling may have been triggered so we need to lookup value again
    //todo: optimize by avoiding extra tree lookup
    //return encrypt(pt);

}
*/
