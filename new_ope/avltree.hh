/* AVL Tree implementation. 

   We modified and adapted from original implementation of Br. David Carlson
   http://cis.stvincent.edu/html/tutorials/swd/avltrees/avltrees.html
*/

#pragma once

#include "avlnode.hh"
#include "bstree.hh"


const int LeftHeavy = -1;
const int Balanced = 0;
const int RightHeavy = 1;


class AVLClass: public BSTClass
{
public:
    AVLClass(void);
    AVLClass(const AVLClass & Tree);
    ~AVLClass(void);
    AVLClass & operator= (const AVLClass & Tree);
    void Insert(const ItemType & Item);
    
    //TODO: lacking delete

private:
    AVLNodePtr GetNode(const ItemType & Item,
		       AVLNodePtr LeftPtr = NULL, AVLNodePtr RightPtr = NULL,
		       int BalanceValue = 0);
    void CopyTree(const AVLClass & Tree);
    AVLNodePtr CopySubtree(const AVLNodePtr Current);
    void SingleRotateLeft(AVLNodePtr & ParentPtr);
    void SingleRotateRight(AVLNodePtr & ParentPtr);
    void DoubleRotateLeft(AVLNodePtr & ParentPtr);
    void DoubleRotateRight(AVLNodePtr & ParentPtr);
    void UpdateLeftTree(AVLNodePtr & ParentPtr, bool & ReviseBalance);
    void UpdateRightTree(AVLNodePtr & ParentPtr, bool & ReviseBalance);
    void AVLInsert(AVLNodePtr & Tree, AVLNodePtr NewNodePtr,
		   bool & ReviseBalance);
    void FreeNode(AVLNodePtr NodePtr);
    void ClearTree(void);
    void ClearSubtree(AVLNodePtr Current);
       
  
};




