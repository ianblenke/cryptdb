#pragma once

#include <string>
#include <cassert>

#define FIELDTYPE(m) \
    m(TYPE_TEXT) \
    m(TYPE_INTEGER) \
    m(TYPE_AGG_RESULT_COUNT) \
    m(TYPE_AGG_RESULT_SUM) \
    m(TYPE_AGG_RESULT_SET) \
    m(TYPE_OPE)

typedef enum fieldType {
#define __temp_m(n) n,
FIELDTYPE(__temp_m)
#undef __temp_m
} fieldType;

inline std::string fieldtype_to_string(fieldType f) {
    switch (f) {
#define __temp_m(n) case n: return #n;
FIELDTYPE(__temp_m)
#undef __temp_m
    default: assert(false); return "";
    }
}

#undef FIELDTYPE

// oAGG_ORIGINAL is a hack for now, so we can distingush between
// new style agg and old-style agg
#define ONION(m) \
    m(oDET) \
    m(oOPE) \
    m(oAGG) \
    m(oAGG_ORIGINAL) \
    m(oNONE) \
    m(oSWP) \
    m(oINVALID)

typedef enum onion {
#define __temp_m(n) n,
ONION(__temp_m)
#undef __temp_m
} onion;

inline std::string onion_to_string(onion o) {
    switch (o) {
#define __temp_m(n) case n: return #n;
ONION(__temp_m)
#undef __temp_m
    default: assert(false); return "";
    }
}

#undef ONION

#define SECLEVELS(m)    \
    m(INVALID)          \
    m(PLAIN)            \
    m(PLAIN_DET)        \
    m(DETJOIN)          \
    m(DET)              \
    m(SEMANTIC_DET)     \
    m(PLAIN_OPE)        \
    m(OPEJOIN)          \
    m(OPE)              \
    m(SEMANTIC_OPE)     \
    m(PLAIN_AGG)        \
    m(SEMANTIC_AGG)     \
    m(PLAIN_SWP)        \
    m(SWP)              \
    m(SEMANTIC_VAL)     \
    m(SECLEVEL_LAST)

typedef enum class SECLEVEL {
#define __temp_m(n) n,
SECLEVELS(__temp_m)
#undef __temp_m
} SECLEVEL;

const std::string levelnames[] = {
#define __temp_m(n) #n,
SECLEVELS(__temp_m)
#undef __temp_m
};

inline SECLEVEL string_to_sec_level(const std::string &s)
{
#define __temp_m(n) if (s == #n) return SECLEVEL::n;
SECLEVELS(__temp_m)
#undef __temp_m
    // TODO: possibly raise an exception
    return SECLEVEL::INVALID;
}

//returns max and min levels on onion o
SECLEVEL getMax(onion o);
SECLEVEL getMin(onion o);

