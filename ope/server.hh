#pragma once

#include <stdint.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <map>

#include <util/net.hh>
#include <util/ope-util.hh>
#include <edb/Connect.hh>
#include <ope/stree.hh>
#include <ope/btree.hh>



template<class EncT>
class Server {
public:
    int sock_cl; //socket to client; server connects thru it
    int sock_udf; //socket to udfs; server listens on it

    tree<EncT> ope_tree;
    OPETable ope_table;
    
    Server();
    ~Server();

    /*************** helper functions *****************************/

    void handle_enc(int csock, std::istringstream & iss, bool do_ins);

    /*
     * Given ciph, interacts with client and returns
     * a pair (node, index of subtree of node) where node should be inserted
     * ope path of node, nbits being bits ob this path,
     * a flag, equals, indicating if node is the element at index is equal to
     * underlying val of ciph
     */
    void interaction(EncT ciph,
		     uint & rindex, uint & nbits,
		     uint64_t & ope_path,
		     bool & requals);

    /* Dispatched messages the server received to their handlers. */
    void dispatch(int csock, std::istringstream & iss);

};

   

