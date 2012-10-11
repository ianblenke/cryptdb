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
#include "btree.hh"
#include <util/ope-util.hh>



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

class ope_lookup_failure {};


#if MALICIOUS
std::string cur_merkle_hash;
#endif


template<class V, class BlockCipher>
class ope_client {
public:
    int hsock;
    struct sockaddr_in my_addr;
	

    ope_client(BlockCipher *bc);
    ~ope_client();
  

    /* Determines the index of pt's predecesor given a node's key vector
     * Returns -1 if pt is in the vector, 0 is if is smaller than all
     * elements in the vector, or 1+index where index is the pred's index
     * in the vector (1 is added due to my protocol, where 0 represents null)
     */
    static int predIndex(std::vector<V> vec, V pt);
  
    /* Encryption is the path bits (aka v) bitwise left shifted by num_bits
     * so that the last num_bits can indicate the index of the value pt at
     * at the node found by the path
     */
    /*Server protocol code:
     * lookup(encrypted_plaintext) = 1
     * lookup(v, nbits) = 2
     * insert(v, nbits, index, encrypted_plaintext) = 3
     * delete(v, nbits, index) = 4
     */

    uint64_t encrypt(V det, bool imode) const;
    V decrypt(uint64_t ct) const;
    
    void delete_value(V pt);

    //Function to tell tree server to insert plaintext pt w/ v, nbits
    void insert(uint64_t v, uint64_t nbits, uint64_t index, V pt, V det) const;


private:
    V block_decrypt(V ct) const;

    V block_encrypt(V pt) const;

    BlockCipher *b;
    
};

// must define here these few functions if we want to call them from other files..

template<class V, class BlockCipher>
ope_client<V, BlockCipher>::ope_client(BlockCipher * bc) : b(bc) {
    assert_s(BlockCipher::blocksize == sizeof(V), "problem with block cipher size");

    hsock = create_and_connect(OPE_SERVER_HOST, OPE_SERVER_PORT);

#if MALICIOUS        
    cur_merkle_hash="";
#endif

    	
}


template<class V, class BlockCipher>
ope_client<V, BlockCipher>::~ope_client(){
    close(hsock);
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

template<class V, class BlockCipher>
uint64_t
ope_client<V, BlockCipher>::encrypt(V det, bool imode) const{

    if (imode==false) {
	std::cout<< "IMODE FALSE!" << std::endl;
    }
    
    uint64_t v = 0;
    uint64_t nbits = 0;

    V pt = block_decrypt(det);

    if(DEBUG_COMM || !imode) std::cout << "Encrypting pt: " << pt << " det: " << det << std::endl;

    char buffer[10240];
    memset(buffer, '\0', 10240);

    //table_storage test = s->lookup(pt);
    std::ostringstream o;
    std::string msg;

    for (;;) {
        if (DEBUG) {
	    std::cout<<"Do lookup for "<<pt<<" with det: "<< det<<" v: "<< v <<" nbits: "<< nbits << std::endl;
	}
        //vector<V> xct = s->lookup(v, nbits);
        o.str("");
        o.clear();
        o<<"2 "<<v<<" "<<nbits;
        msg = o.str();
        if (DEBUG_COMM) {
	    std::cout << "Sending msg: " << msg << std::endl;
	}
        if(send(hsock, msg.c_str(), msg.size(), 0)!= (int)msg.size()){
            assert_s(false, "encrypt 1 send failed");
        }
        memset(buffer, 0, 10240);
        if(recv(hsock, buffer, 10240, 0)<=0){
            assert_s(false, "encrypt 1 recv failed");
        }
        if(DEBUG_COMM || DEBUG_BTREE) std::cout << "Received during iterative lookup: " << buffer << std::endl;
	std::string check(buffer);           

        if(check=="ope_fail"){
            if(imode){
                insert(v, nbits, 0, pt, det);
                return 0;
            }else{
		std::cout << "n mode ope_fail" << std::endl;
                nbits+=num_bits;
                v=(v<<num_bits) | 0;
                return (v<<(64-nbits)) | (mask<<(64-num_bits-nbits));
            }

        }               

	std::stringstream iss_tmp(buffer);
	std::vector<V> xct_vec;
        V xct;
	std::string checker="";
        while(true){
            iss_tmp >> checker;

	    if(checker==";")
		break;

	    if(DEBUG_BTREE)
		std::cout << "xct_vec: " << checker << std::endl;
	    
	    std::stringstream tmpss;

	    tmpss<<checker;
            tmpss>>xct;
            

            //predIndex later assumes vector is of original plaintext vals
            xct_vec.push_back(block_decrypt(xct));
        }

#if MALICIOUS
        MerkleProof mp;

	if(DEBUG_BTREE) cout<<"Remaining mp info = "<<iss_tmp.str()<<endl;

        for(int i=0; i<(int) xct_vec.size(); i++){
            iss_tmp >> mp;
            if(DEBUG_BTREE) cout<<"MP "<<i<<"="<<mp<<endl;
            const std::string tmp_merkle_hash = cur_merkle_hash;
            assert_s(verify_merkle_proof(mp, tmp_merkle_hash),
		     "Merkle hash during lookup didn't match!");
	    
            if(DEBUG_BTREE) std::cout << "MP "<< i <<" succeeded" << std::endl;
        }
#endif

        //Throws ope_lookup_failure if xct size < N, meaning can insert at node
        int pi;
        try{
            pi = predIndex(xct_vec, pt);
        }catch(ope_lookup_failure&){
            int index;
            for (index=0; index<(int) xct_vec.size(); index++){
                if(pt<xct_vec[index]) break;
            }
            if (imode){
                insert(v, nbits, index, pt, det);
                return 0;                
            }else{
		std::cout << "n mode inserting " << index << " : " << xct_vec.size() << std::endl;
                nbits+=num_bits;
                v = (v<<num_bits) | index;
                return ((v<<(64-nbits)) | (mask<<(64-num_bits-nbits)));
            }

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
        } else {
            v = (v<<num_bits) | pi;
        }
        //Check that we don't run out of bits!
        if(nbits > 63) {
	    std::cout<<"nbits larger than 64 bits!!!!"<< std::endl;
	    assert_s(false, "nbits > 63");
        }           
    }
    
    /* If value was inserted previously, then should get result
     * from server ope_table. If not, then it will insert. So never
     * should reach this point (which means no ope_table result or insertion)
     */
    assert_s(false, "SHOULD NEVER REACH HERE!");

    return -1;
    //FL todo: s->update_table(block_encrypt(pt),v,nbits);
/*      if(DEBUG) cout<<"Encryption of "<<pt<<" has v="<< v<<" nbits="<<nbits<<endl;
	return (v<<(64-nbits)) | (mask<<(64-num_bits-nbits));*/
}



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
    int index = (int) v & mask; //Index (last num_bits) of v
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
    int index = (int) v & mask; //Index (last num_bits) of v

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

template<class V, class BlockCipher>
V
ope_client<V, BlockCipher>::block_decrypt(V ct) const {
    V pt;
    b->block_decrypt((const uint8_t *) &ct, (uint8_t *) &pt);
    return pt;
}

template<class V, class BlockCipher>
V
ope_client<V, BlockCipher>::block_encrypt(V pt) const {
    V ct;
    b->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
    return ct;
}

