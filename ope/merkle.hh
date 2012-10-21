
/****************************
 *  Merkle data structures
 *  and related functions.
 ****************************/

#pragma once

#include <string>
#include <vector>
#include <list>
#include <util/util.hh>
#include <crypto/sha.hh>
#include <util/ope-util.hh>


// Represents one element: its key and the hash of its subtree
struct ElInfo {
    std::string key;
    std::string hash;

    static const uint repr_size = sha256::hashsize + b_key_size;
    
    ElInfo(std::string key_, std::string hash_): key(key_), hash(hash_) {}
    ElInfo() {}

    bool operator==(const ElInfo & el) const;
    
    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &out);
    
    std::string pretty() const;
    
};

// Records information at a B tree node; key of node, children, etc.
struct NodeInfo {
    std::string key;
    int pos_in_parent;
    std::vector<ElInfo> childr;

    NodeInfo();
    
    std::string hash() const;
    uint key_count();

    bool operator==(const NodeInfo & ni) const;
    
    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);
    
    std::string pretty() const;

};

// Merkle proof for one node 
struct MerkleProof {
    std::list<NodeInfo> path;

    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);
    
    std::string pretty() const;

};

    
// Information needed about a delete on a level
struct DelInfo  {
    NodeInfo node;
    
    bool has_left_sib;
    NodeInfo left_sib;

    bool has_right_sib;
    NodeInfo right_sib;

    // info added by client during verification
    bool this_was_del;
    bool right_was_del;
    
    DelInfo() {
	has_left_sib = false;
	has_right_sib = false;
	this_was_del = false;
	right_was_del = false;
    }

    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);
    
    std::string pretty() const;
    
};

// checks if two DelInfo are equivalent
bool
equiv(const DelInfo & sim, const DelInfo & given);

typedef std::vector<DelInfo> State;

struct PrettyPrinter {
    static std::string pretty(const State & st);
};

/*
 * Merkle Proofs for updates
 *
 * 1. Deletion Merkle Proof
 *
 * - delete changes at most the nodes on a path from a node (``deletion path'')
 *   up to the root and one additional node, sibling of one node on this path
 *
 * - there are a few transformations deletion may perform on the tree and we
 * - need to convey these in the proofs
 *
 * 2. Insertion Merkle Proof
 *
 * - Changes only the nodes on a path from a node up to root
 *
 *
 * UpdateMerkleProof:
 * ``old'' information in DelInfo is before delete/insert
 * ``new'' is after delete/insert
 */
struct UpdateMerkleProof {
    // vector is from root to leaf
    State st_before; 
    State st_after; 
    
    std::string old_hash() const;
    bool check_del_change(std::string key_to_del) const;
    bool check_ins_change(std::string key_to_ins) const;
    std::string new_hash() const;

    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);

    std::string pretty() const;
   
};

// Two special values of the hash,
// it is unlikely a real hash will take them
const std::string hash_empty_node = "0";
const std::string hash_not_extracted = "1";


// formats (key, hash) pair for hashing
// arranges key in key_size bytes and hash in hash_size bytes
// and concatenates them
std::string
format_concat(const std::string & key, uint key_size, const std::string & hash, uint hash_size);
