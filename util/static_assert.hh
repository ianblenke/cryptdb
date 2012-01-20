#pragma once

/*
 * Compile-time checks.
 */

#define _static_assert(p) \
    do { switch(p) { default: case 0: case (p): break; } } while (0)
#define _static_assert_is_ptr(p) \
    do { typeof(p)  x __attribute__((unused)) = 0; \
         typeof(*p) y __attribute__((unused)); } while (0)
#define _static_assert_is_array(p) \
    _static_assert(!(((void*)&(p)) - ((void*)(p))))
#define _static_assert_is_array_nonzero(p) \
    do { typeof(p) x __attribute__((unused)) = { [0] 0 }; } while (0)
#define _static_array_size(a) \
    ({ _static_assert_is_array(a); \
       sizeof(a) ? (sizeof(a) / sizeof((a)[0])) : 0; })
#define _static_assert_is_int(v) \
    do { int x __attribute__((unused)) = v * 2; } while (0)

