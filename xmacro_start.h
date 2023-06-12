#ifndef _XMACRO_START_H
#define _XMACRO_START_H
// Include this in your header files before any xmacros
// Xmacros with this header should be formatted as such:
// #define _MACRO_TYPE_NAME_XMACROS \
//     ENUM_VAL(MacroTypeVariantName, 99) \
//     ENUM(MacroTypeVariantName2)
// And then call...
// xmacro_enum(_MACRO_TYPE_NAME_XMACROS, macro_type)
// ...to declare the enums as macro_type_t
//
// AND THEN MAKE SURE TO INCLUDE "xmacro_hdr_end.h" at the
// end of the corresponding header file!!!

#define ENUM_VAL(name, val) name = val,
#define ENUM(name) name,

#define xmacro_enum(xmacro, typename) \
    typedef enum typename##_t { \
    xmacro \
    } typename##_t; \
    const char *typename##_as_str(typename##_t x);

#endif
