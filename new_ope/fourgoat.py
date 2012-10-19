# N-ary Scapegoat trees.  Based on
# http://cg.scs.carleton.ca/~morin/teaching/5408/refs/gr93.pdf
# http://en.wikipedia.org/wiki/Scapegoat_tree
# http://people.scs.carleton.ca/~maheshwa/Honor-Project/scapegoat.pdf

import random, math, sys

# If NOISY, dump trees and balancing information
NOISY = False
# If DOTS, print a . for each insert and the height of each scapegoat
DOTS = False

# N is the number of children per node
# ALPHA should be between 1 (unbalanced) and 1/N (completely balanced)

# Binary tree
#N = 2
#ALPHA = 0.6

# Quad tree
N = 4
ALPHA = 0.75

touchednodes = 0
num_rebalances = 0
num_inserted = 0
merklecost = 0

class Node(object):
    def __init__(self):
        # Keys in this node.  NOT sorted.
        self.keys = []
        # Child node to the right of each key in keys, plus a mapping
        # of None for the left-most child.
        self.right = {}
        self.parent = None

    def __pred(self, key):
        """Return the key that is predecessor of key, or None."""
        return max([k for k in self.keys if k < key] or [None])

    def insert(self, key):
        """Return the path from the inserted node up to the root, or
        None if key is already present."""

        if key in self.keys:
            return None
        if len(self.keys) < N - 1:
            self.keys.append(key)
            return [self]
        else:
            path = self.right.setdefault(self.__pred(key), Node()).insert(key)
            if path is not None:
                path.append(self)
            return path

    def __contains__(self, key):
        if key in self.keys:
            return True
        child = self.right.get(self.__pred(key))
        if child:
            return key in child
        return False

    def __len__(self):
        return len(self.keys) + sum(len(child) for child in self.right.values())

    def height(self):
        return 1 + max([child.height() for child in self.right.values()] or [0])

    def depth(self):
        if 
        return 1 +
    
    def rebalance(self):
        # LAAAME; you can do this in linear time
	global touchednodes
	global num_rebalances
        
	num_rebalances+=1
        keys = []
        def flatten(n):
            if n is None:
                return
            keys.extend(n.keys)
            for child in n.right.values():
                flatten(child)
        flatten(self)
        keys.sort()
	touchednodes = touchednodes + len(keys)
        my_merkle_cost = self.merklecost()
	print "Rebalance # ", num_rebalances, " at ", num_inserted, " rebalance size: ", len(keys), " total touched: ", touchednodes
        
        def getMid(keys):
            mid = len(keys)/2
            return keys[:mid], keys[mid], keys[mid+1:]
        def rebuild(keys):
            node = Node()
            if len(keys) < N:
                node.keys = keys
            elif N == 2:
                (l, m, r) = getMid(keys)
                node.keys = [m]
                node.right = {None: rebuild(l), m: rebuild(r)}
            elif N == 4:
                # [ll] lm [lr] m [rl] rm [rr]
                (l, m, r) = getMid(keys)
                (ll, lm, lr) = getMid(l)
                (rl, rm, rr) = getMid(r)
                node.keys = [lm, m, rm]
                node.right = {None: rebuild(ll),
                              lm:   rebuild(lr),
                              m:    rebuild(rl),
                              rm:   rebuild(rr)}
            else:
                raise ValueError("Can't rebalance N=%d" % N)
            return node
        nnode = rebuild(keys)
        self.keys = nnode.keys
        self.right = nnode.right

    def dump(self, indent = 0):
        for key in [None] + sorted(self.keys):
            if key is not None:
                print "  "*indent + str(key)
            if key in self.right:
                self.right[key].dump(indent+1)

class UnbalancedTree(Node):
    pass

class BalancedTree(Node):
    def __init__(self):
        Node.__init__(self)
        self.size = 0

    def insert(self, key):
	global num_inserted
	num_inserted+=1
        path = Node.insert(self, key)
        if path is None:
            # key was already in the tree
            return

        if NOISY:
            print "==============="
            tree.dump()
            preheight = tree.height()

        self.size += 1
        height = len(path)
        if NOISY:
            print height, math.log(self.size, 1/ALPHA)

        if height > math.log(self.size, 1/ALPHA) + 1:
            # The tree is no longer alpha-height-balanced overall, so
            # find the first parent of the inserted node where the
            # subtree rooted at that node isn't alpha-height-balanced
            # (in the worst case, this is the root of the tree).  Note
            # that the node we just inserted must be the lowest node
            # in the entire tree (since any lower node would have
            # already violated this property), so 'height' is, in
            # fact, the height of the tree.
            scapegoat = self.findScapegoat(path)
            scapegoat.rebalance()
            if NOISY:
                tree.dump()
                postheight = tree.height()
                print "Pre-height ", preheight
                print "Post-height", postheight
                opt = int(math.floor(math.log(self.size, N) + 1))
                print "Optimal    ", opt
            if DOTS:
                sys.stdout.write(str(path.index(scapegoat)))
                sys.stdout.flush()
        else:
            if DOTS:
                sys.stdout.write(".")
                sys.stdout.flush()

    @staticmethod
    def findScapegoatDumb(path):
        # This is equivalent to findScapegoat, but doesn't optimize to
        # eliminate repeated work computing the subtree size as we
        # move up the tree.
        for n in path:
            if any(len(child) > ALPHA*len(n) for child in n.right.values()):
                return n

    # number of nodes (not keys) and siblings on the path of a node n to root
    @staticmethod
    def compute_merkle_cost(n, path):
        count = 0
        do_count = False
        for node in path:
            if do_count:
                count += len(node.right.keys())
            if (node == n):
                do_count = True
         return count + 1 # 1 is for the root
    
    @staticmethod
    def findScapegoat(path):
        # Return the lowest node in path (that is, the earliest) where
        # the subtree rooted at that node isn't alpha-height-balanced.
        size = 0
        global merklecost
        last = None
        for n in path:
            childSizes = [size if child is last else len(child)
                          for child in n.right.values()]
            size = sum(childSizes) + len(n.keys)
            last = n
            if NOISY:
                print "-->", childSizes, size, ALPHA*size
#            assert size == len(n)
            if any(childSize > ALPHA*size for childSize in childSizes):
                mymerklecost = compute_merkle_cost(n, path)
                if NOISY:
                    print " extra merkle cost ", mymerklecost 
                merklecost += mymerklecost
                return n
            continue

tree = BalancedTree()
keys = set()
num_vals = 1000000
for n in range(num_vals):
    # Random key distribution
#    key = random.randint(0,1000000000)
    #print key
    # Worst-case
    key = n
    tree.insert(key)
    keys.add(key)

print touchednodes/(1.0*num_vals)
print "extra merkle cost ", merklecost/(1.0 * num_vals)
print "total merkle cost", (touchednodes + merklecost)/(1.0 * num_vals)
print tree.height()
#for key in range(max(keys)):
#    assert (key in tree) == (key in keys)
