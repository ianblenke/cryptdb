#pragma once

#include "bstnode.hh"


class AVLNode: public BSTNode
{
public:
    AVLNode(const NodeData * data, AVLNode * LeftPtr = NULL,
		 AVLNode * RightPtr = NULL, int BalanceValue = 0);
    friend class AVLClass;

private:
    int Balance;
};






