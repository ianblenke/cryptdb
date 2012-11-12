#pragma once


#include <exception>
#include <utility>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <sys/types.h>

#include <edb/Connect.hh>
#include <ope/tree.hh>
#include <ope/stree.hh>
#include <ope/btree.hh>




class Server {
public:
    int sock_cl; //socket to client; server sends request to clients thru it
    int sock_req; //socket on which server receives requests from udfs or clients

    Tree * ope_tree;
    bool MALICIOUS;

    OPETable<std::string> * ope_table;
    Connect * db; 
   
    Server(bool malicious);
    ~Server();

    void work();
    
    /*************** helper functions *****************************/

    void handle_enc(int csock, std::istringstream & iss, bool do_ins);

    /*
     * Given ciph, interacts with client and returns
     * a pair (node, index of subtree of node) where node should be inserted
     * ope path of node, nbits being bits ob this path,
     * a flag, equals, indicating if node is the element at index is equal to
     * underlying val of ciph
     * Returns the tree last tree node on the path. 
     */
    TreeNode * interaction(std::string ciph,
		     uint & rindex, uint & nbits,
		     uint64_t & ope_path,
		     bool & requals);

    /* Dispatched messages the server received to their handlers. */
    void dispatch(int csock, std::istringstream & iss);

};

   

