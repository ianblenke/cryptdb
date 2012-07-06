#include <iostream>
#include "client.hh"
#include <sstream>
#include <utility>
#include <time.h>
#include <stdlib.h>

using namespace std;

//0 == random
//1 == sorted incr
//2 == sorted decr
bool test_order(int num_vals, int sorted){

        ope_client<uint64_t>* my_client = new ope_client<uint64_t>();

        vector<uint64_t> inserted_vals;
        srand( time(NULL));
        //srand(0);

        //Build vector of all inserted values
        for(int i=0; i<num_vals; i++){
                inserted_vals.push_back((uint64_t) rand());
        }
        if(sorted>0){
                sort(inserted_vals.begin(), inserted_vals.end());
        }
        if(sorted>1){
                reverse(inserted_vals.begin(), inserted_vals.end());
        }

        //Vector to hold already inserted values
        vector<uint64_t> tmp_vals;

        for(int i=0; i<num_vals; i++){
                uint64_t val = inserted_vals[i];
                tmp_vals.push_back(val);
                uint64_t enc_val = my_client->encrypt(val);

                //Check that the enc_val can be decrypted back to val
                if(val!=my_client->decrypt(enc_val)) {
                        cout<<"Decryption wrong! "<<val<<" : "<<enc_val<<endl;
                        return false;
                }                       
                if(DEBUG) cout<<"Enc complete for "<<val<<" with enc="<<enc_val<<endl;

                //Check that all enc vals follow OPE properties
                //So if val1<val2, enc(val1)<enc(val2)
                sort(tmp_vals.begin(), tmp_vals.end());
                uint64_t last_val = 0;
                uint64_t last_enc = 0;
                uint64_t cur_val = 0;
                uint64_t cur_enc = 0;
                for(int j=0; j<tmp_vals.size(); j++){
                        cur_val = tmp_vals[j];
                        cur_enc = my_client->encrypt(cur_val);          
                        if(cur_val>=last_val && cur_enc>=last_enc){
                                last_val = cur_val;
                                last_enc = cur_enc;
                        }else{
                                cout<<"j: "<<j<<" cur:"<<cur_val<<": "<<cur_enc<<" last: "<<last_val<<": "<<last_enc<<endl;
                                return false;
                        }
                        if(DEBUG) cout<<"Decrypting "<<cur_enc<<endl;
                        if(cur_val!=my_client->decrypt(cur_enc)) {
                                cout<<"No match with "<<j<<endl;
                                return false;
                        }
                }               
        }

        cout<<"Sorted order "<<sorted<<" passes test"<<endl;

        //Kill server
        send(my_client->hsock, "0",1,0);
        close(my_client->hsock);
        delete my_client;

        return true;

}

int main(){
        //Build mask based on N
        mask=make_mask();
        cout<<"Test result: "<<test_order(213,0);
/*        ope_client<uint64_t>* client = new ope_client<uint64_t>();

       int val;
        while(true){
                cout<<"Enter value: ";
                cin>>val;
                if(val==-1){
                        send(client->hsock, "0",1,0);
                        close(client->hsock);
                        break;
                }else{
                        uint64_t enc_val = client->encrypt((uint64_t) val);
                        cout<<"Encrypting "<<val<<" to "<<enc_val<<endl;
                        cout<<"Decrypting "<<enc_val<<" to "<<client->decrypt(enc_val)<<endl;
                }

        }*/

}

