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
    int maxHeight() const;
    
    bool Empty(void) const;
    void Insert(const ItemType & Item);
    //  Some sort of Remove method could also be added, but
    //  it would require effort to remake the binary search tree
    //  after the deletion of a node.
    BSTNodePtr Find(const ItemType & Item) const;
    void Print(void) const;
    friend class AVLClass;
    
private:
    BSTNodePtr GetNode(const ItemType & Item,
		       BSTNodePtr LeftPtr = NULL, BSTNodePtr RightPtr = NULL);
    void FreeNode(BSTNodePtr NodePtr);
    void ClearTree(void);
    void ClearSubtree(BSTNodePtr Current);
    BSTNodePtr SubtreeFind(BSTNodePtr Current,
			   const ItemType & Item) const;
    void PrintSubtree(BSTNodePtr NodePtr, int Level) const;
    // The following data fields could be made protected instead of
    // private.  This would make them accessible to the derived AVLClass
    // without making AVLClass a friend of BSTClass.
    BSTNodePtr Root;
    int Count;

};




