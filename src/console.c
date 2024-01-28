#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"
#include "utility.h"
#include "mem.h"

// this is one file where we DO want to use raw fopen so we disable new.h's saftey bars
#undef fopen

static FILE *console_file;
static CONSOLE_CALLBACK console_callback;
//static char printbuf[512 + 1];

void Console_Init(const char *file)
{
	console_file = fopen(file, "w");
	console_callback = NULL;
}
void Console_Quit(void)
{
	fclose(console_file);
	console_callback = NULL;
}

void Console_Print(const char *format, ...)
{
	if (!strnlen(format, 0x10000))
		return;

	va_list args;
	va_start(args, format);
	vfprintf(console_file, format, args);
	va_start(args, format);
	fprintf(console_file, "\n");
	va_start(args, format);
	vprintf(format, args);
	va_start(args, format);

	int bufsz = vsnprintf(NULL, 0, format, args) + 1;
	assert(bufsz > 0);
	char *buf = arena_alloc(bufsz);
	vsnprintf(buf, bufsz, format, args);

	va_end(args);
	printf("\n");
	if (console_callback != NULL)
		console_callback(buf);//format, args);
	arena_popfree(buf);
}
void Console_SetCallback(CONSOLE_CALLBACK callback)
{
	console_callback = callback;
}
