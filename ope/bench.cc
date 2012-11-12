#include <algorithm>
#include <edb/Connect.hh>
#include <crypto/blowfish.hh>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <ope/client.hh>

#include <util/util.hh>

using namespace std; 

static void
bench_passive_enc(uint num_vals) {

    cerr << "comparison encryption of " << num_vals << " random vals\n";

    
    blowfish* bc = new blowfish(passwd);
    ope_client<uint64_t, blowfish> * my_client =  new ope_client<uint64_t, blowfish>(bc, false);    
    
    uint64_t ope_encodings[num_vals];

    Timer t;

    for(uint i = 0; i < num_vals; i++){
	uint64_t val = (uint64_t) rand();
	uint64_t ct = 0;
	bc->block_encrypt((const uint8_t * ) & val, (uint8_t *) &ct);
	
	ope_encodings[i] = my_client->encrypt(ct, 1);
    }
    
    uint mspassed = t.lap_ms();

    cerr << "time per insert " << mspassed*1.0/num_vals << "\n";
    
    cerr << "ope encodings ";
    for (uint i = 0 ; i < num_vals; i++) {
	cerr << ope_encodings[i] << " ";
    }
    cerr << "\n";
    
}


int main(){

    bench_passive_enc(10);

    return 0;
}
