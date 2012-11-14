#include <util/ope-util.hh>

#include <list>

struct transf {
    std::vector<uint> ope_path;
    int index_inserted;
    int split_point;
};

class OPETransform {

public:
    OPETransform();
    ~OPETransform();
    /*
     * Pushes a new transformation. Transformations must be pushed in leaf to
     * root order.
     * split_point+1 is the position of the first node in the new node created
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
    std::istream& operator<<(std::istream &out);
    bool equals(OPETransform ts);
  

private:
    // first transformation is the leaf
    std::list<transf> ts;

    bool new_root;
 };
