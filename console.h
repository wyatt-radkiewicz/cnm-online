#ifndef _console_h_
#define _console_h_
#include <stdarg.h>

typedef void(*CONSOLE_CALLBACK)(const char *string);//const char *format, va_list args);

void Console_Init(const char *file);
void Console_Quit(void);

void Console_Print(const char *format, ...);
void Console_SetCallback(CONSOLE_CALLBACK callback);

#endif