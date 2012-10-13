#pragma once

#include <stdint.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <map>

#include <util/net.hh>
#include <util/ope-util.hh>
#include <edb/Connect.hh>
#include <ope/btree.hh>



// OPE encoding v||index|| 
struct table_entry {
    uint64_t ope; // v should now be everything
  
    uint64_t refcount;
    //int version;
};

class ope_lookup_failure {};

template<class EncT>
struct tree_node;

template<class EncT>
struct predecessor;

template<class EncT>
struct successor;

template<class EncT>
class tree {

public:

    tree_node<EncT> *root;
    unsigned int num_nodes; //number of tree nodes, not values
    unsigned int max_size;
    Connect * dbconnect;
    //int global_version;
    int num_rebalances;

#if MALICIOUS
    RootTracker tracker;
#endif

    /************ Main methods *****************/
    
    tree();
    ~tree();

    /* Inputs: v is path to node where encval should be inserted, nbits is the
     * length of the path; index is 
     * the position where it should be inserted
     * Requires that encval is not in tree.
     */
    void insert(uint64_t v, uint64_t nbits, uint64_t index, EncT encval, std::map<EncT, table_entry >  & ope_table);



    /*********** Helper methods **************/
    
    std::vector<tree_node<EncT>* > tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, EncT encval, uint64_t pathlen, std::map<EncT, table_entry >  & ope_table);

    tree_node<EncT>* findScapegoat( std::vector<tree_node<EncT>* > path , uint64_t & path_index);

    void print_tree();

    void update_ope_table(tree_node<EncT>* node, uint64_t base_v, uint64_t base_nbits, std::map<EncT, table_entry >  & ope_table);
    void update_db(table_entry old_entry, table_entry new_entry);
    void delete_db(table_entry del_entry);
    void clear_db_version();


    std::vector<EncT> flatten(tree_node<EncT>* node);
    tree_node<EncT>* rebuild(std::vector<EncT> key_list);
    void rebalance(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t path_index,  std::map<EncT, table_entry >  & ope_table);
    void delete_nodes(tree_node<EncT>* node);

    successor<EncT> find_succ(tree_node<EncT>* node, uint64_t v, uint64_t nbits);
    predecessor<EncT> find_pred(tree_node<EncT>* node, uint64_t v, uint64_t nbits);
    //EncT tree_delete(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, uint64_t pathlen, bool swap);
    //std::string delete_index(uint64_t v, uint64_t nbits, uint64_t index);

    /************ Test methods ******************/
    
    bool test_tree(tree_node<EncT>* cur_node);

    bool test_node(tree_node<EncT>* cur_node);

    bool test_vals(tree_node<EncT>* cur_node, EncT low, EncT high);


};


template<class EncT>
class Server {
public:
    int sock_cl; //socket to client; server connects thru it
    int sock_udf; //socket to udfs; server listens on it

    tree<EncT> ope_tree;
    std::map<EncT, table_entry > ope_table;
    
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

   

