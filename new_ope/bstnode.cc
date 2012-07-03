#include "bstnode.hh"

int MerkleNode::compare(const NodeData * N) const {
    const MerkleNode * n = static_cast<const MerkleNode *>(N);

    return unique_enc.compare(n->unique_enc);
}


