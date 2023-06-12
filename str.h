#ifndef _STR_H
#define _STR_H
#include <string.h>
#include "types.h"
#include "result.h"

// utf-8 strings
typedef struct str_t {
    u8 *data;
    usize len;
} str_t;
typedef struct str_const_t {
    const u8 *data;
    usize len;
} str_const_t;

#define STRL(cstr_literal) ((str_const_t){ .val = cstr_literal, len = strlen(cstr_literal) })

// String offset type
make_option_type(str_off, usize)
make_option_type(str_c32, usize)

str_t str_from_cstr(char *cstr);
str_const_t str_from_cstr_const(const char *cstr);
str_t str_from_mem(void *buf, usize bufsz);
str_const_t str_from_mem_const(const void *buf, usize bufsz);
str_t str_substr(str_t str, usize start, usize end);
// Gets an offset where there is a char
str_off_option_t str_char_idx(str_t str, c32 chr);
// Returns the offset to the next character
str_off_option_t str_next_char(str_t str, usize curr_char_off);
str_c32_option_t str_c32(str_t str, usize off);

#endif
