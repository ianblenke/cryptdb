#pragma once

#include <util/ope-util.hh>

#include <list>

#define TransTypes(m)    \
    m(ROTATE_FROM_RIGHT) \
    m(ROTATE_FROM_LEFT)  \
    m(MERGE_WITH_RIGHT)	  \
    m(MERGE_WITH_LEFT)	  \
    m(SIMPLE)		  \
    m(NONLEAF)


enum class TransType {
#define __temp_m(n) n,
    TransTypes(__temp_m)
#undef __temp_m
};

struct itransf {
    std::vector<uint> ope_path;
    
    int index_inserted;
    int split_point;
};



/* For rotates:
 *  -- ope_path is path to node
 
 * For simple deletion: parent_index contains index deleted
 */
struct dtransf {
    std::vector<uint> ope_path; // the path leading to parent of nodes affected
				// by operation
    
    int parent_index; //index in parent where replacement was made, or of key deleted
		      //during merge 
    int left_mcount; // the number of keys in the left node affected by this
		     // operation at the time  before this operation
    
    TransType optype;
};




class OPEdTransform {
    
public:
    OPEdTransform();
    ~OPEdTransform();
    /*
     * Pushes a new transformation. Transformations must be pushed in the order
     * that they happen. 
     * Each transformation indicates what happened to a node at a level.
     * ope_path and num were the path of the node in the old tree
     * split_point+1 is the position of the first node  in the old tree that now
     * is in the new node created
     * during split
     * If split point is negative, the node was not split.
     * Num is the number of edges in the ope_path.
     * Push_change should only be called for changes that happended to old
     * nodes; for a new root, call add_root()
     */
    void push_change(OPEType ope_path, int num, int parent_index, int left_mcount);
    
    // returns the interval of values affected by the transformation
    // min and max can also be affected
    // these are two ope encodings 
    void get_interval(OPEType & omin, OPEType & omax);
    
    // transforms the given OPE value
    // the value should be a correct OPE encoding 
    OPEType transform(OPEType val);
    
    // for network messaging
    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);
    void from_stream(std::istream &is);
    bool equals(OPEdTransform ts);
    

private:
    // first transformation is the leaf
    std::list<dtransf> ts;
};

class OPEiTransform {
    
public:
    OPEiTransform();
    ~OPEiTransform();
    /*
     * Pushes a new transformation. Transformations must be pushed in leaf to
     * root order.
     * Each transformation indicates what happened to a node at a level.
     * ope_path and num were the path of the node in the old tree
     * split_point+1 is the position of the first node  in the old tree that now
     * is in the new node created
     * during split
     * If split point is negative, the node was not split.
     * Num is the number of edges in the ope_path.
     * Push_change should only be called for changes that happended to old
     * nodes; for a new root, call add_root()
     */
    void push_change(OPEType ope_path, int num, int index_inserted, int split_point);
    
    /* Call this if root was split */
    void add_root();
    
    // returns the interval of values affected by the transformation
    // min and max can also be affected
    // these are two ope encodings 
    void get_interval(OPEType & omin, OPEType & omax);
    
    // transforms the given OPE value
    // the value should be a correct OPE encoding 
    OPEType transform(OPEType val);
    
    // for network messaging
    std::ostream& operator>>(std::ostream &out);
    std::istream& operator<<(std::istream &is);
    void from_stream(std::istream &is);
    bool equals(OPEiTransform ts);
    
    
private:
    // first transformation is the leaf
    std::list<itransf> ts;
    
    bool new_root;
};
