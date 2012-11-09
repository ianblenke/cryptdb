#include "client.hh"

#include <signal.h>

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

int main(int argc, char ** argv){

    
    blowfish * bc = new blowfish(passwd);
    
    ope_client<uint64_t, blowfish> * ope_cl = new ope_client<uint64_t, blowfish>(bc);
    

    ope_cl->encrypt(5, true);
}

