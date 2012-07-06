#include <iostream>
#include "client.hh"
#include <sstream>

using namespace std;

int main(){
        mask=make_mask();
        ope_client<uint64_t>* client = new ope_client<uint64_t>();

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

        }

}

