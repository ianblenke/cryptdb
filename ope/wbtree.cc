#include <vector>
#include <util/util.hh>
#include <algorithm>

using namespace std;

static const int w = 8;

struct WBNode;


struct WBNode {
    WBNode * left;
    WBNode * right;
    uint size;
    uint key;

    WBNode(WBNode * _left, WBNode * _right,  uint _key) : left(_left), right(_right), key(_key) {
	size = (left == 0 ? 0 : left->size) +
	    (right == 0 ? 0 : right->size) + 1;
    }
};

static uint
node_size(WBNode * n) {
    return n == NULL ? 0 : n->size;
}

static int cost = 0;
static int existing = 0;
static int not_balanced = 0;

struct WBTree {
    WBNode * root;
    WBTree() {root = NULL;}
    void tree_insert(uint key);
};


static WBNode *
rot_single_left(WBNode * left, WBNode * right, uint key) {
    WBNode * res = new  WBNode(new WBNode(left, right->left, key),
		       right->right, right->key);
    delete(right);
    return res;
}

static WBNode *
rot_double_left(WBNode * left, WBNode * right, uint key) {
    WBNode * res = new WBNode(
	new WBNode(left, right->left->left, key),
	new WBNode(right->left->right, right->right, right->key),
	right->left->key);

    delete(right->left);
    delete(right);
    return res;
}

static WBNode *
rot_single_right(WBNode * left, WBNode * right, uint key) {
    WBNode * res = new WBNode(left->left,
			      new WBNode(left->right, right, key),
			      left->key);

    delete(left);
    return res;
}

static WBNode *
rot_double_right(WBNode * left, WBNode * right, uint key) {
    WBNode * res = new WBNode(
	new WBNode(left->left, left->right->left, left->key),
	new WBNode(left->right->right, right, key),
	left->right->key);
    
    delete(left->right);
    delete(left);
    return res;
}

static WBNode *
balanced_left(WBNode * left, WBNode * right, uint key) {
    int rln = node_size(right->left);
    int rrn = node_size(right->right);

    if (rln < rrn) {
	return rot_single_left(left, right, key);
    } else {
	return rot_double_left(left, right, key);
    }
}

static WBNode *
balanced_right(WBNode * left, WBNode * right, uint key) {
    int lln = node_size(left->left);
    int lrn = node_size(left->right);

    if (lrn < lln) {
	return rot_single_right(left, right, key);
    } else {
	return rot_double_right(left, right, key);
    }
}

static WBNode *
balanced(WBNode * cur, WBNode * left, WBNode * right) {
    int ln = node_size(left);
    int rn = node_size(right);
    uint key = cur->key;

    if (ln + rn < 2) {
	return new WBNode(left, right, key);
    }
    if (rn > w * ln) {
	cost += cur->size;
	return balanced_left(left, right, key);
    }
    if (ln > w * rn) {
	cost += cur->size;
	return balanced_right(left, right, key);
    }
    
    // no need for balancing
    WBNode * res=  new WBNode(left, right, key);
    delete(cur);
    return res;
}
/*
static WBNode *
deleteMin(WBNode * node, uint & min) {
    WBNode * left = node->left;
    WBNode * right = node->right;

    if (!left) {
	min = node->key;
	return right;
    }

    return balanced(node, deleteMin(left, min), right);
    }*/
/*
static WBNode *
tree_delete(WBNode * node, uint key, bool & deleted) {
    WBNode * min, *left, *right;

    if (!node) {
	deleted = false;
	return NULL;
    }

    left = node->left;
    right = node->right;

    if (key < node->key) {
	return balanced(node, tree_delete(left, key, deleted), right);
    }
    if (key > node->key) {
	return balanced(node, left, tree_delete(right, key, deleted));
    }

    //we found node to delete
    deleted = true;

    if (!left) {
	return right;
    }
    if (!right) {
	return left;
    }

    right = deleteMin(right, min);

    return balanced(min, left, right);

}
*/
/*
static uint
find(WBNode * root, uint val) {
    WBNode * node = root;
    
    while (node) {
	if (node->key == val) {
	    return node ? node->key : NULL;
	}
	if (node->key > val) {
	    node = node->left;
	} else {
	    node = node->right;
	}
	       
    }
}
*/
static bool
is_balanced(WBNode * node) {
    int sl = node_size(node->left);
    int sr = node_size(node->right);
    
    if (sr > 0 && (sl > w * sr)) {
	return false;
    }
    if (sl > 0 && sr > w * sl) {
	return false;
    }
    return true;
   
}
static void
check_tree(WBNode * node) {
    assert_s(!node->left || node->left->key < node->key, "");
    assert_s(!node->right || node->right->key > node->key, "");
    assert_s(node_size(node) == (1 + node_size(node->left) + node_size(node->right)), "");
    //check if it is balanced
    if (!is_balanced(node)) {
	not_balanced++;
    }
//    assert_s(is_balanced(node), "not balanced");
    
    if (node->left) {
	check_tree(node->left);
    }

    if (node->right) {
	check_tree(node->right);
    }
}

static void
delete_tree(WBNode * node) {
    if (node->left) {
	delete_tree(node->left);
    }
    if (node->right) {
	delete_tree(node->right);
    }
    delete(node);
}

static WBNode *
insert(WBNode * node, uint key) {
    if (!node) {
	return new WBNode(NULL, NULL, key);
    }

    if (key < node->key) {
	return balanced(node, insert(node->left, key), node->right);
    }
    if (key > node->key) {
	return balanced(node, node->left, insert(node->right, key));
    }
//    cerr << "existing \n";
    existing++;
    return node;
}

void
WBTree::tree_insert(uint key) {
    root = insert(root, key);    
}

static void
print_tree(WBNode * node){
    if (node->left)
	print_tree(node->left);

    cerr << " " << node->key;

    if (node->right)
	print_tree(node->right);
}

static int
height(WBNode * n) {
    if (!n) {
	return 0;
    }
    int hl = height(n->left);
    int hr = height(n->right);

    return hl > hr ? hl+1 : hr+1;
    
}
static void
run(vector<uint> vals, uint no_elems) {

    WBTree tree;

    cost = 0;
    existing = 0;
    not_balanced = 0;
    
    for (uint i = 0; i < no_elems; i++) {
	tree.tree_insert(vals[i]);
    }
    
    check_tree(tree.root);

    //cerr << "in order pass is "; print_tree(tree.root);

    cerr << "nodes inserted " << tree.root->size << "\n";
    cerr << "max_height " << height(tree.root) << "\n";
    cerr << "nodes not balanced " << not_balanced << "\n";
    cerr << "overall cost was " << cost*1.0/(no_elems-existing) << "\n";
    
    delete_tree(tree.root);
 
}
int
main(int argc, char **argv) {

    if (argc != 2) {
	cerr << "usage: ./wbtree no_elems ";
	return 0;
    }
    
    vector<uint> vals;
    srand( time(NULL));
    uint no_elems = atoi(argv[1]);
    
    cerr << "\n\n- " << no_elems << " values in random order: \n";

    // test on random values
    for(uint i=0; i< no_elems; i++){
	vals.push_back(rand());
    }
    
    run(vals, no_elems);
    
    // test on values in increasing order
    sort(vals.begin(), vals.end());
    cerr << "\n\n- " << no_elems << " values in increasing order: \n";

    run(vals, no_elems);
    
    // test on values in decreasing order
    reverse(vals.begin(), vals.end());
    cerr << "\n\n- " << no_elems << " values in decreasing order: \n";

    run(vals, no_elems);
    
    
}
