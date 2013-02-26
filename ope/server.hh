#pragma once


#include <exception>
#include <utility>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <sys/types.h>
#include <map>
#include <list>

#include <edb/Connect.hh>
#include <ope/tree.hh>
#include <ope/stree.hh>
#include <ope/btree.hh>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>



struct tablemeta {
    Tree * ope_tree;
    OPETable<std::string> * ope_table;
    tablemeta(Tree * otree, OPETable<std::string> * otable) {
	ope_tree = otree;
	ope_table = otable;
    }

    ~tablemeta() {
    delete ope_tree;
    delete ope_table;
    }

};

const string auto_table = "opetable";


class Server {
public:
    int sock_cl; //socket to client; server sends request to clients thru it
    int sock_req; //socket on which server receives requests from udfs or clients
    
    bool MALICIOUS;
   
    std::map<string, tablemeta *> tables;
      
    Connect * db; 
    
    Server(bool malicious, int cport = OPE_CLIENT_PORT, int sport = OPE_SERVER_PORT,
	   std::list<string> * tables = NULL,
	   bool db_updates = true);
    ~Server();

    // returns the ope tree of the server and asserts there is just one
    Tree * ope_tree();

    void work();
    
    /*************** helper functions *****************************/

    uint64_t store_tree();

    void handle_enc(int csock, std::istringstream & iss, bool do_ins, string table_name = auto_table);
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
			   bool & requals,
			   Tree * ope_tree);

    /* Dispatched messages the server received to their handlers. */
    void dispatch(int csock, std::istringstream & iss);

    // statistics : number of rewrites done so far
    uint num_rewrites();
    
};

   

