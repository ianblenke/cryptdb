#pragma once

#include <algorithm>
#include <vector>
#include <util/net.hh>
#include <cmath>
#include <stdint.h>
#include <iostream>
#include <map>
#include <edb/Connect.hh>
#include "btree.hh"
#include <util/ope-util.hh>


// OPE encoding v||index|| 
struct table_storage {
    uint64_t v; // v should now be everything
  
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
    unsigned int num_nodes;
    unsigned int max_size;
    Connect * dbconnect;
    //int global_version;
    int num_rebalances;

#if MALICIOUS
    RootTracker tracker;
#endif


/////////TO DOOOOO: Switch index to int, no need for uint64_t
    std::string insert(uint64_t v, uint64_t nbits, uint64_t index, EncT encval);
    std::vector<tree_node<EncT>* > tree_insert(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, EncT encval, uint64_t pathlen);

    tree_node<EncT>* findScapegoat( std::vector<tree_node<EncT>* > path );

    void print_tree();

    tree_node<EncT>* tree_lookup(tree_node<EncT> *root, uint64_t v, uint64_t nbits) const;
    std::vector<EncT> lookup(uint64_t v, uint64_t nbits) const;
    table_storage lookup(EncT xct);

    std::map<EncT, table_storage > ope_table;
    std::map<EncT, int> ref_table;
    void update_ope_table(tree_node<EncT>* node, table_storage base);
    void update_db(table_storage old_entry, table_storage new_entry);
    void delete_db(table_storage del_entry);
    void clear_db_version();


    std::vector<EncT> flatten(tree_node<EncT>* node);
    tree_node<EncT>* rebuild(std::vector<EncT> key_list);
    void rebalance(tree_node<EncT>* node);
    void delete_nodes(tree_node<EncT>* node);

    successor<EncT> find_succ(tree_node<EncT>* node, uint64_t v, uint64_t nbits);
    predecessor<EncT> find_pred(tree_node<EncT>* node, uint64_t v, uint64_t nbits);
    EncT tree_delete(tree_node<EncT>* node, uint64_t v, uint64_t nbits, uint64_t index, uint64_t pathlen, bool swap);
    std::string delete_index(uint64_t v, uint64_t nbits, uint64_t index);

    tree();
    ~tree();

    bool test_tree(tree_node<EncT>* cur_node);

    bool test_node(tree_node<EncT>* cur_node);

    bool test_vals(tree_node<EncT>* cur_node, EncT low, EncT high);


};


