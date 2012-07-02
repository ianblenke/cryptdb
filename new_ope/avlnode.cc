#include "avlnode.hh"


/* Given:  Item          Data item to place in Info field.
           LeftPtr       Pointer to place in Left field.
           RightPtr      Pointer to place in Right field.
           BalanceValue  Value to place in Balance field.
   Task:   This is the constructor.  It's job is to create a new
           object containing the above 4 values.
   Return: Nothing directly, but the implicit object is created.
*/
AVLNodeClass::AVLNodeClass(const ItemType & Item,
   AVLNodeClass * LeftPtr, AVLNodeClass * RightPtr, int BalanceValue):
   // call BSTNodeClass constructor, initialize field:
   BSTNodeClass(Item, LeftPtr, RightPtr), Balance(BalanceValue)
   {
   #ifdef DEBUG
      cout << "DEBUG: AVLNodeClass constructor called" << endl;
   #endif
   }


