#pragma once

#include <string>
#include <iostream>

// comment out for non verbose
#define DEBUG

class NodeData {
public:
    // returns sign (this-N)
    virtual int
    compare(const NodeData * N) const =  0;

    virtual std::string stringify() const = 0;

    virtual NodeData * clone() const  = 0;
};

class MerkleNode : NodeData {
public:
    std::string unique_enc;
    std::string merkle_hash;

    MerkleNode(std::string unique_enc, std::string merkle_hash) :
	unique_enc(unique_enc), merkle_hash(merkle_hash){}
    
    int compare(const NodeData * N) const;

    std::string stringify() const {
	return  "(" + unique_enc + ", hash: " +  merkle_hash + ")";
    }

    NodeData * clone() const {
	return (NodeData *) new MerkleNode(unique_enc, merkle_hash);
    }
};

class BSTNode
{
public:
    
    NodeData * data;
    BSTNode * Left, * Right;

    BSTNode(const NodeData * data, BSTNode * LeftPtr = NULL,
		 BSTNode * RightPtr = NULL):
	data(data->clone()), Left(LeftPtr), Right(RightPtr)
	{};

};






