#include "avltree.hh"

using namespace std;


/* Given:   Item          A data item to insert in a new node.
   LeftPtr       A pointer to place in Left field of the new node.
   RightPtr      A pointer to place in the Right field.
   BalanceValue  A value to put in Balance field of the node.
   Task:    To create a new node filled with the above 4 values.
   Return:  A pointer to the new node in the function name.
*/
AVLNode * AVLClass::GetNode(const NodeData * data,
			     AVLNode * LeftPtr, AVLNode * RightPtr, int BalanceValue)
{
    AVLNode * NodePtr;

#ifdef DEBUG
    cout << "DEBUG: AVLClass GetNode called" << endl;
    cout << "Press ENTER to continue";
    cin.get();
#endif

    NodePtr = new AVLNode(data, LeftPtr, RightPtr, BalanceValue);
    if (NodePtr == NULL)
    {
	cerr << "Memory allocation error!" << endl;
	exit(1);
    }
    return NodePtr;
}


/* Given:  NodePtr   A pointer to a node of the implicit AVL tree.
   Task:   To reclaim the space used by this node.
   Return: Nothing directly, but the implicit object is modified.
*/
void AVLClass::FreeNode(AVLNode * NodePtr)
{
    delete NodePtr;
}


/* Given:  Nothing (other than the implicit AVLClass object).
   Task:   This is the destructor.  It's job is to wipe out all data
   storage used by this AVL tree.
   Return: Nothing directly, but the implicit AVLClass object is destroyed.
*/
AVLClass::~AVLClass(void)
{
    ClearTree();
}


/* Given:  Nothing (other than the implicit AVLClass object).
   Task:   To clear out all nodes of the implicit AVL tree.
   Return: Nothing directly, but the implicit AVLClass object is modified
   to be an empty AVL tree.
*/
void AVLClass::ClearTree(void)
{
    ClearSubtree(reinterpret_cast <AVLNode *> (Root));
    Root = NULL;
    Count = 0;
}


/* Given:  Current   A pointer to a node in the implicit AVLClass object.
   Task:   To wipe out all nodes of this subtree.
   Return: Nothing directly, but the implicit AVLClass object is modified.
*/
void AVLClass::ClearSubtree(AVLNode * Current)
{
    //  Use a postorder traversal:
    if (Current != NULL)
    {
	ClearSubtree(reinterpret_cast <AVLNode *> (Current->Left));
	ClearSubtree(reinterpret_cast <AVLNode *> (Current->Right));
	FreeNode(Current);
    }
}


/* Given:  Tree   An AVL tree.
   Task:   To copy Tree into the current (implicit) AVL tree object.
   Return: Nothing directly, but the implicit AVL tree is modified.
*/
void AVLClass::CopyTree(const AVLClass & Tree)
{
#ifdef DEBUG
    cout << "DEBUG: AVLClass CopyTree called" << endl;
#endif

    Root = CopySubtree(reinterpret_cast <AVLNode *> (Tree.Root));
    Count = Tree.Count;
}


/* Given:  Current   Pointer to the current node of the implicit AVL tree.
   Task:   To make a copy of the subtree starting at node Current.
   Return: A pointer to the root node of this copy.
*/
AVLNode * AVLClass::CopySubtree(const AVLNode * Current)
{
    AVLNode * NewLeftPtr, *NewRightPtr, *NewParentPtr;
    // Once this section works, leave this out as it gives too much output:
    //#ifdef DEBUG
    // cout << "DEBUG: AVLClass CopySubtree called" << endl;
    //#endif

    if (Current == NULL)
	return NULL;

    if (Current->Left == NULL)
	NewLeftPtr = NULL;
    else
	NewLeftPtr = CopySubtree(reinterpret_cast <AVLNode *> (Current->Left));

    if (Current->Right == NULL)
	NewRightPtr = NULL;
    else
	NewRightPtr = CopySubtree(reinterpret_cast <AVLNode *> (Current->Right));

    NewParentPtr = GetNode(Current->data, NewLeftPtr, NewRightPtr,
			   Current->Balance);

    return NewParentPtr;
}


/* Given:  ParentPtr   A pointer to the node at which to rotate left.
   Task:   To do a left rotation at the node specified above in the implicit
   AVL tree object.
   Return: ParentPtr   Points to the new root node of the rotated subtree.
   Note that the implicit AVL tree is modified.
*/
void AVLClass::SingleRotateLeft(AVLNode * & ParentPtr)
{
    AVLNode * RightChildPtr;

#ifdef DEBUG
    cout << "DEBUG: single rotate left at " << ParentPtr->data << endl;
#endif

    RightChildPtr = reinterpret_cast <AVLNode *> (ParentPtr->Right);
    ParentPtr->Balance = Balanced;
    RightChildPtr->Balance = Balanced;
    ParentPtr->Right = RightChildPtr->Left;
    RightChildPtr->Left = ParentPtr;
    ParentPtr = RightChildPtr;
}


/* Given:  ParentPtr   A pointer to the node at which to rotate right.
   Task:   To do a right rotation at the node specified above in the
   implicit AVL tree object.
   Return: ParentPtr   Points to the new root node of the rotated subtree.
   Note that the implicit AVL tree is modified.
*/
void AVLClass::SingleRotateRight(AVLNode * & ParentPtr)
{
    AVLNode * LeftChildPtr;

#ifdef DEBUG
    cout << "DEBUG: single rotate right at " << ParentPtr->data << endl;
#endif

    LeftChildPtr = reinterpret_cast <AVLNode *> (ParentPtr->Left);
    ParentPtr->Balance = Balanced;
    LeftChildPtr->Balance = Balanced;
    ParentPtr->Left = LeftChildPtr->Right;
    LeftChildPtr->Right = ParentPtr;
    ParentPtr = LeftChildPtr;
}


/* Given:  ParentPtr   A pointer to the node at which to double rotate left.
   Task:   To do a double left rotation at the node specified above in the
   implicit AVL tree object.
   Return: ParentPtr   Points to the new root node of the rotated subtree.
   Note that the implicit AVL tree is modified.
*/
void AVLClass::DoubleRotateLeft(AVLNode * & ParentPtr)
{
    AVLNode * RightChildPtr, *NewParentPtr;

#ifdef DEBUG
    cout << "DEBUG: double rotate left at " << ParentPtr->data << endl;
#endif

    RightChildPtr = reinterpret_cast <AVLNode *> (ParentPtr->Right);
    NewParentPtr = reinterpret_cast <AVLNode *> (RightChildPtr->Left);

    if (NewParentPtr->Balance == LeftHeavy)
    {
	ParentPtr->Balance = Balanced;
	RightChildPtr->Balance = RightHeavy;   // patched to fix bug in book
    }
    else if (NewParentPtr->Balance == Balanced)
    {
	ParentPtr->Balance = Balanced;
	RightChildPtr->Balance = Balanced;
    }
    else
    {
	ParentPtr->Balance = LeftHeavy;
	RightChildPtr->Balance = Balanced;
    }

    NewParentPtr->Balance = Balanced;
    RightChildPtr->Left = NewParentPtr->Right;
    NewParentPtr->Right = RightChildPtr;
    ParentPtr->Right = NewParentPtr->Left;
    NewParentPtr->Left = ParentPtr;
    ParentPtr = NewParentPtr;
}


/* Given:  ParentPtr   Pointer to the node at which to double rotate right.
   Task:   To do a double right rotation at the node specified above in the
   implicit AVL tree object.
   Return: ParentPtr   Points to the new root node of the rotated subtree.
   Note that the implicit AVL tree is modified.
*/
void AVLClass::DoubleRotateRight(AVLNode * & ParentPtr)
{
    AVLNode * LeftChildPtr, *NewParentPtr;

#ifdef DEBUG
    cout << "DEBUG: double rotate right at " << ParentPtr->data << endl;
#endif

    LeftChildPtr = reinterpret_cast <AVLNode *> (ParentPtr->Left);
    NewParentPtr = reinterpret_cast <AVLNode *> (LeftChildPtr->Right);

    if (NewParentPtr->Balance == RightHeavy)
    {
	ParentPtr->Balance = Balanced;
	LeftChildPtr->Balance = LeftHeavy;   // patched to fix bug in book
    }
    else if (NewParentPtr->Balance == Balanced)
    {
	ParentPtr->Balance = Balanced;
	LeftChildPtr->Balance = Balanced;
    }
    else
    {
	ParentPtr->Balance = RightHeavy;
	LeftChildPtr->Balance = Balanced;
    }

    NewParentPtr->Balance = Balanced;
    LeftChildPtr->Right = NewParentPtr->Left;
    NewParentPtr->Left = LeftChildPtr;
    ParentPtr->Left = NewParentPtr->Right;
    NewParentPtr->Right = ParentPtr;
    ParentPtr = NewParentPtr;
}


/* Given:  ParentPtr      Pointer to parent node, whose subtree needs
   to be updated to an AVL tree (due to insertion).
   ReviseBalance  Indicates if we need to revise balances.
   Assume: Subtree rooted at this parent node is left heavy.
   Task:   To update this subtree so that it forms an AVL tree.
   Return: ParentPtr      Points to the root node of the updated subtree.
   ReviseBalance  Indicates if we need to revise balances.
   Note that the implicit AVL tree is modified.
*/
void AVLClass::UpdateLeftTree(AVLNode * & ParentPtr, bool & ReviseBalance)
{
    AVLNode * LeftChildPtr;

#ifdef DEBUG
    cout << "DEBUG: UpdateLeftTree called at " << ParentPtr->data << endl;
#endif

    LeftChildPtr = reinterpret_cast <AVLNode *> (ParentPtr->Left);
    if (LeftChildPtr->Balance == LeftHeavy)
    {   // left subtree is also left heavy
	SingleRotateRight(ParentPtr);
	ReviseBalance = false;
    }
    else if (LeftChildPtr->Balance == RightHeavy)
    {   // right subtree is also right heavy
	DoubleRotateRight(ParentPtr);
	ReviseBalance = false;
    }
}


/* Given:  ParentPtr      Pointer to parent node, whose subtree needs
   to be updated to an AVL tree (due to insertion).
   ReviseBalance  Indicates if we need to revise balances.
   Assume: Subtree rooted at this parent node is right heavy.
   Task:   To update this subtree so that it forms an AVL tree.
   Return: ParentPtr      Points to the root node of the updated subtree.
   ReviseBalance  Indicates if we need to revise balances.
   Note that the implicit AVL tree is modified.
*/
void AVLClass::UpdateRightTree(AVLNode * & ParentPtr, bool & ReviseBalance)
{
    AVLNode * RightChildPtr;

#ifdef DEBUG
    cout << "DEBUG: UpdateRightTree called at " << ParentPtr->data << endl;
#endif

    RightChildPtr = reinterpret_cast <AVLNode *> (ParentPtr->Right);
    if (RightChildPtr->Balance == RightHeavy)
    {   // right subtree is also right heavy
	SingleRotateLeft(ParentPtr);
	ReviseBalance = false;
    }
    else if (RightChildPtr->Balance == LeftHeavy)
    {   // right subtree is also left heavy
	DoubleRotateLeft(ParentPtr);
	ReviseBalance = false;
    }
}


/* Given:  Tree           Pointer to node of implicit AVL tree.  This node
   is the root of the subtree in which to insert.
   NewNodePtr     Pointer to the new node to be inserted.
   ReviseBalance  Indicates if we need to revise balances.

   Task:   To insert the new node in the subtree indicated above.

   Return: Tree           Pointer to the root node of the subtree.
   ReviseBalance  Indicates if we need to revise balances.
   
   Note that the implicit AVL tree object is modified.
*/
void AVLClass::AVLInsert(AVLNode * & Tree, AVLNode * NewNodePtr,
			 bool & ReviseBalance)
{
    bool RebalanceCurrentNode;

#ifdef DEBUG
    if (Tree == NULL) {
	cout << "DEBUG: AVLInsert called at empty subtree to insert "
	     << NewNodePtr->data << endl;
    }
    else {
	cout << "DEBUG: AVLInsert called at " << Tree->data << " to insert "
	     << NewNodePtr->data << endl;
    }
#endif

    if (Tree == NULL)   // insert in empty subtree
    {
	Tree = NewNodePtr;
	Tree->Balance = Balanced;
	ReviseBalance = true;  // tell others to check balances due to new node
	
    }
    else if (NewNodePtr->data->compare(Tree->data) < 0)
    {
	AVLInsert(reinterpret_cast <AVLNode * &> (Tree->Left), NewNodePtr,
		  RebalanceCurrentNode);
	if (RebalanceCurrentNode)
	{
	    if (Tree->Balance == LeftHeavy)   // now 1 node worse than this
		UpdateLeftTree(Tree, ReviseBalance);
	    else if (Tree->Balance == Balanced)   // was balanced, now 1 extra
            {
		Tree->Balance = LeftHeavy;
		ReviseBalance = true;
            }
	    else   // was right heavy, due to extra node on left now balanced
            {
		Tree->Balance = Balanced;
		ReviseBalance = false;
            }
	}
	else
	    ReviseBalance = false;
    }
    else
    {
	AVLInsert(reinterpret_cast <AVLNode * &> (Tree->Right), NewNodePtr,
		  RebalanceCurrentNode);
	if (RebalanceCurrentNode)
	{
	    if (Tree->Balance == LeftHeavy)  // extra node on right balances it
            {
		Tree->Balance = Balanced;
		ReviseBalance = false;
            }
	    else if (Tree->Balance == Balanced)   // now 1 node heavy on right
            {
		Tree->Balance = RightHeavy;
		ReviseBalance = true;
            }
	    else   // was right heavy, now off by 2 on right, must fix:
		UpdateRightTree(Tree, ReviseBalance);
	}
	else
	    ReviseBalance = false;
    }
}


/* Given:  Nothing.
   Task:   This is the default constructor for creating the implicit AVL
   tree object.
   Return: Nothing directly, but the implicit AVL tree object is created.
*/
AVLClass::AVLClass(void)
{
    // just use the automatic BSTClass default constructor
#ifdef DEBUG
    cout << "DEBUG: AVLClass default constructor called" << endl;
#endif
}


/* Given:  Tree   An AVLClass object.
   Task:   This is the copy constructor.  It copies Tree into the implicit
   AVL tree object.
   Return: Nothing directly, but the implicit AVL tree object is modified.
*/
AVLClass::AVLClass(const AVLClass & Tree)
{
#ifdef DEBUG
    cout << "DEBUG: AVLClass copy constructor called" << endl;
#endif

    CopyTree(Tree);
}


/* Given:  Tree   AN AVLClass object.
   Task:   This is the overloaded = operator.  It places into the implicit
   AVL tree object an identical copy of Tree.
   Return: A reference to the modified AVLClass object (in case one wants
   to use something like C = B = A).
*/
AVLClass & AVLClass::operator=(const AVLClass & Tree)
{
#ifdef DEBUG
    cout << "DEBUG: AVLClass overloaded = operator called" << endl;
#endif

    // cannot copy a tree to itself:
    if (this == &Tree)
	return * this;
    ClearTree();
    CopyTree(Tree);
    return * this;
}


/* Given:  Item  A data item to be inserted into the implicit AVL tree.
   Task:   To insert Item (in a new node) in the AVL tree.
   Return: Nothing directly, but the implicit AVL tree object is modified.
*/
void AVLClass::Insert(const NodeData * data)
{
    AVLNode * NewNodePtr;
    bool ReviseBalance = false;

#ifdef DEBUG
    cout << "DEBUG: AVLClass Insert called for inserting " << data->stringify() << endl;
#endif

    NewNodePtr = GetNode(data);
    AVLInsert(reinterpret_cast <AVLNode * &> (Root), NewNodePtr,
	      ReviseBalance);
    Count++;
}


