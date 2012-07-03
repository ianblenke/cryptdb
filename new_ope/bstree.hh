#pragma once

#include "bstnode.hh"


class BSTClass
{
    
public:
    BSTClass(void);
    ~BSTClass(void);
    int NumItems(void) const;

    // returns the max height over all nodes in the tree
    // the height of a node is the number of edges from root to that node
    uint maxHeight() const;
    
    bool Empty(void) const;
    void Insert(const NodeData * data);
    //  Some sort of Remove method could also be added, but
    //  it would require effort to remake the binary search tree
    //  after the deletion of a node.
    BSTNode * Find(const NodeData * data) const;
    void Print(void) const;
    friend class AVLClass;
    
private:
    BSTNode * GetNode(const NodeData * data,
		       BSTNode * LeftPtr = NULL, BSTNode * RightPtr = NULL);
    void FreeNode(BSTNode * NodePtr);
    void ClearTree(void);
    void ClearSubtree(BSTNode * Current);
    BSTNode * SubtreeFind(BSTNode * Current,
			   const NodeData * data) const;
    void PrintSubtree(BSTNode * NodePtr, int Level) const;
    // The following data fields could be made protected instead of
    // private.  This would make them accessible to the derived AVLClass
    // without making AVLClass a friend of BSTClass.
    BSTNode * Root;
    int Count;

};




