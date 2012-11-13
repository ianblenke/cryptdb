#include <util/ope-util.hh>

#include <list>

struct transf {
    std::vector<uint> ope_path;
    int num;
    int index_inserted;
    int split_point;
};

class OPETransform {

public:
    /*
     * Pushes a new transformation. Transformations must be pushed in leaf to
     * root order.
     * split_point+1 is the position of the first node in the new node created
     * during split
     * If split point is negative, the node was not split.
     * Num is the number of edges in the ope_path.
     * 
     */
    void push_change(OPEType ope_path, int num, int index_inserted, int split_point);

    // returns the interval of values affected by the transformation
    // min and max can also be affected
    void get_interval(OPEType & omin, OPEType & omax);

    // transforms the given OPE value
    OPEType transform(OPEType val);


    //must deal with root

private:
    // first transformation is the leaf
    std::list<transf> ts;
 
};
