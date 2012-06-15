#pragma once

#include <cstdio>

#define EXECUTION_VERBOSE 1
#define EXTRA_SANITY_CHECKS 1

#ifdef EXECUTION_VERBOSE
  #define dprintf(args...) \
    do { \
      fprintf(stderr, "%s: ", __PRETTY_FUNCTION__); \
      fprintf(stderr, args); \
    } while (0)
#else
  #define dprintf(args...)
#endif /* EXECUTION_VERBOSE */

#ifdef EXTRA_SANITY_CHECKS
  #define SANITY(x) assert(x)
#else
  #define SANITY(x)
#endif /* EXTRA_SANITY_CHECKS */

namespace hom_agg_constants {
  static const size_t BitsPerDecimalSlot = 83; // hardcoded for now
}
