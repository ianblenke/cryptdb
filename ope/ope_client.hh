#pragma once

#include <crypto/blowfish.hh>
//#include <util/static_assert.hh>
#include <iostream>
#include <utility>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sstream>

using namespace std;

template<class V, class BlockCipher>
class ope_client {
 public:
    int hsock;

    ope_client(BlockCipher *bc) : b(bc){
//        _static_assert(BlockCipher::blocksize == sizeof(V));
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

    V decrypt(uint64_t ct) const {
/*	uint64_t nbits = 64 - ffsl((uint64_t)ct);
	
        return block_decrypt(s->lookup(ct>>(64-nbits), nbits));*/
	
    }

/*Server protocol code:
 * lookup(encrypted_plaintext) = 1
 * lookup(v, nbits) = 2
 * insert(v, nbits, encrypted_laintext) = 3
*/

    pair<uint64_t, V>  encrypt(V pt) const {
	//s->lookup(block_encrypt(pt)
	char buffer[1024];
	memset(buffer, '\0', 1024);
	ostringstream o;
	o<<"1 "<< block_encrypt(pt);
	string str= o.str();
	cout<<"Sending str: "<<str<<endl;
	send(hsock, str.c_str(), str.size(),0);
	recv(hsock,buffer, 1024, 0);
	uint64_t early_rtn_val_first, early_rtn_val_second;

	istringstream iss(buffer);
	iss>>early_rtn_val_first;
	iss>>early_rtn_val_second;
//	pair<uint64_t, uint64_t> early_rtn_val = s->lookup(block_encrypt(pt));
	cout<<"Return:"<<early_rtn_val_first<<" : "<<early_rtn_val_second<<endl;
	cout<<"Bool"<<(early_rtn_val_first==(uint64_t)-1)<<endl;
	if(early_rtn_val_first!=((uint64_t)-1) && early_rtn_val_second!=((uint64_t)-1)) {
		cout<<"Found match from table, nice!"<<endl;
		return make_pair((early_rtn_val_first<<(64-early_rtn_val_second)) |(1ULL<<(63-early_rtn_val_second)),
			block_encrypt(pt));
	}
        uint64_t v = 0;
        uint64_t nbits = 0;
        for (;;) {
		o.str("");
		o<<"2 "<<v<<" "<<nbits;
		str = o.str();
		cout<<"Sending str: "<<str<<endl;
		send(hsock, str.c_str(), str.size(), 0);
		memset(buffer, 0, 1024);
		recv(hsock, buffer, 1024, 0);
		cout<<"Received during iterative lookup: "<<buffer<<endl;
		string check(buffer);
		if(check=="ope_fail"){
		    cout<<pt<<"  not in tree. "<<nbits<<": " <<block_encrypt(pt)<<" v: "<<v<<endl;
	
		    o.str("");
		    o<<"3 "<<v<<" "<<nbits<<" "<<block_encrypt(pt);
		    str = o.str();
		    cout<<"Sending str: "<<str<<endl;
		    send(hsock, str.c_str(), str.size(),0);
	//            s->insert(v, nbits, block_encrypt(pt));
		    //relabeling may have been triggered so we need to lookup value again
		    //todo: optimize by avoiding extra tree lookup
		    return encrypt(pt);

		}
		istringstream iss_tmp(buffer);
		V xct;
		iss_tmp>>xct;
		cout<<"Received ciphertext node: "<<xct<<endl;
		V xpt = block_decrypt(xct);
		
                if (pt == xpt) {
		    break;
		}
                if (pt < xpt) {
                    v = (v<<1) | 0;
		}
                else {
                    v = (v<<1) | 1;
		}
                nbits++;
		cout<<"v: "<<v<<" ;nbits: "<<nbits<<endl;
        }
        //assert(nbits <= 63);
//	s->update_table(block_encrypt(pt),v,nbits);
//	send(hsock,"0",1,0);
	cout<<"Encryption of "<<pt<<" has v="<< v<<endl;;
        return make_pair((v<<(64-nbits)) | (1ULL<<(63-nbits)),(uint64_t) block_encrypt(pt));
    }
    



 private:
    struct sockaddr_in my_addr;
   
    V block_decrypt(V ct) const {
        V pt;
	pt=ct;
//        b->block_decrypt((const uint8_t *) &ct, (uint8_t *) &pt);
        return pt;
    }

    V block_encrypt(V pt) const {
        V ct;
	ct=pt;
//        b->block_encrypt((const uint8_t *) &pt, (uint8_t *) &ct);
        return ct;
    }

    BlockCipher *b;
//    ope_server<V> *s;
};



class not_a_cipher{

	public:
	not_a_cipher(){}
	~not_a_cipher(){}

	void block_decrypt(const uint8_t* ct, uint8_t * pt){
		*pt=*ct;
	}

	void block_encrypt(const uint8_t* pt, uint8_t * ct){
		*ct=*pt;
	}
};
