#pragma once

/***********************************************
 *
 * File contains scapegoat tree implementation.
 *
 **********************************************/

#include <util/net.hh>
#include <util/ope-util.hh>
#include <edb/Connect.hh>
#include <ope/opetable.hh>

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
    void insert(uint64_t v, uint64_t nbits, uint64_t index, EncT encval,
		OPETable<EncT> & ope_table);


    /*********** Helper methods **************/
    
    std::vector<tree_node<EncT>* > tree_insert(tree_node<EncT>* node,
					       uint64_t v, uint64_t nbits, uint64_t index,
					       EncT encval, uint64_t pathlen,
					       OPETable<EncT>  & ope_table,
	                                       bool & node_inserted);

    tree_node<EncT>* findScapegoat( std::vector<tree_node<EncT>* > path,
				    uint64_t & path_index);

    void print_tree();

    
    void update_shifted_paths(uint index, uint64_t v, int pathlen,
			      tree_node<EncT> * node,
			      OPETable<EncT> & ope_table);
    void update_opetable_db(tree_node<EncT>* node, 
			  uint64_t base_v, uint64_t base_nbits,
			  OPETable<EncT>  & ope_table);
    void update_db(OPEType old_entry, OPEType new_entry);
    void delete_db(table_entry del_entry);
    void clear_db_version();


    std::vector<EncT> flatten(tree_node<EncT>* node);
    tree_node<EncT>* rebuild(std::vector<EncT> key_list);
    void rebalance(tree_node<EncT>* node, uint64_t v,
		   uint64_t nbits, uint64_t path_index,
		   OPETable<EncT>  & ope_table);

    void delete_nodes(tree_node<EncT>* node);

    successor<EncT> find_succ(tree_node<EncT>* node, uint64_t v, uint64_t nbits);
    predecessor<EncT> find_pred(tree_node<EncT>* node, uint64_t v, uint64_t nbits);
    //EncT tree_delete(tree_node<EncT>* node,
    //                 uint64_t v, uint64_t nbits, uint64_t index, uint64_t
    //                 pathlen,
    //                 bool swap);
    //std::string delete_index(uint64_t v, uint64_t nbits, uint64_t index);
    

    /************ Test methods ******************/
    
    bool test_tree(tree_node<EncT>* cur_node);
    bool test_node(tree_node<EncT>* cur_node);
    bool test_vals(tree_node<EncT>* cur_node, EncT low, EncT high);

};

template<class EncT>
struct tree_node
{
    uint num_nodes; //the number of nodes in the subtree rooted in this node -->
		    //these are tree nodes, not keys
    std::vector<EncT> keys;
    std::map<EncT, tree_node *> right; // right has one more element than keys, the 0
				  // element represents the leftmost subtree

    tree_node(){
	num_nodes = 1;
    }

    ~tree_node(){
	keys.clear();
	right.clear();
    }

    // Returns the subtree to the right of key if there is such a subtree, else
    // returns null 
    tree_node<EncT> * has_subtree(EncT key);

    // returns the subtree at position index
    // or null if it does not exist
    tree_node<EncT> * get_subtree(uint index);

    //creates a new subtree at index
    // asserts the subtree does not exist
    tree_node<EncT> * new_subtree(uint index);
    
    // Calculate height of node in subtree of current node 
    // height is defined as the no. of nodes on the longest path
    int height();

    //Recursively calculate number of keys at node and subtree nodes
    int size();

};
