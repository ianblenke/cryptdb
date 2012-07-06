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
    void Insert(const NodeData * data);
    
    //TODO: lacking delete

private:
    AVLNode * GetNode(const NodeData * data,
		       AVLNode * LeftPtr = NULL, AVLNode * RightPtr = NULL,
		       int BalanceValue = 0);
    void CopyTree(const AVLClass & Tree);
    AVLNode * CopySubtree(const AVLNode * Current);
    void SingleRotateLeft(AVLNode * & ParentPtr);
    void SingleRotateRight(AVLNode * & ParentPtr);
    void DoubleRotateLeft(AVLNode * & ParentPtr);
    void DoubleRotateRight(AVLNode * & ParentPtr);
    void UpdateLeftTree(AVLNode * & ParentPtr, bool & ReviseBalance);
    void UpdateRightTree(AVLNode * & ParentPtr, bool & ReviseBalance);
    void AVLInsert(AVLNode * & Tree, AVLNode * NewNodePtr,
		   bool & ReviseBalance);
    void FreeNode(AVLNode * NodePtr);
    void ClearTree(void);
    void ClearSubtree(AVLNode * Current);
       
  
};




