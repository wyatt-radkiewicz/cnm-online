#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utility.h"

int Util_GetLine(char *buffer, int buffer_size, FILE *fp) {
	char c;
	int r, i = 0, first;

	if (buffer_size > 0)
		buffer[0] = '\0';
	first = 1;
	while (1) {
		r = fread(&c, 1, 1, fp);
		if (feof(fp) || r != 1) return 0;
		if (c == '\0') return 1;
		if (c == '\n' || c == '\r') {
			if (first)
				continue;
			else
				return 1;
		}
		if (i+1 < buffer_size) {
			buffer[i] = c;
			buffer[++i] = '\0';
		}
		first = 0;
	}
	return 0;
}
static int Utility_PutChar(int *count, char **buffer, int buffer_size, char c)
{
	if (*count >= buffer_size || !c)
	{
		return CNM_FALSE;
	}
	if (*buffer != NULL)
	{
		*(*buffer)++ = c;
	}

	(*count)++;
	return CNM_TRUE;
}
int Util_StringPrintF(char *buffer, int buffer_size, const char *format, va_list args)
{
	const char *cur = format, *str_arg;
	char frmt_buf[64];
	int count = 0, i, str_len;

	memset(buffer, 0, buffer_size);
	buffer_size -= 1;
	while (*cur)
	{
		if (*cur != '%')
		{
			if (!Utility_PutChar(&count, &buffer, buffer_size, *cur++))
			{
				return buffer_size;
			}
		}
		else
		{
			switch (*(++cur))
			{
			case '\0':
				return count;

			case '%':
				if (!Utility_PutChar(&count, &buffer, buffer_size, '%'))
				{
					return buffer_size;
				}
				break;

			case 'd': case 'i': case 'u':
				memset(frmt_buf, 0, sizeof(frmt_buf));
				if (*(cur-1) == 'u') sprintf(frmt_buf, "%u", va_arg(args, int));
				else sprintf(frmt_buf, "%d", va_arg(args, int));
				count += (int)strlen(frmt_buf);
				if (count >= buffer_size)
				{
					return buffer_size;
				}
				strcat(buffer, frmt_buf);
				buffer += strlen(frmt_buf);
				break;

			case 'f':
				memset(frmt_buf, 0, sizeof(frmt_buf));
				sprintf(frmt_buf, "%.2f", va_arg(args, double));
				count += (int)strlen(frmt_buf);
				if (count >= buffer_size)
				{
					return buffer_size;
				}
				strcat(buffer, frmt_buf);
				buffer += strlen(frmt_buf);
				break;

			case 's':
				str_arg = va_arg(args, const char *);
				str_len = (int)strlen(str_arg);
				for (i = 0; i < str_len; i++)
				{
					if (!Utility_PutChar(&count, &buffer, buffer_size, *str_arg++))
					{
						return buffer_size;
					}
				}
				break;

			case 'c':
				if (!Utility_PutChar(&count, &buffer, buffer_size, (char)va_arg(args, int)))
					return buffer_size;
				break;
			}
			cur++;
		}
	}

	return count;
}
int Util_IsSafePrintable(char c)
{
	return c != '%' && c != '\0' && c >= 0x20 && c < 0x7f && c != '`';
}
void Util_SetRect(CNM_RECT *r, int x, int y, int w, int h)
{
	r->x = x;
	r->y = y;
	r->w = w;
	r->h = h;
}
void Util_SetBox(CNM_BOX *b, float x, float y, float w, float h)
{
	b->x = x;
	b->y = y;
	b->w = w;
	b->h = h;
}
int Util_AABBCollision(const CNM_BOX *a, const CNM_BOX *b)
{
	if (a == NULL || b == NULL)
	{
		return CNM_FALSE;
	}
	else
	{
		if (b->x < a->x + a->w &&
			b->y < a->y + a->h &&
			b->x + b->w > a->x &&
			b->y + b->h > a->y)
		{
			return CNM_TRUE;
		}
		else
		{
			return CNM_FALSE;
		}
	}
}
int Util_ResolveAABBCollision(CNM_BOX *a, const CNM_BOX *b, int *resolved_in_x, int *resolved_in_y)
{
	int dummy[2];
	int sides[4];
	
	if (resolved_in_x == NULL)
		resolved_in_x = dummy + 0;
	if (resolved_in_y == NULL)
		resolved_in_y = dummy + 1;

	if (Util_AABBCollision(a, b))
	{
		sides[0] = (int)((b->x + b->w) - (a->x));		// If smallest: Move "b" right
		sides[1] = (int)((a->y + a->h) - (b->y));		// If smallest: Move "b" up
		sides[2] = (int)((a->x + a->w) - (b->x));		// If smallest: Move "b" left
		sides[3] = (int)((b->y + b->h) - (a->y));		// If smallest: Move "b" down
		int min = CNM_MIN(sides[0], CNM_MIN(sides[1], CNM_MIN(sides[2], sides[3])));

		if (sides[1] <= min + 2) {
			*resolved_in_y = CNM_TRUE;
			*resolved_in_x = CNM_FALSE;
			a->y = b->y - a->h;
		}
		if (sides[0] == min) {
			*resolved_in_x = CNM_TRUE;
			*resolved_in_y = CNM_FALSE;
			a->x = b->x + b->w;
		}
		if (sides[2] == min) {
			*resolved_in_x = CNM_TRUE;
			*resolved_in_y = CNM_FALSE;
			a->x = b->x - a->w;
		}
		if (sides[3] == min) {
			*resolved_in_y = CNM_TRUE;
			*resolved_in_x = CNM_FALSE;
			a->y = b->y + b->h;
		}

		return CNM_TRUE;
	}
	else
	{
		return CNM_FALSE;
	}
}

float Util_RandomFloat(void)
{
	return (float)rand() / (float)RAND_MAX;
}
int Util_RandomInt(int min, int max)
{
	int temp;
	if (min > max)
	{
		temp = min;
		min = max;
		max = temp;
	}

	return (rand() % (max - min)) + min;
}

void Util_RandSetSeed(unsigned int seed)
{
	if (seed != 0)
	{
		srand(seed);
	}
	else
	{
		srand((unsigned int)time(NULL));
	}
}
int Util_RandInt(int min, int max)
{
	if (min == max) return min;
	else return rand() % (max - min) + min;
}
float Util_RandFloat(void)
{
	return (float)(rand() % (RAND_MAX / 2)) / (float)(RAND_MAX / 2);
}
