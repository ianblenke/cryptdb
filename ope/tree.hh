#pragma once

/***************
 * Pure virtual classes which are
 * interfaces for
 * for a generic search tree.
 **************/

#include <util/ope-util.hh>
#include <ope/opetable.hh>
#include <edb/Connect.hh>
#include <vector>


class TreeNode {
public:    
    virtual std::vector<std::string> get_keys() = 0;
    
    /* Returns the subtree at index,
     * or NULL if there is no such subtree */
    virtual TreeNode * get_subtree(uint index) = 0;
    
    virtual ~TreeNode() {}
};    

class Tree {
public:

    virtual TreeNode * get_root() = 0;

    /* Inserts ciph in the tree; the position of insert is given node at
     * position indicated by index.
     * Index is: consider all keys at node. Index is position w.r.t them (e.g.,
     * 0 before all of them)
     * ope_path and nbits indicate path to this node in the tree.
     * Requires that node has no subtree at index.
     * Updates ope_table and DB. 
     */ 
    virtual void insert(std::string ciph,
			TreeNode * node,
			OPEType ope_path, uint64_t nbits, uint64_t index) = 0;

};


