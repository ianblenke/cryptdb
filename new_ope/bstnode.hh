#pragma once

#include "itemtype.hh"


class BSTNodeClass
   {
   protected:
      ItemType Info;
      BSTNodeClass * Left, * Right;
   public:
      BSTNodeClass(const ItemType & Item, BSTNodeClass * LeftPtr = NULL,
         BSTNodeClass * RightPtr = NULL):
         Info(Item), Left(LeftPtr), Right(RightPtr)
         {
         };
      void GetInfo(ItemType & TheInfo) const;
   friend class BSTClass;
   friend class AVLClass;
   };

typedef BSTNodeClass * BSTNodePtr;




