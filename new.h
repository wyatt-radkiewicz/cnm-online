#ifndef _NEW_H
#define _NEW_H
// This file is to help move the codebase into the future
// currently it uses a really old coding style like this one
// void NAMESPACE_FunctionName(int params_here)
// {
//     STRUCT_NAME struct;
//     int i;
//     for (i = 0; i < SIZE; i++) struct.x[i] = i;
// }
// And it should move to this:
// void namespace_function_name(int32_t params_here) {
//     struct_name_t struct;
//     for (int32_t i = 0; i < struct.len; i++) {
//         struct.arr[i] = i;
//     }
// }
#include <assert.h>
#include <stdbool.h>

// It also deprecates some functions because some things use newer methods/apis
#ifdef REFACTOR
#define fopen(path, mode) static_assert("Stop using fopen! Start using the newer filesystem api!" && 0)
#endif

#endif
