#include "server.hh"
#include <boost/unordered_map.hpp>
#include <vector>
#include <algorithm>
#include <exception>
#include <cmath>
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

//using namespace std;

template<class EncT>
struct tree_node
{
	vector<EncT> keys;
	boost::unordered_map<EncT, tree_node *> right;

	tree_node(){}

	~tree_node(){
		keys.clear();
		right.clear();
	}

	bool key_in_map(EncT key){
	    // FL: EncT is being used as a primitive, so this is fine.
		typename boost::unordered_map<EncT, tree_node *>::iterator it;
		for(it=right.begin(); it!=right.end(); it++){
			if(it->first==key) return true;
		}
		return false;
	}

	int height(){
		typename boost::unordered_map<EncT, tree_node *>::iterator it;
		int max_child_height=0;
		int tmp_height=0;

		for(it=right.begin(); it!=right.end(); it++){
			tmp_height=it->second->height();
			if(tmp_height>max_child_height) max_child_height=tmp_height;
		}		

		return max_child_height+1;

	}

	int len(){
		int len = keys.size();
		typename boost::unordered_map<EncT, tree_node *>::iterator it;

		for(it=right.begin(); it!=right.end(); it++){
			len+=it->second->len();
		}		

		return len;		

	}

};

/****************************/
//Tree rebalancing

template<class EncT>
vector<EncT> 
tree<EncT>::flatten(tree_node<EncT>* node){
	vector<EncT> rtn_keys = node->keys;

	typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

	vector<EncT> child_keys;

	for(it=node->right.begin(); it!=node->right.end(); it++){
		child_keys = flatten(it->second);
		for(int i=0; i<child_keys.size(); i++){
			rtn_keys.push_back(child_keys[i]);
		}
	}	

	return rtn_keys;			

}

template<class EncT>
tree_node<EncT>* 
tree<EncT>::rebuild(vector<EncT> key_list){
	//Avoid building a node without any values
	if(key_list.size()==0) return NULL;

	tree_node<EncT>* rtn_node = new tree_node<EncT>();

	if(key_list.size()<N){
		//Remaining keys fit in one node, return that node
		rtn_node->keys=key_list;
	}else{
		//Borders to divide key_list by
		double base = ((double) key_list.size())/ ((double) N);

		//Fill keys with every base-th key;
		for(int i=1; i<N; i++){
			rtn_node->keys.push_back(key_list[floor(i*base)]);
		}
		//Handle the subvector rebuilding except for the first and last
		for(int i=1; i<N-1; i++){
			vector<EncT> subvector;
			for(int j=floor(i*base)+1; j<floor((i+1)*base); j++){
				subvector.push_back(key_list[j]);
			}
			tree_node<EncT>* tmp_child = rebuild(subvector);
			if(tmp_child!=NULL){
				rtn_node->right[key_list[floor(i*base)]]=tmp_child;
			}
		}
		//First subvector rebuilding special case
		vector<EncT> subvector;
		for(int j=0; j<floor(base); j++){
			subvector.push_back(key_list[j]);
		}
		tree_node<EncT>* tmp_child = rebuild(subvector);
		if(tmp_child!=NULL){
			rtn_node->right[NULL]=tmp_child;
		}	
		subvector.clear();
		//Last subvector rebuilding special case
		for(int j=floor((N-1)*base)+1; j<key_list.size(); j++){
			subvector.push_back(key_list[j]);
		}
		tmp_child = rebuild(subvector);
		if(tmp_child!=NULL){
			rtn_node->right[key_list[floor((N-1)*base)]]=tmp_child;
		}	

	}
	return rtn_node;

}

template<class EncT>
void 
tree<EncT>::rebalance(tree_node<EncT>* node){
	if(DEBUG) std::cout<<"Rebalance"<<std::endl;

	table_storage base = ope_table[node->keys[0]];

	vector<EncT> key_list = flatten(node);
	std::sort(key_list.begin(), key_list.end());
	tree_node<EncT>* tmp_node = rebuild(key_list);
	delete_nodes(node);
	node->keys = tmp_node->keys;
	node->right = tmp_node->right;
	delete tmp_node;

	update_ope_table(node, base);
}

template<class EncT>
void 
tree<EncT>::delete_nodes(tree_node<EncT>* node){
	typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

	for(it=node->right.begin(); it!=node->right.end(); it++){
		delete_nodes(it->second);
		delete it->second;
	}

}

/****************************/

template<class EncT>
void
tree<EncT>::update_ope_table(tree_node<EncT> *node, table_storage base){
	for(int i=0; i<node->keys.size(); i++){
		table_storage tmp = base;
		tmp.index=i;
		ope_table[node->keys[i]] = tmp;
	}
	if(node->key_in_map(NULL)){
		table_storage tmp = base;
		tmp.v = tmp.v<<num_bits;
		tmp.pathlen+=num_bits;
		tmp.index=0;
		update_ope_table(node->right[NULL], tmp);
	}
	for(int i=0; i<node->keys.size(); i++){
		if(node->key_in_map(node->keys[i])){
			table_storage tmp = base;
			tmp.v = tmp.v<<num_bits | (i+1);
			tmp.pathlen+=num_bits;
			tmp.index=0;
			update_ope_table(node->right[node->keys[i]], tmp);	
		}	
	}

}

template<class EncT>
void 
tree<EncT>::print_tree(){
	bool last_was_null = false;
	vector<tree_node<EncT>* > queue;
	queue.push_back(root);
	while(queue.size()!=0){
		tree_node<EncT>* cur_node = queue[0];
		if(cur_node==NULL){
			queue.erase(queue.begin());
			std::cout<<std::endl;
			continue;
		}
		std::cout<<cur_node->height()<<": ";
		if(cur_node->key_in_map(NULL)){
			queue.push_back(cur_node->right[NULL]);
		}	
		for(int i=0; i<cur_node->keys.size(); i++){
			EncT key = cur_node->keys[i];
			std::cout<<key<<", ";
			if(cur_node->key_in_map(key)){
				queue.push_back(cur_node->right[key]);
			}
		}	
		std::cout<<std::endl;
		queue.push_back(NULL);
		queue.erase(queue.begin());
	}

}

template<class EncT>
tree_node<EncT>* 
tree<EncT>::findScapegoat( vector<tree_node<EncT>* > path ){
	int tmp_size = 0;
	tree_node<EncT>* last = NULL;
	typename boost::unordered_map<EncT, tree_node<EncT> *>::iterator it;

	int childSize=0;
	vector<int> childSizes;

	//Move along path until you find the scapegoat node
	for(int i=0; i<path.size(); i++){
		tree_node<EncT>* cur_node = path[i];

		int tmp_sum=0;
		for(it=cur_node->right.begin(); it!=cur_node->right.end(); it++){
			if(it->second!=last){
				childSize = it->second->len();
				tmp_sum+=childSize;
				childSizes.push_back(childSize);
			}else{
				tmp_sum+=tmp_size;
				childSizes.push_back(tmp_size);
			}

		}		
		tmp_size = tmp_sum + cur_node->keys.size();
		last = cur_node;

		for(int i=0; i<childSizes.size(); i++){
			if(childSizes[i] > alpha*tmp_size) return cur_node;
		}

		childSizes.clear();

	}

}

template<class EncT>
void
tree<EncT>::insert(uint64_t v, uint64_t nbits, EncT encval){
	//If root doesn't exsit yet, create it, then continue insertion
	if(!root){
		if(DEBUG) std::cout<<"Inserting root"<<std::endl;
		root=new tree_node<EncT>();
	}
	vector<tree_node<EncT> * > path=tree_insert(root, v, nbits, encval, nbits);
	double height= (double) path.size();

	if (height==0) return;

	if(DEBUG) print_tree();

	if(height> log(num_nodes)/log(((double)1.0)/alpha)+1 ){
		tree_node<EncT>* scapegoat = findScapegoat(path);
		rebalance(scapegoat);
		if(DEBUG) print_tree();
	}
}

template<class EncT>
vector<tree_node<EncT>* >
tree<EncT>::tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, EncT encval, uint64_t pathlen){

	vector<tree_node<EncT>* > rtn_path;

	//End of the insert line, check if you can insert
	//Assumes v and nbits were from a lookup of an insertable node
	if(nbits==0){
		if(node->keys.size()>N-2) std::cout<<"Insert fail, found full node"<<std::endl;
		else{
			if(DEBUG) std::cout<<"Found node, inserting"<<std::endl;
			node->keys.push_back(encval);
			std::sort(node->keys.begin(), node->keys.end());
			num_nodes++;
			rtn_path.push_back(node);

			table_storage new_entry;
			new_entry.v = v;
			new_entry.pathlen = pathlen;
			new_entry.index = 0;
			ope_table[encval] = new_entry;

			for(int i=0; i<node->keys.size(); i++){
				ope_table[node->keys[i]].index = i;
			}
			
		}
		return rtn_path;
	}

    int key_index = (int) (v&(mask<<(nbits-num_bits)))>>(nbits-num_bits);

    EncT key;

    //Protocol set: index 0 is NULL (branch to node with lesser elements)
    //Else, index is 1+(key's index in node's key-vector)
    if(key_index==0) key=NULL;
	else if(key_index<N) key = node->keys[key_index-1];
	else std::cout<<"Insert fail, key_index not legal"<<std::endl;

	//std::cout<<"Key: "<<key<<std::endl;

	//See if pointer to next node to be checked 
    if(!node->key_in_map(key)){
	    if (nbits == num_bits && node->keys.size()==N-1) {
	    	if(DEBUG) std::cout<<"Creating new node"<<std::endl;
	    	node->right[key] = new tree_node<EncT>();
	    }else std::cout<<"Insert fail, wrong condition to create new node child"<<std::endl;
    }
    rtn_path = tree_insert(node->right[key], v, nbits-num_bits, encval, pathlen);
    rtn_path.push_back(node);
    return rtn_path;
}

template<class EncT>
tree_node<EncT> *
tree<EncT>::tree_lookup(tree_node<EncT> *node, uint64_t v, uint64_t nbits) const
{

    if (nbits == 0) {
        return node;
    }

    int key_index = (int) (v&(mask<<(nbits-num_bits)))>>(nbits-num_bits);

    EncT key;

    if(key_index==0) key=NULL;
	else if(key_index<N) key = node->keys[key_index-1];
	else return 0;

    if(!node->key_in_map(key)){
    	if(DEBUG) std::cout<<"Key not in map"<<std::endl;
    	return 0;
    }
    return tree_lookup(node->right[key], v, nbits-num_bits);
}

template<class EncT>
vector<EncT>
tree<EncT>::lookup(uint64_t v, uint64_t nbits) const
{
	tree_node<EncT>* n;
	//Handle if root doesn't exist yet
	if(!root){
		if(DEBUG) std::cout<<"First lookup no root"<<std::endl;
		n=0;
	}else{
		n  = tree_lookup(root, v, nbits);
	}

    if(n==0){
    	if(DEBUG) std::cout<<"NOPE! "<<v<<": "<<nbits<<std::endl;    	
    	throw ope_lookup_failure();
    }
    return n->keys;

}

template<class EncT>
table_storage
tree<EncT>::lookup(EncT xct){
    if(ope_table.find(xct)!=ope_table.end()){
        if(DEBUG) std::cout <<"Found "<<xct<<" in table with v="<<ope_table[xct].v<<" nbits="<<ope_table[xct].pathlen<<" index="<<ope_table[xct].index<<std::endl;
        return ope_table[xct];
    }
    table_storage negative;
    negative.v=-1;
    negative.pathlen=-1;
    negative.index=-1;
    return negative;
}

/*
 * Explicitly instantiate the ope_server template for various ciphertext types.
 */
template class tree<uint64_t>;
template class tree<uint32_t>;
template class tree<uint16_t>;

template<class EncT>
void handle_client(void* lp, tree<EncT>* s){
        int *csock = (int*) lp;

        std::cout<<"Call to function!"<<std::endl;
        char buffer[1024];
        while(true){
                memset(buffer, 0, 1024);
                recv(*csock, buffer, 1024, 0);

                int func_d;
                std::istringstream iss(buffer);
                iss>>func_d;
                std::cout<<"See value func_d: "<<func_d<<std::endl;

                std::ostringstream o;
                std::string rtn_str;
                if(func_d==0){
                         break;
                }else if(func_d==1) {
                        uint64_t blk_encrypt_pt;
                        iss>>blk_encrypt_pt;
                        std::cout<<"Blk_encrypt_pt: "<<blk_encrypt_pt<<std::endl;
                        table_storage table_rslt = s->lookup((uint64_t) blk_encrypt_pt);
                        std::cout<<"Rtn b/f ostringstream: "<<table_rslt.v<<" : "<<table_rslt.pathlen<<" : "<<table_rslt.index<<std::endl;
                        o<<table_rslt.v<<" "<<table_rslt.pathlen<<" "<<table_rslt.index;
                        rtn_str=o.str();
                        std::cout<<"Rtn_str: "<<rtn_str<<std::endl;
                        send(*csock, rtn_str.c_str(), rtn_str.size(),0);
                }else if(func_d==2){
                        uint64_t v, nbits;
                        iss>>v;
                        iss>>nbits;
                        std::cout<<"Trying lookup("<<v<<", "<<nbits<<")"<<std::endl;
                        vector<EncT> xct_vec;
                        try{
                                xct_vec = s->lookup(v, nbits);
                        }catch(ope_lookup_failure&){
                                std::cout<<"Lookup fail, need to insert!"<<std::endl;
                                const char* ope_fail = "ope_fail";
                                send(*csock, ope_fail, strlen(ope_fail),0);
                                continue;
                        }
                        for(int i=0; i<xct_vec.size(); i++){
                        	o<<xct_vec[i];
                        	o<<" ";
                        }
                        rtn_str=o.str();
                        std::cout<<"Rtn_str: "<<rtn_str<<std::endl;
                        send(*csock, rtn_str.c_str(), rtn_str.size(),0);
                }else if(func_d==3){
                        uint64_t v, nbits, blk_encrypt_pt;
                        iss>>v;
                        iss>>nbits;
                        iss>>blk_encrypt_pt;
                        std::cout<<"Trying insert("<<v<<", "<<nbits<<", "<<blk_encrypt_pt<<")"<<std::endl;
                        s->insert(v, nbits, blk_encrypt_pt);
                }else{
                        std::cout<<"Something's wrong!: "<<buffer<<std::endl;
                        break;
                }
        }
        free(csock);
}


int main(){

	mask = make_mask();

	tree<uint64_t>* server = new tree<uint64_t>();

	int host_port = 1111;
    int hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock ==-1){
            std::cout<<"Error initializing socket"<<std::endl;
    }

    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(host_port);
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    int bind_rtn =bind(hsock, (struct sockaddr*) &my_addr, sizeof(my_addr));
    if(bind_rtn<0){
            std::cout<<"Error binding to socket"<<std::endl;
    }
    int listen_rtn = listen(hsock, 10);
    if(listen_rtn<0){
            std::cout<<"Error listening to socket"<<std::endl;
    }

    socklen_t addr_size=sizeof(sockaddr_in);
    int* csock;
    struct sockaddr_in sadr;
    int i=2;
    while(i>0){
            std::cout<<"Listening..."<<std::endl;
            csock = (int*) malloc(sizeof(int));
            if((*csock = accept(hsock, (struct sockaddr*) &sadr, &addr_size))!=-1){
                    handle_client((void*)csock,server);
            }
            else{
                    std::cout<<"Error accepting!"<<std::endl;
            }
            i--;
    }

    close(hsock);

}
