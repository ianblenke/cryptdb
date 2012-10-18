#pragma once

#include <util/ope-util.hh>
#include <ope/stree.hh>
#include <ope/btree.hh>
#include <ope/opetable.hh>
#include <edb/Connect.hh>

enum TreeType {SCAPEGOAT, BTREE};


template <class EncT>
class TreeNode {
    
    virtual std::vector<EncT> get_keys() = 0;
    
    /* Returns the subtree at index,
     * or NULL if there is no such subtree */
    virtual TreeNode * get_subtree(uint index) = 0;
    
};    


template<class EncT>
class Tree {
public:

    /* Creates tree.
       Gets pointers to ope_table and db to keep these updated upon
       ope encodings changing.
     */
    Tree(OPETable<EncT> * ot, Connect * dbcon): ope_table(ot), db(dbcon) {}

    virtual TreeNode<EncT> * get_root();

    /* Inserts ciph in the tree; the position of insert is at given node, at
     * position indicated by index.
     * ope_path and nbits indicate path to this node in the tree.
     * Requires that node has no subtree at index.
     * Updates ope_table and DB. 
     */ 
    virtual void insert(EncT ciph,
		OPEType ope_path, uint64_t nbits, uint64_t index);

 protected:
    OPETable<EncT> * ope_table;
    Connect * db;
};

