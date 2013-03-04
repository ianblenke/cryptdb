#pragma once

/************************
 *
 * Scapegoat tree.
 *
 ***********************/

#include <vector>

#include <util/net.hh>
#include <util/ope-util.hh>
#include <edb/Connect.hh>
#include <ope/opetable.hh>
#include <ope/tree.hh>

class ope_lookup_failure {};


template<class EncT>
struct Stree_node;

template<class EncT>
struct predecessor;

template<class EncT>
struct successor;




template<class EncT>
class Stree : public Tree {

public:

    Stree_node<EncT> * root;
    unsigned int max_size;
    OPETable<EncT> * ope_table;
    Connect * dbconnect;
    int num_rebalances;
 
/*
#if MALICIOUS
    RootTracker tracker;
#endif
*/
    /************ Main methods *****************/
    
    Stree(OPETable<EncT> * ot, Connect * db);
    ~Stree();

    Stree_node<EncT> * get_root();

    /* Inputs: v is path to node where encval should be inserted, nbits is the
     * length of the path; index is 
     * the position where it should be inserted
     * Requires that encval is not in tree.
     */
    void insert(std::string encval, TreeNode * tnode,
		OPEType v, uint64_t nbits, uint64_t index,
		UpdateMerkleProof & p, string rowid = "");
    
    void remove(std::string ciph, TreeNode * tnode,
		OPEType ope_path, uint64_t nbits, uint64_t index,
		UpdateMerkleProof & p, string rowid = "");


    MerkleProof merkle_proof(TreeNode * n);

    uint num_rewrites() {assert_s(false,"unimplemented"); return 0;}

    /*********** Helper methods **************/
    
    std::vector<Stree_node<EncT>* > tree_insert(Stree_node<EncT>* node,
					       uint64_t v, uint64_t nbits, uint64_t index,
						EncT encval, uint64_t pathlen,
						bool & node_inserted);

    Stree_node<EncT>* findScapegoat(std::vector<Stree_node<EncT>* > path,
				    uint64_t & path_index);

    void print_tree();

    
    void update_shifted_paths(uint index, uint64_t v, int pathlen,
			      Stree_node<EncT> * node);
    void update_opetable_db(Stree_node<EncT>* node, 
			  uint64_t base_v, uint64_t base_nbits);
    void update_db(OPEType old_entry, OPEType new_entry);
    void delete_db(table_entry del_entry);
    void clear_db_version();


    std::vector<EncT> flatten(Stree_node<EncT>* node);
    Stree_node<EncT>* rebuild(std::vector<EncT> key_list);
    void rebalance(Stree_node<EncT>* node, uint64_t v,
		   uint64_t nbits, uint64_t path_index);

    void delete_nodes(Stree_node<EncT>* node);

    successor<EncT> find_succ(Stree_node<EncT>* node, uint64_t v, uint64_t nbits);
    predecessor<EncT> find_pred(Stree_node<EncT>* node, uint64_t v, uint64_t nbits);
    //EncT tree_delete(tree_node<EncT>* node,
    //                 uint64_t v, uint64_t nbits, uint64_t index, uint64_t
    //                 pathlen,
    //                 bool swap);
    //std::string delete_index(uint64_t v, uint64_t nbits, uint64_t index);
    

    /************ Test methods ******************/
    
    bool test_tree(Stree_node<EncT>* cur_node);
    bool test_node(Stree_node<EncT>* cur_node);
    bool test_vals(Stree_node<EncT>* cur_node, EncT low, EncT high);

};

template<class EncT>
struct Stree_node : public TreeNode
{
    uint num_nodes; //the number of nodes in the subtree rooted in this node -->
		    //these are tree nodes, not keys
    std::vector<EncT> keys;
    std::map<EncT, Stree_node<EncT> *> right; // right has one more element than keys, the 0
				  // element represents the leftmost subtree

    Stree_node() {
	num_nodes = 1;
    }

    virtual ~Stree_node() {
	keys.clear();
	right.clear();
    }

    //returns set of keys at this node
    std::vector<std::string> get_keys();

    // Returns the subtree to the right of key if there is such a subtree, else
    // returns null 
    Stree_node<EncT> * has_subtree(EncT key);

    // implements the TreeNode interface
    Stree_node<EncT> * get_subtree(uint index);
    string  get_ciph(uint index);
    
    //creates a new subtree at index
    // asserts the subtree does not exist
    Stree_node<EncT> * new_subtree(uint index);
    
    // Calculate height of node in subtree of current node 
    // height is defined as the no. of nodes on the longest path
    int height();

    //Recursively calculate number of keys at node and subtree nodes
    int size();

};

