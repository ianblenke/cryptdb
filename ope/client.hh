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
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>
#include <sys/types.h>
#include <edb/Connect.hh>
#include <crypto/blowfish.hh>
#include <util/static_assert.hh>
#include "btree.hh"

//Whether to print debugging output or not
#define DEBUG 0
#define DEBUG_COMM 1
#define DEBUG_BTREE 1

using namespace std;

const int N = 4;
const double alpha = 0.3;

Connect* dbconnect;

// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

uint64_t mask;

uint64_t make_mask();
bool test_order(int num_vals, int sorted, bool deletes);

//Make mask of num_bits 1's
uint64_t make_mask(){
	uint64_t cur_mask = 1ULL;
	for(int i=1; i<num_bits; i++){
		cur_mask = cur_mask | (1ULL<<i);
	}
	return cur_mask;
}

static inline int
ffsl(uint64_t ct)
{
	int bit;

	if (ct == 0)
		return (0);
	for (bit = 1; !(ct & 1); bit++)
		ct = (uint64_t)ct >> 1;
	return (bit+num_bits-1);
}

class ope_lookup_failure {};

string cur_merkle_hash;

template<class V, class BlockCipher>
class ope_client {
 public:
	int hsock;
	struct sockaddr_in my_addr;
	

    ope_client(BlockCipher *bc): b(bc) {
    	_static_assert(BlockCipher::blocksize == sizeof(V));

		int host_port = 1111;
		string host_name = "127.0.0.1";

		hsock = socket(AF_INET, SOCK_STREAM,0);
		if(hsock==-1) cout<<"Error initializing socket!"<<endl;
		

		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(host_port);
		memset(&(my_addr.sin_zero),0, 8);
		my_addr.sin_addr.s_addr = inet_addr(host_name.c_str());

		if(connect(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr))<0){
			cout<<"Connect Failed!"<<endl;
		}
		cur_merkle_hash="";

    }

    ~ope_client(){
    	close(hsock);
    }

    /* Determines the index of pt's predecesor given a node's key vector
     * Returns -1 if pt is in the vector, 0 is if is smaller than all
     * elements in the vector, or 1+index where index is the pred's index
     * in the vector (1 is added due to my protocol, where 0 represents null)
    */
    static int predIndex(vector<V> vec, V pt){
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

    V decrypt(uint64_t ct) const {
    	char buffer[1024];

		uint64_t nbits = 64 - ffsl((uint64_t)ct);
		uint64_t v= ct>>(64-nbits); //Orig v
		uint64_t path = v>>num_bits; //Path part of v
		int index = (int) v & mask; //Index (last num_bits) of v
		if(DEBUG) cout<<"Decrypt lookup with path="<<path<<" nbits="<<nbits-num_bits<<" index="<<index<<endl;
        //vector<V> vec = s->lookup(path, nbits- num_bits);

   		ostringstream o;
	    o.str("");
	    o.clear();
	    o<<"2 "<<path<<" "<<(nbits-num_bits);
	    string msg = o.str();
	    if(DEBUG_COMM) cout<<"Sending decrypt msg: "<<msg<<endl;
	    send(hsock, msg.c_str(), msg.size(),0);
		recv(hsock, buffer, 1024, 0);

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

    void delete_value(V pt){
    	pair<uint64_t, int> enc = encrypt(pt);
    	uint64_t ct = enc.first;

		uint64_t nbits = 64 - ffsl((uint64_t)ct);
		uint64_t v= ct>>(64-nbits); //Orig v
		uint64_t path = v>>num_bits; //Path part of v
		int index = (int) v & mask; //Index (last num_bits) of v

		ostringstream o;
		o.str("");
		o.clear();
		o<<"4 "<<path<<" "<<nbits-num_bits<<" "<<index;
		string msg = o.str();
		if(DEBUG) cout<<"Deleting pt="<<pt<<" with msg "<<msg<<endl;

		char buffer[1024];
	    memset(buffer, '\0', 1024);

		send(hsock, msg.c_str(), msg.size(), 0);
		recv(hsock, buffer, 1024, 0);

		if(DEBUG_BTREE){
			cout<<"Delete_value buffer recv: "<<buffer<<endl;
		}
	    const string tmp_merkle_hash = cur_merkle_hash;
	    string new_merkle_hash;
	    DelMerkleProof dmp;
	    stringstream iss(buffer);
	    iss>>new_merkle_hash;
	    char tmp_hash[1024];
	    strcpy(tmp_hash, buffer);
	    char * space = strchr(tmp_hash, ' ');
	    *space='\0';
	    string tmp_hash_str (tmp_hash);
	    cur_merkle_hash=tmp_hash_str;	    
   	    //cur_merkle_hash=new_merkle_hash;

	    iss>>dmp;	

	    if(DEBUG_BTREE){
	    	cout<<"Delete_value new mh="<<cur_merkle_hash<<endl;
	    	cout<<"DMP="<<dmp<<endl;
	    }
	    if(!verify_del_merkle_proof(dmp, tmp_merkle_hash, new_merkle_hash)){
	    	cout<<"Deletion merkle proof failed!"<<endl;
	    	exit(-1);
	    }
	    if(DEBUG_BTREE) cout<<"Deletion merkle proof succeeded"<<endl;

    }


    //Function to tell tree server to insert plaintext pt w/ v, nbits
    pair<uint64_t, int> insert(uint64_t v, uint64_t nbits, uint64_t index, V pt, V det) const{
		if(DEBUG) cout<<"pt: "<<pt<<" det: "<<det<<"  not in tree. "<<nbits<<": "<<" v: "<<v<<endl;
		
		//s->insert(v, nbits, index, pt);
    	ostringstream o;
	    o.str("");
	    o.clear();
	    o<<"3 "<<v<<" "<<nbits<<" "<<index<<" "<<det;
	    string msg = o.str();
	    if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;

	    char buffer[1024];
	    memset(buffer, '\0', 1024);
	    send(hsock, msg.c_str(), msg.size(),0);
	    recv(hsock, buffer, 1024, 0);

	    if(DEBUG_BTREE) cout<<"Insert buffer="<<buffer<<endl;
	    const string tmp_merkle_hash = cur_merkle_hash;
	    string new_merkle_hash;
	    InsMerkleProof imp;
	    stringstream iss(buffer);
	    iss>>new_merkle_hash;
	    char tmp_hash[1024];
	    strcpy(tmp_hash, buffer);
	    char* space = strchr(tmp_hash,' ');
	    *space='\0';
	    string tmp_hash_str (tmp_hash);
	    cur_merkle_hash=tmp_hash_str;
	    iss>>imp;
	    
	    if(DEBUG_BTREE){
	    	cout<<"Insert New mh="<<cur_merkle_hash<<endl;
	    	cout<<"Insert proof="<<imp<<endl;
	    }
	    if(!verify_ins_merkle_proof(imp, tmp_merkle_hash, new_merkle_hash)){
	    	cout<<"Insertion merkle proof failed!"<<endl;
	    	exit(-1);
	    }
	    if(DEBUG_BTREE) cout<<"Insert mp succeeded"<<endl;
	    //relabeling may have been triggered so we need to lookup value again
	    //todo: optimize by avoiding extra tree lookup
	    return encrypt(pt);
    }

    /* Encryption is the path bits (aka v) bitwise left shifted by num_bits
     * so that the last num_bits can indicate the index of the value pt at
     * at the node found by the path
     */

    /*Server protocol code:
	 * lookup(encrypted_plaintext) = 1
	 * lookup(v, nbits) = 2
	 * insert(v, nbits, index, encrypted_laintext) = 3
	 * delete(v, nbits, index) = 4
	*/
    pair<uint64_t, int> encrypt(V pt) const{

        uint64_t v = 0;
        uint64_t nbits = 0;

        V det = block_encrypt(pt);

        if(DEBUG_COMM) cout<<"Encrypting pt: "<<pt<<" det: "<<det<<endl;

    	char buffer[1024];
    	memset(buffer, '\0', 1024);

    	//table_storage test = s->lookup(pt);
    	ostringstream o;
    	o<<"1 "<<det;

    	string msg = o.str();
    	if(DEBUG_COMM) cout<<"Sending init msg: "<<msg<<endl;

    	send(hsock, msg.c_str(), msg.size(), 0);
    	recv(hsock, buffer, 1024, 0);
    	uint64_t early_v, early_pathlen, early_index, early_version;

    	istringstream iss(buffer);
    	iss>>early_v;
    	iss>>early_pathlen;
    	iss>>early_index;
    	iss>>early_version;

    	if(DEBUG_COMM) cout<<"Received early: "<<early_v<<" : "<<early_pathlen<<" : "<<early_index<<endl;
    	if(early_v!=(uint64_t)-1 && early_pathlen!=(uint64_t)-1 && early_index!=(uint64_t)-1){
    		if(DEBUG_COMM) cout<<"Found match from table early!"<<endl;

    		v = early_v;
    		nbits = early_pathlen+num_bits;
    		if(DEBUG_COMM) cout<<"Found "<<pt<<" with det="<<det<<" in table w/ v="<<v<<" nbits="<<nbits<<" index="<<early_index<<endl;
    		v = (v<<num_bits) | early_index;  
    		return make_pair((v<<(64-nbits)) | (mask<<(64-num_bits-nbits)),
    			early_version);
    	}
        for (;;) {
        	if(DEBUG) cout<<"Do lookup for "<<pt<<" with det: "<<det<<" v: "<<v<<" nbits: "<<nbits<<endl;
			//vector<V> xct = s->lookup(v, nbits);
        	o.str("");
        	o.clear();
        	o<<"2 "<<v<<" "<<nbits;
        	msg = o.str();
        	if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;
			send(hsock, msg.c_str(), msg.size(), 0);
			memset(buffer, 0, 1024);
			recv(hsock, buffer, 1024, 0);
			if(DEBUG_COMM || DEBUG_BTREE) cout<<"Received during iterative lookup: "<<buffer<<endl;
			string check(buffer);        	

			if(check=="ope_fail"){
				return insert(v, nbits, 0, pt, det);

			}				

			stringstream iss_tmp(buffer);
			vector<V> xct_vec;
			V xct;
			string checker="";
			while(true){
				iss_tmp >> checker;
				if(checker==";") break;
				if(DEBUG_BTREE) cout<<"xct_vec: "<<checker<<endl;
				stringstream tmpss;
				tmpss<<checker;
				tmpss>>xct;
				

				//predIndex later assumes vector is of original plaintext vals
				xct_vec.push_back(block_decrypt(xct));
			}
			MerkleProof mp;
			if(DEBUG_BTREE) cout<<"Remaining mp info = "<<iss_tmp.str()<<endl;
			for(int i=0; i<(int) xct_vec.size(); i++){
				iss_tmp >> mp;
				if(DEBUG_BTREE) cout<<"MP "<<i<<"="<<mp<<endl;
				const string tmp_merkle_hash = cur_merkle_hash;
				if(!verify_merkle_proof(mp, tmp_merkle_hash)){
					cout<<"Merkle hash during lookup didn't match!"<<endl;
					exit(-1);
				}
				if(DEBUG_BTREE) cout<<"MP "<<i<<" succeeded"<<endl;
			}

			//Throws ope_lookup_failure if xct size < N, meaning can insert at node
			int pi;
			try{
				pi = predIndex(xct_vec, pt);
			}catch(ope_lookup_failure&){
				int index;
				for(index=0; index<(int) xct_vec.size(); index++){
					if(pt<xct_vec[index]) break;
				}
				return insert(v, nbits, index, pt, det);
			}
			nbits+=num_bits;
	        if (pi==-1) {
	        	//pt already exists at node
	        	int index;
	        	for(index=0; index< (int) xct_vec.size(); index++){
	        		//Last num_bits are set to represent index of pt at node
	        		if(xct_vec[index]==pt) v = (v<<num_bits) | index;
	        	}
			    break;
			}else {
	            v = (v<<num_bits) | pi;
			}
	        //Check that we don't run out of bits!
	        if(nbits > 63) {
	        	cout<<"nbits larger than 64 bits!!!!"<<endl;
	        }			
	    }
	    /*If value was inserted previously, then should get result
		 *from server ope_table. If not, then it will insert. So never
		 *should reach this point (which means no ope_table result or insertion)
		 */
        cout<<"SHOULD NEVER REACH HERE!"<<endl;
        exit(1);
        return make_pair(0,0);
		//FL todo: s->update_table(block_encrypt(pt),v,nbits);
/*		if(DEBUG) cout<<"Encryption of "<<pt<<" has v="<< v<<" nbits="<<nbits<<endl;
        return (v<<(64-nbits)) | (mask<<(64-num_bits-nbits));*/
    }

private:
    V block_decrypt(V ct) const {
        V pt;
        b->block_decrypt((const uint8_t *) &ct, (uint8_t *) &pt);
        return pt;
    }

    V block_encrypt(V pt) const {
        V ct;
        b->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
        return ct;
    }

    BlockCipher *b;
    
};
