#include "bstree.hh"

using namespace std;

/* Given:  Nothing,
   Task:   This is the constructor to initialize a binary search tree as empty.
   Return: Nothing directly, but it creates the implicit BSTClass object.
*/
BSTClass::BSTClass(void)
{
    Root = NULL;
    Count = 0;
}


/* Given:  Nothing (other than the implicit BSTClass object).
   Task:   This is the destructor.  It's job is to wipe out all data
   storage used by this binary search tree.
   Return: Nothing directly, but the implicit BSTClass object is destroyed.
*/
BSTClass::~BSTClass(void)
{
    ClearTree();
}


/* Given:  Nothing (other than the implicit BSTClass object).
   Task:   To clear out all nodes of the implicit binary search tree.
   Return: Nothing directly, but the implicit BSTClass object is modified
   to be an empty binary search tree.
*/
void
BSTClass::ClearTree(void)
{
    if (Root != NULL)
    {
	ClearSubtree(Root);
	Root = NULL;
	Count = 0;
    }
}


/* Given:  Current   A pointer to a node in the implicit BSTClass object.
   Task:   To wipe out all nodes of this subtree.
   Return: Nothing directly, but the implicit BSTClass object is modified.
*/
void
BSTClass::ClearSubtree(BSTNode * Current)
{
    //  Use a postorder traversal:
    if (Current != NULL)
    {
	ClearSubtree(Current->Left);
	ClearSubtree(Current->Right);
	FreeNode(Current);
    }
}


/* Given:  node      A data item to place into a new node.
   LeftPtr   The pointer to place in the left field of the node.
   RightPtr  The pointer to place in the right field of the node.
   Task:   To create a new node containing the above 3 items.
   Return: A pointer to the new node.
*/
BSTNode *
BSTClass::GetNode(const NodeData * node,
		  BSTNode * LeftPtr, BSTNode * RightPtr)
{
    BSTNode * NodePtr;

    NodePtr = new BSTNode(node, LeftPtr, RightPtr);
    if (NodePtr == NULL)
    {
	cerr << "Memory allocation error!" << endl;
	exit(1);
    }
    return NodePtr;
}


/* Given:  NodePtr   A pointer to a node of the implicit binary search tree.
   Task:   To reclaim the space used by this node.
   Return: Nothing directly, but the implicit object is modified.
*/
void
BSTClass::FreeNode(BSTNode * NodePtr)
{
    delete NodePtr;
}


/* Given:  Nothing (other than the implicit BSTClass object).
   Task:   To look up the number of items in this object.
   Return: This number of items in the function name.
*/
int
BSTClass::NumItems(void) const
{
    return Count;
}


/* Given:  Nothing (other than the implicit BSTClass object).
   Task:   To check if this object (binary search tree) is empty.
   Return: true if empty; false otherwise.
*/
bool
BSTClass::Empty(void) const
{
    if (Count > 0)
	return false;
    else
	return true;
}


/* Given:  node   A data item to be inserted.
   Task:   To insert a new node containing node into the implicit binary
   search tree so that it remains a binary search tree.
   Return: Nothing directly, but the implicit binary search tree is modified.
*/
void
BSTClass::Insert(const NodeData * node)
{
    BSTNode * Current, * Parent, * NewNodePtr;

    Current = Root;
    Parent = NULL;
    while (Current != NULL)
    {
	Parent = Current;
	if (Current->data->compare(node) > 0)
	    Current = Current->Left;
	else
	    Current = Current->Right;
    }

    NewNodePtr = GetNode(node, NULL, NULL);
    if (Parent == NULL)
	Root = NewNodePtr;
    else if (Parent->data->compare(node) > 0) { 
	Parent->Left = NewNodePtr;
    }
    else {
	Parent->Right = NewNodePtr;
    }
    Count++;
}



/* Given:  node    A data item to look for.
   Task:   To search for node in the implicit binary search tree.
   Return: A pointer to the node where node was found or a NULL pointer
   if it was not found.
*/
BSTNode *
BSTClass::Find(const NodeData * node) const
{
    return SubtreeFind(Root, node);
}


/* Given:  Current  A pointer to a node in the implicit binary search tree.
   node     A data item to look for.
   Task:   To search for node in the subtree rooted at the node Current
   points to.
   Return: A pointer to the node where node was found or a NULL pointer
   if it was not found.
*/
BSTNode *
BSTClass::SubtreeFind(BSTNode * Current,
		      const NodeData * node) const
{
    if (Current == NULL)
	return NULL;
    else if (node->compare(Current->data) == 0)
	return Current;
    else if (node->compare(Current->data) < 0)
	return SubtreeFind(Current->Left, node);
    else
	return SubtreeFind(Current->Right, node);
}


static uint
do_maxHeight(BSTNode * subroot) {

    if (subroot == NULL) {
	return 0;
    }
    
    return max(do_maxHeight(subroot->Left) + 1,
	       do_maxHeight(subroot->Right) + 1);
}

uint
BSTClass::maxHeight() const {
    return do_maxHeight(Root);
}


/* Given:  NodePtr   A pointer to the root of the subtree to be printed.
   Level     Integer indentation level to be used.
   Task:   To print (sideways) the subtree to which NodePtr points, using
   an indentation proportional to Level.
   Return: Nothing.
*/
void
BSTClass::PrintSubtree(BSTNode * NodePtr, int Level) const
{
    int k;

    if (NodePtr != NULL)
    {
	PrintSubtree(NodePtr->Right, Level + 1);
	for (k = 0; k < 3 * Level; k++)
	    cout << " ";
	cout << NodePtr->data->stringify() << endl;
	PrintSubtree(NodePtr->Left, Level + 1);
    }
}


/* Given:  Nothing (other than the implicit object).
   Task:   To print (sideways) the implicit binary search tree.
   Return: Nothing.
*/
void
BSTClass::Print(void) const
{
    PrintSubtree(Root, 0);
}


