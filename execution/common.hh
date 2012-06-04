#pragma once

#include <cstdio>

#define dprintf(args...) \
  do { \
    fprintf(stderr, "%s: ", __PRETTY_FUNCTION__); \
    fprintf(stderr, args); \
  } while (0)

#define EXTRA_SANITY_CHECKS 1

#ifdef EXTRA_SANITY_CHECKS
  #define SANITY(x) assert(x)
#else
  #define SANITY(x)
#endif /* EXTRA_SANITY_CHECKS */
