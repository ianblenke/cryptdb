#include <vector>
#include <cmath>
#include <stdint.h>
#include <iostream>
#include <assert.h>
#include <boost/unordered_map.hpp>

//Whether to print debugging output or not
#define DEBUG 0

using namespace std;

const int N = 4;
const double alpha = 0.3;


// for N not a power of two, we want ceil(log_2(N))
// for N power of two we want log_2(N) + 1
const int num_bits = (int) ceil(log2(N+1.0));

uint64_t mask;


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

struct table_storage{
	uint64_t v;
	uint64_t pathlen;
	uint64_t index;
};

class ope_lookup_failure {};

template<class EncT>
struct tree_node;

template<class EncT>
class tree {

public:
	tree_node<EncT> *root; 
	unsigned int num_nodes;

	void insert(uint64_t v, uint64_t nbits, EncT encval);
	vector<tree_node<EncT>* > tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, EncT encval, uint64_t pathlen);

	tree_node<EncT>* findScapegoat( vector<tree_node<EncT>* > path );

	void print_tree();

	tree_node<EncT>* tree_lookup(tree_node<EncT> *root, uint64_t v, uint64_t nbits) const;
	vector<EncT> lookup(uint64_t v, uint64_t nbits) const;
	table_storage lookup(EncT xct);

	boost::unordered_map<EncT, std::pair<uint64_t,uint64_t> > ope_table;
	//void update_ope_table(tree_node<EncT>*n);

	vector<EncT> flatten(tree_node<EncT>* node);
	tree_node<EncT>* rebuild(vector<EncT> key_list);
	void rebalance(tree_node<EncT>* node);
	void delete_nodes(tree_node<EncT>* node);

	tree(){
		num_nodes=0;
		root = NULL;
	}

	~tree(){
		delete_nodes(root);
		delete root;
	}

	bool test_tree(tree_node<EncT>* cur_node);

	bool test_node(tree_node<EncT>* cur_node);

	bool test_vals(tree_node<EncT>* cur_node, EncT low, EncT high);
};


template<class V>
class ope_client {
 public:
    ope_client(tree<V> *server) : s(server) {
//        _static_assert(BlockCipher::blocksize == sizeof(V));
    }

    ~ope_client(){}

    /* Determines the index of pt's predecesor given a node's key vector
     * Returns -1 if pt is in the vector, 0 is if is smaller than all
     * elements in the vector, or 1+index where index is the pred's index
     * in the vector (1 is added due to my protocol, where 0 represents null)
    */
    static int predIndex(vector<V> vec, V pt){
		V tmp_index = 0;
		V tmp_pred_key =0;
		bool pred_found=false;
		for (int i = 0; i<vec.size(); i++)
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
		uint64_t nbits = 64 - ffsl((uint64_t)ct);
		uint64_t v= ct>>(64-nbits); //Orig v
		uint64_t path = v>>num_bits; //Path part of v
		int index = (int) v & mask; //Index (last num_bits) of v
        vector<V> vec = s->lookup(path, nbits-num_bits);
        return vec[index];
	
    }

    /* Encryption is the path bits (aka v) bitwise left shifted by num_bits
     * so that the last num_bits can indicate the index of the value pt at
     * at the node found by the path
     */
    uint64_t encrypt(V pt) const{
        uint64_t v = 0;
        uint64_t nbits = 0;
        try {
            for (;;) {
            	if(DEBUG) cout<<"Do lookup for "<<pt<<" with v: "<<v<<" nbits: "<<nbits<<endl;
				vector<V> xct = s->lookup(v, nbits);
				int pi = predIndex(xct, pt);
				nbits+=num_bits;
		        if (pi==-1) {
		        	//pt already exists at node
		        	int index;
		        	for(index=0; index<xct.size(); index++){
		        		//Last num_bits are set to represent index of pt at node
		        		if(xct[index]==pt) v = (v<<num_bits) | index;
		        	}
				    break;
				}else {
		            v = (v<<num_bits) | pi;
				}
		    }
        } catch (ope_lookup_failure&) {
	    	if(DEBUG) cout<<pt<<"  not in tree. "<<nbits<<": "<<" v: "<<v<<endl;

            s->insert(v, nbits, pt);
	    	//relabeling may have been triggered so we need to lookup value again
	   		// FL todo: optimize by avoiding extra tree lookup
	   		return encrypt(pt);
        }
        //Check that we don't run out of bits!
        if(nbits > 63) {
        	cout<<"nbits larger than 64 bits!!!!"<<endl;
        }
		//FL todo: s->update_table(block_encrypt(pt),v,nbits);
		if(DEBUG) cout<<"Encryption of "<<pt<<" has v="<< v<<" nbits="<<nbits<<endl;
        return (v<<(64-nbits)) | (mask<<(64-num_bits-nbits));
    }

 private:


    tree<V> *s;
};
