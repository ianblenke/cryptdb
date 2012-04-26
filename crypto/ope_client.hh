#pragma once

#include "blowfish.hh"
#include "../util/static_assert.hh"
#include "ope_server.hh"
#include <iostream>
#include <utility>

template<class V, class BlockCipher>
class ope_client {
 public:
    ope_client(BlockCipher *bc, ope_server<V> *server) : b(bc), s(server) {
//        _static_assert(BlockCipher::blocksize == sizeof(V));
    }

    V decrypt(uint64_t ct) const {
	uint64_t nbits = 64 - ffsl((uint64_t)ct);
	
        return block_decrypt(s->lookup(ct>>(64-nbits), nbits));
	
    }

    uint64_t encrypt(V pt) const {
	pair<uint64_t, uint64_t> early_rtn_val = s->lookup(block_encrypt(pt));
	if(early_rtn_val.first!=-1 && early_rtn_val.second!=-1) {
		return (early_rtn_val.first<<(64-early_rtn_val.second)) |
			(1ULL<<(63-early_rtn_val.second));
	}
        uint64_t v = 0;
        uint64_t nbits = 0;
        try {
            for (;;) {
		V xct = s->lookup(v, nbits);
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
            }
        } catch (ope_lookup_failure&) {
	    cout<<pt<<"  not in tree. "<<nbits<<": " <<block_encrypt(pt)<<" v: "<<v<<endl;

            s->insert(v, nbits, block_encrypt(pt));
	    //relabeling may have been triggered so we need to lookup value again
	    //todo: optimize by avoiding extra tree lookup
	    return encrypt(pt);
        }
        assert(nbits <= 63);
	s->update_table(block_encrypt(pt),v,nbits);
	cout<<"Ecryption of "<<pt<<" has v="<< v<<endl;;
        return (v<<(64-nbits)) | (1ULL<<(63-nbits));
    }

 private:
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
    ope_server<V> *s;
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
