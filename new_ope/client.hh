#include <vector>
#include <cmath>
#include <stdint.h>
#include <iostream>
#include <exception>
#include <boost/unordered_map.hpp>
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

//Whether to print debugging output or not
#define DEBUG 0
#define DEBUG_COMM 0

using namespace std;

const int N = 4;
const double alpha = 0.3;


// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

uint64_t mask;

uint64_t make_mask();
bool test_order(int num_vals, int sorted);

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


template<class V>
class ope_client {
 public:
	int hsock;
	struct sockaddr_in my_addr;

    ope_client() {

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
	    o<<"2 "<<path<<" "<<(nbits-num_bits);
	    string msg = o.str();
	    if(DEBUG_COMM) cout<<"Sending decrypt msg: "<<msg<<endl;
	    send(hsock, msg.c_str(), msg.size(),0);
		recv(hsock, buffer, 1024, 0);

		istringstream iss_tmp(buffer);
		vector<V> xct_vec;
		V xct, last;
		last= (V) NULL;
		while(true){
			iss_tmp >> xct;
			if(last==xct) break;
			last = xct;

			xct_vec.push_back(xct);
		}        
        return xct_vec[index];
	
    }

    //Function to tell tree server to insert plaintext pt w/ v, nbits
    pair<uint64_t, int> insert(uint64_t v, uint64_t nbits, V pt) const{
		if(DEBUG) cout<<pt<<"  not in tree. "<<nbits<<": "<<" v: "<<v<<endl;
		
		//s->insert(v, nbits, pt);
    	ostringstream o;
	    o.str("");
	    o<<"3 "<<v<<" "<<nbits<<" "<<pt;
	    string msg = o.str();
	    if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;
	    send(hsock, msg.c_str(), msg.size(),0);

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
	 * insert(v, nbits, encrypted_laintext) = 3
	*/
    pair<uint64_t, int> encrypt(V pt) const{

        uint64_t v = 0;
        uint64_t nbits = 0;

    	char buffer[1024];
    	memset(buffer, '\0', 1024);

    	//table_storage test = s->lookup(pt);
    	ostringstream o;
    	o<<"1 "<<pt;

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
    		if(DEBUG_COMM) cout<<"Found "<<pt<<" in table w/ v="<<v<<" nbits="<<nbits<<" index="<<early_index<<endl;
    		v = (v<<num_bits) | early_index;  
    		return make_pair((v<<(64-nbits)) | (mask<<(64-num_bits-nbits)),
    			early_version);
    	}
        for (;;) {
        	if(DEBUG) cout<<"Do lookup for "<<pt<<" with v: "<<v<<" nbits: "<<nbits<<endl;
			//vector<V> xct = s->lookup(v, nbits);
        	o.str("");
        	o<<"2 "<<v<<" "<<nbits;
        	msg = o.str();
        	if(DEBUG_COMM) cout<<"Sending msg: "<<msg<<endl;
			send(hsock, msg.c_str(), msg.size(), 0);
			memset(buffer, 0, 1024);
			recv(hsock, buffer, 1024, 0);
			if(DEBUG_COMM) cout<<"Received during iterative lookup: "<<buffer<<endl;
			string check(buffer);        	

			if(check=="ope_fail"){
				return insert(v, nbits, pt);

			}				

			istringstream iss_tmp(buffer);
			vector<V> xct_vec;
			V xct, last;
			last = (V) NULL;
			while(true){
				iss_tmp >> xct;
				if(last==xct) break;
				last = xct;

				xct_vec.push_back(xct);
			}


			//Throws ope_lookup_failure if xct size < N, meaning can insert at node
			int pi;
			try{
				pi = predIndex(xct_vec, pt);
			}catch(ope_lookup_failure&){
				return insert(v, nbits, pt);
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
        return make_pair(0,0);
		//FL todo: s->update_table(block_encrypt(pt),v,nbits);
/*		if(DEBUG) cout<<"Encryption of "<<pt<<" has v="<< v<<" nbits="<<nbits<<endl;
        return (v<<(64-nbits)) | (mask<<(64-num_bits-nbits));*/
    }
    
};
