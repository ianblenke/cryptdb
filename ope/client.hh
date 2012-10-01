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
#define DEBUG_COMM 0
#define DEBUG_BTREE 0
#define MALICIOUS 0

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::vector;
using std::cerr;
using std::max;

const int N = 4;
const double alpha = 0.3;

Connect* dbconnect;

// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

uint64_t mask;

uint64_t make_mask();
bool test_order(int num_vals, int sorted, bool deletes);

void handle_udf(void* lp);

class ope_lookup_failure {};

#if MALICIOUS
    string cur_merkle_hash;
#endif

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
#if MALICIOUS        
        cur_merkle_hash="";
#endif

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

    V decrypt(uint64_t ct) const;

    void delete_value(V pt);

    //Function to tell tree server to insert plaintext pt w/ v, nbits
    void insert(uint64_t v, uint64_t nbits, uint64_t index, V pt, V det) const;

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
    uint64_t encrypt(V det, bool imode) const;

private:
    V block_decrypt(V ct) const;

    V block_encrypt(V pt) const;

    BlockCipher *b;
    
};
