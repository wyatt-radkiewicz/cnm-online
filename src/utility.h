#ifndef _utility_h_
#define _utility_h_
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __EMSCRIPTEN__
#define CNM_NO_X86ASSEMBLY
#endif

//#define CNM_LOW_MEM

#ifdef CNM_LOW_MEM
#define NETGAME_MAX_NODES 4
#define NETGAME_MAX_HISTORY 10
#else
#define NETGAME_MAX_NODES 64
#define NETGAME_MAX_HISTORY 12
#endif

#define XSTRINGIZE(s) STRINGIZE(s)
#define STRINGIZE(s) #s
#define CNM_MIN(x, m) ((x) < (m) ? (x) : (m))
#define CNM_MAX(x, m) ((x) > (m) ? (x) : (m))
#define CNM_CLAMP(x, min, max) (CNM_MAX(CNM_MIN(x, max), min))
#define UTIL_MAX_TEXT_WIDTH (320 / 8)
#define CNM_PI 3.141599265f
#define CNM_2RAD(d) ((d) / 180.0f * CNM_PI)
#define CNM_2DEG(r) ((r) / CNM_PI * 180.0f)
#define CNM_VERSION_PREFIX "Beta"
#define CNM_MAJOR_VERSION 1
#define CNM_MINOR_VERSION 5
#define CNM_VERSION_STRING CNM_VERSION_PREFIX " " XSTRINGIZE(CNM_MAJOR_VERSION)"."XSTRINGIZE(CNM_MINOR_VERSION)

inline static uint16_t ror16(uint16_t x, uint16_t n) {
	return (x >> n) | (x << (16-n));
}
inline static uint16_t rol16(uint16_t x, uint16_t n) {
	return (x << n) | (x >> (16-n));
}
inline static uint32_t ror32(uint32_t x, uint32_t n) {
	return (x >> n) | (x << (32-n));
}
inline static uint32_t rol32(uint32_t x, uint32_t n) {
	return (x << n) | (x >> (32-n));
}
inline static uint64_t ror64(uint64_t x, uint64_t n) {
	return (x >> n) | (x << (64-n));
}
inline static uint64_t rol64(uint64_t x, uint64_t n) {
	return (x << n) | (x >> (64-n));
}

inline static float signf(float x) {
    if (x < 0.0f) {
        return -1.0f;
    } else if (x > 0.0f) {
        return 1.0f;
    } else {
        return 0.0f;
    }
}

typedef struct _CNM_RECT
{
	int x, y, w, h;
} CNM_RECT;
typedef struct _CNM_BOX
{
	float x, y, w, h;
} CNM_BOX;

typedef enum _CNM_BOOLEAN
{
	CNM_FALSE,
	CNM_TRUE
} CNM_BOOLEAN;

int Util_GetLine(char *buffer, int buffer_size, FILE *fp);
int Util_StringPrintF(char *buffer, int buffer_size, const char *format, va_list args);
int Util_IsSafePrintable(char c);
void Util_SetRect(CNM_RECT *r, int x, int y, int w, int h);
void Util_SetBox(CNM_BOX *b, float x, float y, float w, float h);
int Util_AABBCollision(const CNM_BOX *a, const CNM_BOX *b);
int Util_ResolveAABBCollision(CNM_BOX *a, const CNM_BOX *b, int *resolved_in_x, int *resolved_in_y);

void Util_RandSetSeed(unsigned int seed);
int Util_RandInt(int min, int max);
float Util_RandFloat(void);

#endif
