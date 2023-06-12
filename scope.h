#ifndef _SCOPE_H
#define _SCOPE_H

#define defer(start_func, end_func) for ( \
        int __defer_##__FILE__##_##__LINE__##__ = 0, start_func; \
        __defer_##__FILE__##_##__LINE__##__ != 1; \
        __defer_##__FILE__##_##__LINE__##__ = 1, end_func; \
    )

#define scope(end_scope_func) for ( \
        int __scope_##__FILE__##_##__LINE__##__ = 0; \
        __scope_##__FILE__##_##__LINE__##__ != 1; \
        __scope_##__FILE__##_##__LINE__##__ = 1, end_func; \
    )

#endif
