#ifndef _RESULT_H
#define _RESULT_H
#include "types.h"

#define make_result_type(typename, result_enum, ok_type) \
    typedef struct typename##_result_t { \
        ok_type val; \
        result_enum result; \
    } typename##_result_t;

#define make_option_type(typename, ok_type) \
    typedef struct typename##_option_t { \
        ok_type val; \
        bool valid; \
    } typename##_option_t;

#endif
