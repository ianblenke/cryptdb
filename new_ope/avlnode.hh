#pragma once

#include "bstnode.hh"


class AVLNodeClass: public BSTNodeClass
   {
   private:
      int Balance;
   public:
      // constructor:
      AVLNodeClass(const ItemType & Item, AVLNodeClass * LeftPtr = NULL,
         AVLNodeClass * RightPtr = NULL, int BalanceValue = 0);
   friend class AVLClass;
   friend class AVLTableClass;   // may not get to implementing this
   };

typedef AVLNodeClass * AVLNodePtr;




