#pragma once



// OPE encoding v||index|| 
struct table_entry {
    uint64_t ope; // v should now be everything
  
    uint64_t refcount;
    //int version;
};

template <class EncT>
struct OPETable {
    std::map<EncT, table_entry> table;

    // returns a pointer to the table_entry for encval
    // or NULL if it does not exist
    table_entry * find(EncT encval);

    // must return a valid pointer
    // if a table_entry for encval does not exist, it throws exception
    table_entry * get(EncT encval);

    // returns true if new table entry was
    // returns false if encval existed (in which case refcount is incremented)
    bool insert(EncT encval, uint64_t ope);

    // returns true if encval was in the table and was thus updated
    bool update(EncT encval, uint64_t newope);
};
