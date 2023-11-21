#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lparse.h"
#include "utility.h"

static int type_sizes[lparse_type_max] = {
	0, //lparse_null 0
	0, //lparse_dummy 1
	4, //lparse_i32 2
	4, //lparse_u32 3
	1, //lparse_u8 4
	2, //lparse_u16 5
	4, //lparse_float 6
	sizeof(CNM_RECT), // lparse_cnm_rect 7
};

typedef struct LParseHeader_s {
	char fourcc[4];
	uint32_t version;
} LParseHeader;

typedef struct LParseEntry_s {
	char name[LPARSE_NAMESZ];
	uint32_t type;
	uint32_t size;
	uint32_t offs;
} LParseEntry;

typedef struct LParse_s {
	LParseMode mode;
	FILE *fp;

	uint32_t end_offs;
	int version;
	LParseEntry entries[LPARSE_MAX_ENTRIES];
} LParse;

static int lparse_read_entries(LParse *lp);

LParse		*lparse_open_from_file(void *fp, LParseMode m) {
	LParse *lp = malloc(sizeof(LParse));

	lp->mode = m;
	lp->fp = fp;
	lp->end_offs = sizeof(LParseHeader) + sizeof(lp->entries);
	memset(lp->entries, 0, sizeof(lp->entries));
	if (fp == NULL) {
		lparse_close(lp);
		return NULL;
	}
	if (lp->mode == lparse_read) {
		if (lparse_read_entries(lp)) {
			return lp;
		}
		else {
			lparse_close(lp);
			return NULL;
		}
	}
	else {
		lparse_set_version(lp, 1);
		fwrite(lp->entries, 1, sizeof(lp->entries), lp->fp);
	}
	return lp;
}
int			lparse_get_version(LParse *lp) {
	return lp->version;
}
void			lparse_set_version(LParse *lp, int version) {
	LParseHeader hdr;

	memcpy(hdr.fourcc, "CNML", 4);
	lp->version = version;
	hdr.version = version;
	fseek(lp->fp, 0, SEEK_SET);
	fwrite(&hdr, sizeof(LParseHeader), 1, lp->fp);
}
void		lparse_close(LParse *lp) {
	if (lp->fp != NULL) fclose(lp->fp);
	free(lp);
}

LParseEntry	*lparse_get_entry(LParse *lp, const char *name) {
	int i;

	for (i = 0; i < LPARSE_MAX_ENTRIES; i++) {
		if (lp->entries[i].type != lparse_null && strncmp(lp->entries[i].name, name, LPARSE_NAMESZ) == 0) {
			break;
		}
	}

	if (i == LPARSE_MAX_ENTRIES) return NULL;
	else return lp->entries + i;
}
LParseEntry	*lparse_make_entry(LParse *lp, const char *name, LParseType type, size_t cnt) {
	int i;

	for (i = 0; i < LPARSE_MAX_ENTRIES; i++) {
		if (lp->entries[i].type == lparse_null) {
			strncpy(lp->entries[i].name, name, LPARSE_NAMESZ);
			lp->entries[i].size = cnt;
			lp->entries[i].type = type;
			lp->entries[i].offs = lp->end_offs;
			lp->end_offs += type_sizes[type] * cnt;
			fseek(lp->fp, sizeof(LParseHeader) + sizeof(LParseEntry) * i, SEEK_SET);
			fwrite(lp->entries + i, sizeof(LParseEntry), 1, lp->fp);

			return lp->entries + i;
		}
	}

	return NULL;
}
size_t		lparse_get_size(LParseEntry *entry) {
	return entry->size;
}
LParseType	lparse_get_type(LParseEntry *entry) {
	if (entry == NULL) return lparse_null;
	else return entry->type;
}
void		lparse_get_data(LParse *lp, LParseEntry *entry, size_t elem, size_t cnt, void *buf) {
	if (!lp->fp) return;
	fseek(lp->fp, entry->offs + elem * type_sizes[entry->type], SEEK_SET);
	fread(buf, type_sizes[entry->type], cnt, lp->fp);
}
void		lparse_set_data(LParse *lp, LParseEntry *entry, size_t elem, size_t cnt, const void *buf) {
	if (!lp->fp) return;
	fseek(lp->fp, entry->offs + elem * type_sizes[entry->type], SEEK_SET);
	fwrite(buf, type_sizes[entry->type], cnt, lp->fp);
}


static int lparse_read_entries(LParse *lp) {
	LParseHeader hdr;

	fread(&hdr, sizeof(LParseHeader), 1, lp->fp);
	if (memcmp(hdr.fourcc, "CNML", 4) != 0) {
		return 0;
	}
	lp->version = hdr.version;
	fread(lp->entries, 1, sizeof(lp->entries), lp->fp);
	return 1;
}
