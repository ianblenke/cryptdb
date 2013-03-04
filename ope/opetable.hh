#pragma once


#include <sstream>
#include <vector>
#include <iostream>
#include <map>


#include <util/util.hh>
#include <ope/tree.hh>

// OPE encoding v||index|| 
struct table_entry {
    uint64_t ope; // v should now be everything
  
    uint64_t refcount;
    //int version;

    TreeNode * n;
};



template <class EncT>
class OPETable {
public:
    std::map<EncT, table_entry> table;
    
    // returns a pointer to the table_entry for encval
    // or NULL if it does not exist
    table_entry * find(EncT encval);

    // must return a valid pointer
    // if a table_entry for encval does not exist, it throws exception
    table_entry get(EncT encval);

    // returns true if new table entry was not in the table and was inserted
    // returns false if encval existed (in which case refcount is incremented)
    bool insert(EncT encval, uint64_t ope, TreeNode * n);

    // returns true if encval was in the table and was thus updated
    bool update(EncT encval, uint64_t newope, TreeNode * n);

    // removes entry and returns true if successfully removed
    bool remove(EncT encval);
};
