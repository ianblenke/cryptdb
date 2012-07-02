#include "bstnode.hh"


/* Given:  Nothing other than the implicit object.
   Task:   To look up the Info field of the object.
   Return: A copy of this data in TheInfo.
*/
void BSTNodeClass::GetInfo(ItemType & TheInfo) const
{
    TheInfo = Info;   // assumes assignment works on this type
}


