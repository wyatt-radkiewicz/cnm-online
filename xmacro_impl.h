#ifndef _XMACRO_IMPL_H
#define _XMACRO_IMPL_H
// Include this file in a c file AFTER the corresponding header file
// that contains the xmacros for the enum
// and call this...
// xmacro_as_str(_MACRO_TYPE_NAME_XMACROS, macro_type)

#define ENUM(name) case name: return #name;
#define ENUM_VAL(name, val) case val: return #name;

#define xmacro_as_str(xmacro, typename) \
    const char *typename##_as_str(typename##_t x) { \
        switch (x) { \
    xmacro \
        } \
    }

#endif
