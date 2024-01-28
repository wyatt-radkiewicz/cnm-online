#ifndef _lparse_h_
#define _lparse_h_
#include <stddef.h>

#define LPARSE_NAMESZ (16)
#define LPARSE_MAX_ENTRIES (128)

typedef enum LParseMode_e {
	lparse_read,
	lparse_write
} LParseMode;

typedef enum LParseType_e {
	lparse_null,
	lparse_dummy,
	lparse_i32,
	lparse_u32,
	lparse_u8,
	lparse_u16,
	lparse_float,
	lparse_cnm_rect,
	lparse_type_max
} LParseType;

typedef struct LParse_s LParse;
typedef struct LParseEntry_s LParseEntry;

extern LParse *global_lparse;

LParse		*lparse_open_from_file(void *fp, LParseMode m);
LParse		*lparse_open_from_file_inplace(LParse *lparse, void *fp, LParseMode m);
int			lparse_get_version(LParse *lp);
void		lparse_set_version(LParse *lp, int version);
void		lparse_close(LParse *lp);
void		lparse_close_inplace(LParse *lp);

LParseEntry	*lparse_get_entry(LParse *lp, const char *name);
LParseEntry	*lparse_make_entry(LParse *lp, const char *name, LParseType type, size_t cnt);
size_t		lparse_get_size(LParseEntry *entry);
LParseType	lparse_get_type(LParseEntry *entry);
void		lparse_get_data(LParse *lp, LParseEntry *entry, size_t elem, size_t cnt, void *buf);
void		lparse_set_data(LParse *lp, LParseEntry *entry, size_t elem, size_t cnt, const void *buf);

#endif
