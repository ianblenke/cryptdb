/*

   This program inserts some hard-coded data (chars) into an AVL
   tree.  It then tests the following functions by calling them and
   reporting the results on the screen:  Find, NumItems, the overloaded =
   operator, the copy constructor, and Print.

   In itemtype.h leave DEBUG defined if you want extra debugging output,
   or comment it off to omit such output.

*/

#include "avltree.hh"

void Pause(void);


int main(void)
   {
   AVLClass AVLTreeA, AVLTreeB;
   AVLNodePtr Result;

   AVLTreeA.Insert('F');
   AVLTreeA.Insert('B');
   AVLTreeA.Insert('W');
   AVLTreeA.Insert('M');
   AVLTreeA.Insert('E');
   AVLTreeA.Insert('A');
   AVLTreeA.Insert('C');

   cout << "AVLTreeA at this point contains:" << endl;
   AVLTreeA.Print();
   Pause();

   AVLTreeA.Insert('X');  // Put several in a row to try to unbalance
   AVLTreeA.Insert('Y');  // the tree.  An AVL tree should adjust.
   AVLTreeA.Insert('Z');
   AVLTreeA.Insert('L');   // Try again to unbalance.
   AVLTreeA.Insert('K');
   AVLTreeA.Insert('J');
   AVLTreeA.Insert('I');
   AVLTreeA.Insert('H');
   AVLTreeA.Insert('N');
   AVLTreeA.Insert('O');
   AVLTreeA.Insert('R');
   AVLTreeA.Insert('P');
   AVLTreeA.Insert('Q');

   cout << "AVLTreeA contains:" << endl;
   AVLTreeA.Print();
   Pause();

   Result = reinterpret_cast <AVLNodePtr> (AVLTreeA.Find('E'));
   if (Result == NULL)
      cout << "The E could not be found in AVLTreeA (incorrect)." << endl;
   else
      cout << "The E was found in AVLTreeA (correct)." << endl;

   cout << "Number of items in AVLTreeA is " << AVLTreeA.NumItems() << endl;
   Pause();

   Result = reinterpret_cast <AVLNodePtr> (AVLTreeA.Find('D'));
   if (Result == NULL)
      cout << "The D could not be found in AVLTreeA (correct)." << endl;
   else
      cout << "The D was found in AVLTreeA (incorrect)." << endl;
   Pause();

   cout << "Try out the overloaded = operator for AVLClass:" << endl;
   AVLTreeB = AVLTreeA;

   cout << "AVLTreeB contains:" << endl;
   AVLTreeB.Print();
   Pause();
   cout << "Number of items in AVLTreeB is " << AVLTreeB.NumItems() << endl;
   Pause();

   Result = reinterpret_cast <AVLNodePtr> (AVLTreeB.Find('E'));
   if (Result == NULL)
      cout << "The E could not be found in AVLTreeB (incorrect)." << endl;
   else
      cout << "The E was found in AVLTreeB (correct)." << endl;
   Pause();

   cout << "Try out the copy constructor for AVLClass:" << endl;
   AVLClass AVLTreeC(AVLTreeA);

   cout << "AVLTreeC contains:" << endl;
   AVLTreeC.Print();
   Pause();
   cout << "Number of items in AVLTreeC is " << AVLTreeC.NumItems() << endl;
   Pause();

   Result = reinterpret_cast <AVLNodePtr> (AVLTreeC.Find('W'));
   if (Result == NULL)
      cout << "The W could not be found in AVLTreeC (incorrect)." << endl;
   else
      cout << "The W was found in AVLTreeC (correct)." << endl;
   Pause();

   return 0;
   }


/*  Given:  Nothing
    Task:   To pause screen output by prompting user to press ENTER.
    Return: Nothing
*/
void Pause(void)
   {
   cout << "Press ENTER to continue:";
   cin.get();
   }


