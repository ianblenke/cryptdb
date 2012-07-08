#include <assert.h>
#include <string.h>
#include <string>
#include "ope_server.hh"
#include <iostream>
#include <cmath>
#include <sstream>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>
#include <sys/types.h>

template<class EncT>
struct tree_node {
    EncT enc_val;
    tree_node *left;
    tree_node *right;

    tree_node(const EncT &ev) : enc_val(ev), left(0), right(0) {}
    ~tree_node() {
       if (left)
            delete left;
        if (right)
            delete right;
    }

    uint64_t height() {
        uint64_t h = 0;
        if (left)
            h = max(h, 1 + left->height());
        if (right)
            h = max(h, 1 + right->height());
        return h;
    }

    uint64_t count() {
        uint64_t n = 1;
        if (left)
            n += left->count();
        if (right)
            n += right->count();
        return n;
    }
};

template<class EncT>
std::string print(tree_node<EncT> * n) {
    if (!n) {
        return "NULL";
    } else {
        std::stringstream ss;
        ss << n->enc_val;
        return ss.str();
    }
}

template<class EncT>
void
ope_server<EncT>::print_tree(tree_node<EncT> * n) {
    if (n) {
        std::cerr << "node " << n->enc_val << " has left " << print(n->left) << " and right " << print(n->right) << "\n";
        print_tree(n->left);
        print_tree(n->right);
    }
}

template<class EncT>
tree_node<EncT> *
ope_server<EncT>::tree_lookup(tree_node<EncT> *root, uint64_t v, uint64_t nbits) const
{
    if (nbits == 0) {
        return root;
    }

    if (!root) {
        return 0;
    }

    return tree_lookup((v&(1ULL<<(nbits-1))) ? root->right : root->left, v, nbits-1);
}

/////////////////////////////////////////////////////

/*
 * Rivest and Galperin's tree balancing: log n memory 
 */

template<class EncT>
static tree_node<EncT> *
flatten(tree_node<EncT> * x, tree_node<EncT> * y) {
    //cerr << "flatten x " << print(x) << " y " << print(y) << "\n";
    if (!x) {
        return y;
    }
    x->right = flatten(x->right, y);
    return flatten(x->left, x);
}

template<class EncT>
static tree_node<EncT> *
build_tree(uint64_t n, tree_node<EncT> * x) {
    // std::cerr << "build tree n " << n << " x " << print(x) << "\n";
    if (n == 0) {
        x->left = NULL;
        return x;
    }
    tree_node<EncT> * r = build_tree(ceil(1.0 * (n-1)/2.0), x);
    tree_node<EncT> * s = build_tree(floor(1.0 * (n-1)/2.0), r->right);

    r->right = s->left;
    s->left = r;

    return s;
}

template<class EncT>
void
ope_server<EncT>::relabel(tree_node<EncT> * parent, bool isLeft, uint64_t size) {

    tree_node<EncT> * scapegoat;
    if (parent == NULL) {
        scapegoat = root;
    } else {
        scapegoat = (isLeft == 1) ? parent->left : parent->right;
    }

    tree_node<EncT> * w = new tree_node<EncT>(0);
    tree_node<EncT> * z = flatten(scapegoat, w);
    build_tree(size, z);

    if (parent) {
        if (isLeft) {
            parent->left = w->left;
        } else {
            parent->right = w->left;
        }
    } else {
        //root has changed
        root = w->left;
    }

    w->left = 0;    /* Something seems fishy here */
    delete w;

    update_ope_table(parent);
}

template<class EncT>
void
ope_server<EncT>::update_ope_table(tree_node<EncT> *n){
    uint64_t v=(ope_table[n->enc_val]).first;
    uint64_t nbits = (ope_table[n->enc_val]).second;
    if(n->left!=NULL) {
        std::ostringstream o;

        ope_table[n->left->enc_val]=std::make_pair(((v<<1)|0), nbits+1);

        uint64_t new_v = ope_table[n->left->enc_val].first;
        uint64_t new_nbits = ope_table[n->left->enc_val].second;
        uint64_t new_ct = (new_v<<(64-new_nbits)) | (1ULL<<(63-new_nbits));
        o.str("");
        o <<new_ct;
        std::string new_ct_string = o.str();

        o.str("");
        o << n->left->enc_val;
        std::string det_enc = o.str();

        std::string s0 = "UPDATE emp SET ope_enc='", 
            s1 = "' where det_enc='", 
            s2 = "'";
        std::string query="";
        query+=s0;
        query+=new_ct_string;
        query+=s1;
        query+=det_enc;
        query+=s2;
        dbconnect->execute(query);


        update_ope_table(n->left);
    }
    if(n->right!=NULL){
        ope_table[n->right->enc_val]=std::make_pair( ((v<<1)|1), nbits+1);

        std::ostringstream o;
        uint64_t new_v = ope_table[n->right->enc_val].first;
        uint64_t new_nbits = ope_table[n->right->enc_val].second;
        uint64_t new_ct = (new_v<<(64-new_nbits)) | (1ULL<<(63-new_nbits));
        o.str("");
        o <<new_ct;
        std::string new_ct_string = o.str();

        o.str("");
        o << n->right->enc_val;
        std::string det_enc = o.str();

        std::string s0 = "UPDATE emp SET ope_enc='", s1 = "' where det_enc='", s2 = "'";
        std::string query="";
        query+=s0;
        query+=new_ct_string;
        query+=s1;
        query+=det_enc;
        query+=s2;
        dbconnect->execute(query);

        update_ope_table(n->right);
    }

}
////////////////////////////////////////////////////


template<class EncT>
void
ope_server<EncT>::tree_insert(tree_node<EncT> **np, uint64_t v,
                              const EncT &encval, uint64_t nbits, uint64_t pathlen)
{
    if (nbits == 0) {
        assert(*np == 0);

        tree_node<EncT> *n = new tree_node<EncT>(encval);
        *np = n;
        ope_table[encval]=std::make_pair(v, pathlen);
        update_tree_stats(pathlen);
        if (trigger(pathlen)) {
            bool isLeft;
            uint64_t subtree_size;
            tree_node<EncT> * parent = node_to_balance(v, pathlen, isLeft, subtree_size);
            std::cout<<"Rebalancing: "<<n->enc_val<<" : "<<parent->enc_val<<std::endl;
            relabel(parent, isLeft, subtree_size);
            std::cout<<encval<<": "<<ope_table[encval].first<<", "<<ope_table[encval].second<<std::endl;
        } else {
                std::cout<<"No rebalance"<<std::endl;
        }

    } else {
        assert(*np);
        tree_insert((v&(1ULL<<(nbits-1))) ? &(*np)->right : &(*np)->left,
                    v, encval, nbits-1, pathlen);
    }
}

static bool
isWeightUnbalanced(uint64_t left, uint64_t right, double alpha) {

    uint64_t total = left + 1 + right;
    if ((left > total * alpha) || (right > total * alpha)) {
        return true;
    }

    return false;
}

template<class EncT>
static tree_node<EncT> *
unbalanced_node(tree_node<EncT> * parent, tree_node<EncT> * current,
                const uint64_t v, const uint64_t nbits,const double alpha,
                bool & isLeft, uint64_t & child_size, bool & found, uint64_t & total_size) {

    if (nbits == 0) {//found v node
        child_size = 1;
        return current;
    }

    if (!current)  {//v does not exist
        return NULL;
    }

    //next child to go to in search for v
    tree_node<EncT> * child = (v&(1ULL<<(nbits-1))) ? current->right : current->left;

    tree_node<EncT> * res = unbalanced_node(current, child,
                                            v, nbits-1, alpha,
                                            isLeft, child_size, found, total_size);
    if (res == NULL) {
        //search failed
        return res;
    }
    if (found) {//the unbalanced node has been found
        return res;
    }
    //unbalanced node not yet found: test if current node is unbalanced
    uint64_t other_child_size = 0;
    if (child == current->right) {
        other_child_size = (current->left == NULL) ? 0 : current->left->count();
    } else {
        other_child_size = (current->right == NULL) ? 0 : current->right->count();
    }

    if (isWeightUnbalanced(child_size, other_child_size, alpha)) {
        found = true;
        total_size = child_size + other_child_size + 1;
        if (parent != NULL) {
            isLeft = (parent->left == current);
        }
        return parent;
    } else {
        //this node is balanced, recursion takes us up
        child_size = child_size + 1 + other_child_size;
        return res;
    }

}


template<class EncT>
tree_node<EncT> *
ope_server<EncT>::node_to_balance(uint64_t v, uint64_t nbits,  bool & isLeft, uint64_t & subtree_size)
{
    bool found = false;
    uint64_t child_size = 0;

    tree_node<EncT> * unb = unbalanced_node((tree_node<EncT> *)0, root,
                                            v, nbits, scapegoat_alpha,
                                            isLeft, child_size, found, subtree_size);

    assert(found); //math guarantees an unbalanced node on v's path to root exists

    return unb;
}


template<class EncT>
void
ope_server<EncT>::update_tree_stats(uint64_t path_len)
{
    num_nodes++;
}


template<class EncT>
bool
ope_server<EncT>::trigger(uint64_t path_len) const
{
    //basic scapegoat trigger
    if ((path_len > 1) && (path_len > log(num_nodes)/log(1/scapegoat_alpha) + 1)) {
        return true;
    } else {
        return false;
    }

}

template<class EncT>
EncT
ope_server<EncT>::lookup(uint64_t v, uint64_t nbits) const
{
    auto
    n = tree_lookup(root, v, nbits);
    if (!n) {
        throw ope_lookup_failure();
    }
    return n->enc_val;

}
template<class EncT>
std::pair<uint64_t, uint64_t>
ope_server<EncT>::lookup(EncT xct){
    if(ope_table.find(xct)!=ope_table.end()){
        std::cout <<"Found "<<xct<<" in table with v="<<ope_table[xct].first<<" nbits="<<ope_table[xct].second<<std::endl;
        return ope_table[xct];
    }
    std::cout<<"Didn't find "<<xct<<" in table"<<std::endl;
    return std::make_pair(-1,-1);
}

template<class EncT>
void
ope_server<EncT>::print_table(){
/*      for(hashtable::iterator it = ope_table.begin(); it!=ope_table.end(); it++){
                std::cout<<it->first<<" : ("<<(it->second)->first<<", "<<(it->second)->second<<")"<<std::endl;
        }*/
}

template<class EncT>
void
ope_server<EncT>::print_tree(){
        print_tree(root);
}

template<class EncT>
void
ope_server<EncT>::insert(uint64_t v, uint64_t nbits, const EncT &encval)
{
    tree_insert(&root, v, encval, nbits, nbits);

}

template<class EncT>
ope_server<EncT>::ope_server()
{
    root = NULL;
    max_height = 0;
    num_nodes = 0;
    dbconnect =new Connect( "localhost", "frank", "passwd","cryptdb", 3306);

}

template<class EncT>
ope_server<EncT>::~ope_server()
{
    if (root)
        delete root;
}

/*
 * Explicitly instantiate the ope_server template for various ciphertext types.
 */
template class ope_server<uint64_t>;
template class ope_server<uint32_t>;
template class ope_server<uint16_t>;

template<class EncT>
void handle_client(void* lp, ope_server<EncT>* s){
        int *csock = (int*) lp;

        std::cout<<"Call to function!"<<std::endl;
        while(true){
                char buffer[1024];
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
                        std::pair<uint64_t,uint64_t> table_rslt = s->lookup((uint64_t) blk_encrypt_pt);
                        std::cout<<"Rtn b/f ostringstream: "<<table_rslt.first<<" : "<<table_rslt.second<<std::endl;
                        o<<table_rslt.first<<" "<<table_rslt.second<<std::endl;
                        rtn_str=o.str();
                        std::cout<<"Rtn_str: "<<rtn_str<<std::endl;
                        send(*csock, rtn_str.c_str(), rtn_str.size(),0);
//                      if(table_rslt.first!=((uint64_t)-1) && table_rslt.second!=((uint64_t)-1)) break;
                }else if(func_d==2){
                        uint64_t v, nbits;
                        iss>>v;
                        iss>>nbits;
                        std::cout<<"Trying lookup("<<v<<", "<<nbits<<")"<<std::endl;
                        EncT xct;
                        try{
                                xct = s->lookup(v, nbits);
                        }catch(ope_lookup_failure&){
                                std::cout<<"Lookup fail, need to insert!"<<std::endl;
                                const char* ope_fail = "ope_fail";
                                send(*csock, ope_fail, strlen(ope_fail),0);
                                continue;
                        }
                        o<<xct;
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
                        s->print_tree();
                }else{
                        std::cout<<"Something's wrong!: "<<buffer<<std::endl;
                        break;
                }
        }
        free(csock);
}

int main(){


        ope_server<uint64_t>* server = new ope_server<uint64_t>();

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
        std::cout<<"Done"<<std::endl;

}

