#include <stdio.h>
#include <stdbool.h>
#include "log.h"
#include "xmacro_impl.h"

static bool g_initialized;

// Implementation specific!
#define LOG_PATH_MAX 4096
static FILE *g_log_file, *g_err_file;
static char g_log_path_buf[LOG_PATH_MAX];

xmacro_as_str(_LOG_LEVEL_XMACROS, log_level)
xmacro_as_str(_LOG_RESULT_XMACROS, log_result)

log_result_t log_init(void) {
    if (g_initialized) return LogResultAlreadyInitialized;
    g_log_file = nullptr;
    g_err_file = nullptr;
    return LogResultNotInitialized;
}
log_result_t log_init_with_logfile(str_t log_file) {
    if (g_initialized) return LogResultAlreadyInitialized;
    //g_log_file = fopen();
    return LogResultNotInitialized;
}
log_result_t log_init_with_errfile(str_t log_file, str_t err_file);
log_result_t log_stop(void);
log_level_t log_get_level(void);
log_result_t log_set_level(log_level_t lvl);

void log_print_u1(log_level_t lvl, str_t s1, u64 u1);
void log_print_u2(log_level_t lvl, str_t s1, u64 u1, str_t s2, u64 u2);
void log_print_u3(log_level_t lvl, str_t s1, u64 u1, str_t s2, u64 u2, str_t s3, u64 u3);
void log_print_u4(log_level_t lvl, str_t s1, u64 u1, str_t s2, u64 u2, str_t s3, u64 u3, str_t s4, u64 u4);

void log_print_x1(log_level_t lvl, str_t s1, u64 x1);
void log_print_x2(log_level_t lvl, str_t s1, u64 x1, str_t s2, u64 x2);
void log_print_x3(log_level_t lvl, str_t s1, u64 x1, str_t s2, u64 x2, str_t s3, u64 x3);
void log_print_x4(log_level_t lvl, str_t s1, u64 x1, str_t s2, u64 x2, str_t s3, u64 x3, str_t s4, u64 x4);

void log_print_i1(log_level_t lvl, str_t s1, i64 i1);
void log_print_i2(log_level_t lvl, str_t s1, i64 i1, str_t s2, i64 i2);
void log_print_i3(log_level_t lvl, str_t s1, i64 i1, str_t s2, i64 i2, str_t s3, i64 i3);
void log_print_i4(log_level_t lvl, str_t s1, i64 i1, str_t s2, i64 i2, str_t s3, i64 i3, str_t s4, i64 i4);

void log_print_f1(log_level_t lvl, str_t s1, f64 f1);
void log_print_f2(log_level_t lvl, str_t s1, f64 f1, str_t s2, f64 f2);
void log_print_f3(log_level_t lvl, str_t s1, f64 f1, str_t s2, f64 f2, str_t s3, f64 f3);
void log_print_f4(log_level_t lvl, str_t s1, f64 f1, str_t s2, f64 f2, str_t s3, f64 f3, str_t s4, f64 f4);

void log_print_u2_f2(log_level_t lvl, str_t s1, u64 u1, str_t s2, u64 u2, str_t s3, f64 f3, str_t s4, f64 f4);
void log_print_u1_f1(log_level_t lvl, str_t s1, u64 u1, str_t s2, u64 u2);
void log_print_ufuf(log_level_t lvl, str_t s1, u64 u1, str_t s2, f64 f1, str_t s3, u64 u2, str_t s4, f64 f2);
void log_print_x2_f2(log_level_t lvl, str_t s1, u64 x1, str_t s2, u64 x2, str_t s3, f64 f3, str_t s4, f64 f4);
void log_print_x1_f1(log_level_t lvl, str_t s1, u64 x1, str_t s2, u64 x2);
void log_print_xfxf(log_level_t lvl, str_t s1, u64 x1, str_t s2, f64 f1, str_t s3, u64 x2, str_t s4, f64 f2);
