#include <iostream>
#include "ope_client.hh"

int main(){
	cout<<"Starting scapegoat tree test"<<endl;

	not_a_cipher nac = not_a_cipher();
//	ope_server<uint64_t> server = ope_server<uint64_t>();
	ope_client<uint64_t,not_a_cipher> client = ope_client<uint64_t, not_a_cipher>(&nac/*, &server*/);

	int i;
	while(true){
		cout<<"Enter a value: ";
		cin>>i;
		if(i==-1){
			send(client.hsock, "0",1,0);
			close(client.hsock);
			break;
		}else{
			cout<<"Encrypting "<<i<<" to "<<client.encrypt(i)<<endl;
		}

	}
/*
	uint64_t insert_array[] = {2,1,6,5,4,3,15,12,7,9,11,10,13,14,16,17,18};
	for(uint64_t i=0; i<17; i++){
		cout<<"Ciphertext="<<client.encrypt(insert_array[i])<<endl;
	}
	for(uint64_t i=0; i<17; i++){
		client.encrypt(insert_array[i]);
	}
	print_tree(server.root);
	for(uint64_t i=0; i<17;i++){
		cout<<insert_array[i]<<": "<<server.ope_table[insert_array[i]].first.first<<", "<<server.ope_table[insert_array[i]].second<<endl;
	}
	for(uint64_t i=0; i<17; i++){
		cout<<"Ciphertext="<<client.encrypt(insert_array[i])<<endl;
	}
*/
//	server.print_table();	
}
