#ifndef _ending_text_h_
#define _ending_text_h_
#include "utility.h"

#define ENDING_TEXT_MAX_LINES 48
#define ENDING_TEXT_MAX_WIDTH (UTIL_MAX_TEXT_WIDTH - 8)
#define DIALOGE_PAGE_LINES 3

void EndingText_ResetYValue(void);
void EndingText_SetLine(int index, const char *line);
const char *EndingText_GetLine(int index);
void EndingText_Start(int start_line, int end_line);
void EndingText_Draw(void);
void EndingText_ClearAllLines(void);

void Dialoge_Start(int start_line, int end_line);
void Dialoge_Update(void);
void Dialoge_Draw(void);
void Dialoge_End(void);
int Dialoge_IsActive(void);

#endif