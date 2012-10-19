#pragma once

/***************
 * Pure virtual classes which are
 * interfaces for
 * for a generic search tree.
 **************/

#include <util/ope-util.hh>
#include <ope/opetable.hh>
#include <edb/Connect.hh>


template <class EncT>
class TreeNode {
public:    
    virtual std::vector<EncT> get_keys() = 0;
    
    /* Returns the subtree at index,
     * or NULL if there is no such subtree */
    virtual TreeNode * get_subtree(uint index) = 0;
    
};    

template<class EncT>
class Tree {
public:

    virtual TreeNode<EncT> * get_root() = 0;

    /* Inserts ciph in the tree; the position of insert is at given node, at
     * position indicated by index.
     * ope_path and nbits indicate path to this node in the tree.
     * Requires that node has no subtree at index.
     * Updates ope_table and DB. 
     */ 
    virtual void insert(EncT ciph,
			OPEType ope_path, uint64_t nbits, uint64_t index) = 0;

};


template class Tree<uint64_t>;
