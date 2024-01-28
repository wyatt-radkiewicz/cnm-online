#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

typedef struct bmp_filehdr {
	char ident[2];
	uint32_t szbytes;
	uint16_t reserved[2];
	uint32_t offsdata;
} __attribute__((packed)) bmp_filehdr_t;

typedef enum bmp_compression {
	BmpRGB				= 0,
	BmpRLE8				= 1,
	BmpRLE4				= 2,
	BmpBitfields		= 3,
	BmpJPG				= 4,
	BmpPNG				= 5,
	BmpAlphaBitfields	= 6,
	BmpCYMK				= 11,
	BmpCYMKRLE8			= 12,
	BmpCYMKRLE4			= 13,
} bmp_compression_t;

typedef enum bmp_resolution_unit {
	BmpPixelsPerMeter = 0,
} bmp_resolution_unit_t;

typedef enum bmp_fillorigin {
	BmpFillOrigin_LowerLeft = 0,
} bmp_fillorigin_t;

typedef enum bmp_halftoning {
	BmpHalftoning_None				= 0,
	BmpHalftoning_ErrorDiffusion	= 1,
	BmpHalftoning_PANDA				= 2,
	BmpHalftoning_SuperCircle		= 3,
} bmp_halftoning_t;

typedef enum bmp_colencoding {
	BmpColorEncoding_RGB			= 0,
} bmp_colencoding_t;

typedef uint8_t color_t;

typedef struct bmp_color {
	uint8_t b, g, r, reserved;
} __attribute__((packed)) bmp_color_t;

typedef struct bmp {
	bmp_filehdr_t filehdr;

	uint32_t hdrsize;
	union {
		struct {
			uint16_t w, h;
			uint16_t nplanes;
			uint16_t bpp;
		} corehdr;
		struct {
			struct {
				int32_t w, h;
				uint16_t nplanes;
				uint16_t bpp;
				uint32_t compression;
				uint32_t szbytes;
				int32_t hres;
				int32_t vres;
				uint32_t ncols;
				uint32_t nimportant_cols;
			} bmpinfohdr;
			struct {
				uint16_t resunits;
				uint16_t padding;
				uint16_t fillorigin;
				uint16_t halftoning;
				int32_t halftoning_params[2];
				uint32_t colencoding;
				uint32_t custom;
			} bmpinfohdr2;
			union {
				struct {
					uint8_t ignore[12];
				} bitfields;
				struct {
					uint8_t ignore[16];
				} alpha_bitfields;
			};
		};
	} hdr;

	size_t ncolors;
	bmp_color_t *palette;
	
	size_t w, h, npixels, pitch;
	color_t *pixels;
} __attribute__((packed)) bmp_t;

#define GET_PIXEL(bmp, x, y) ((bmp).pixels[(y) * (bmp).pitch + (x)])

bool bmp_load(bmp_t *self, const char *path) {
	FILE *file = fopen(path, "rb");
	if (!file) return false;

	// Load the header
	bool ret = false;
	*self = (bmp_t){0};
	fread(&self->filehdr, sizeof(self->filehdr), 1, file);
	if (self->filehdr.ident[0] != 'B' || self->filehdr.ident[1] != 'M') goto cleanup;
	fread(&self->hdrsize, sizeof(self->hdrsize), 1, file);
	fread(&self->hdr, self->hdrsize - sizeof(self->hdrsize), 1, file);
	if (self->hdrsize != 40) goto cleanup;
	if (self->hdr.bmpinfohdr.compression != BmpRGB) goto cleanup;
	if (self->hdr.bmpinfohdr.bpp != 8) goto cleanup;
	if (self->hdr.bmpinfohdr.ncols != 256) goto cleanup;
	
	self->ncolors = self->hdr.bmpinfohdr.ncols;
	self->palette = malloc(sizeof(*self->palette) * self->ncolors);
	fread(self->palette, sizeof(*self->palette), self->ncolors, file);
	assert(self->palette[1].r == 31 && self->palette[1].g == 23 && self->palette[1].b == 11);

	fseek(file, self->filehdr.offsdata, SEEK_SET);
	self->w = self->hdr.bmpinfohdr.w;
	self->h = labs(self->hdr.bmpinfohdr.h);
	self->pitch = self->w;
	self->npixels = self->w * self->h;
	self->pixels = malloc(sizeof(*self->pixels) * self->npixels);

	int y = self->hdr.bmpinfohdr.h < 0 ? 0 : self->h - 1;
	const int ystep = self->hdr.bmpinfohdr.h < 0 ? 1 : -1;
	const int yend = self->hdr.bmpinfohdr.h < 0 ? self->h : -1;
	for (; y != yend; y += ystep) {
		fread(self->pixels + self->pitch * y, 1, self->pitch, file);
		if (self->pitch % 4 != 0) fseek(file, 4 - (self->pitch % 4), SEEK_CUR);
	}

	ret = true;
cleanup:
	fclose(file);
	return ret;
}

void bmp_deinit(bmp_t *self) {
	free(self->palette);
	free(self->pixels);
}

bool bmp_save(bmp_t *self, const char *path) {
	FILE *file = fopen(path, "wb");
	if (!file) return false;

	if (self->hdr.bmpinfohdr.compression != BmpRGB) goto cleanup;
	fwrite(&self->filehdr, sizeof(self->filehdr), 1, file);
	self->hdrsize = 40;
	self->hdr.bmpinfohdr.h = abs(self->hdr.bmpinfohdr.h);
	fwrite(&self->hdrsize, sizeof(self->hdrsize), 1, file);
	fwrite(&self->hdr, self->hdrsize - sizeof(self->hdrsize), 1, file);
	fwrite(self->palette, sizeof(*self->palette), self->ncolors, file);
	const long pixstart = ((ftell(file) - 1) / 4 + 1) * 4;
	self->filehdr.offsdata = pixstart;
	fseek(file, 0, SEEK_SET);
	fwrite(&self->filehdr, sizeof(self->filehdr), 1, file);
	fseek(file, pixstart, SEEK_SET);

	const int fullwidth = ((self->w - 1) / 4 + 1) * 4;
	for (int y = self->h - 1; y != -1; y--) {
		int x;
		for (x = 0; x < self->w; x++) {
			fwrite(&GET_PIXEL(*self, x, y), 1, 1, file);
		}
		if (fullwidth - x > 0) fwrite(&(uint32_t){0}, fullwidth-x, 1, file);
	}

cleanup:
	fclose(file);
	return true;
}

void print_help(void) {
	printf(
		"ibapply - Applies a section of a base image file to level files in levels/ folder\n"
		"usage: ibapply [title | level] [x y w h]\n"
		"\n"
		"If [base file type] is title, then load titlebase.bmp and apply to title levels.\n"
		"Likewise if [base file type] is level, then load levelbase.bmp and apply to normal levels.\n"
	);
}

#define TITLE_PREFIX "_title"

static bool is_correct_file(struct dirent *ent, bool levelty) {
	if (ent->d_type != DT_REG) return false;
	const char *ext = strrchr(ent->d_name, '.');
	if (!ext || strcmp(ext, ".bmp") != 0) return false;
	const bool is_level = strncmp(ent->d_name, TITLE_PREFIX, sizeof(TITLE_PREFIX)) != 0;
	return is_level == levelty;
}

static char **get_level_bmps(const char *dirpath, bool levelty) {
	DIR *dir = opendir(dirpath);
	if (!dir) return NULL;

	struct dirent *ent;
	int nfiles = 0;
	char **bmps = NULL;
	while ((ent = readdir(dir))) {
		nfiles += is_correct_file(ent, levelty);
	}
	if (!nfiles) goto cleanup;
	rewinddir(dir);
	int i = 0;
	bmps = malloc(sizeof(*bmps) * (nfiles + 1));
	while ((ent = readdir(dir))) {
		if (!is_correct_file(ent, levelty)) continue;
		bmps[i] = malloc(strlen(ent->d_name) + 1);
		strcpy(bmps[i], ent->d_name);
		i++;
	}
	bmps[i] = NULL;

cleanup:
	closedir(dir);
	return bmps;
}

int main(int argc, char **argv) {
	if (argc != 6) {
		print_help();
		return 1;
	}

	union {
		struct {
			int x, y, w, h;
		};
		int arr[4];
	} sect;
	for (int i = 2; i < 6; i++) {
		const char *c = argv[i];
		while (c) {
			if (!isdigit(*(c++))) {
				print_help();
				return 1;
			}
		}
		sect.arr[i-2] = atoi(argv[i]);
	}

	bool levelty = strcmp(argv[1], "level") == 0;
	if (!levelty && strcmp(argv[1], "title") != 0) {
		print_help();
		return 1;
	}
	bmp_t base;
	if (!bmp_load(&base, levelty ? "levelbase.bmp" : "titlebase.bmp")) {
		printf("error: couldn't load input file\n");
		return 1;
	}
	if (sect.x < 0 || sect.y < 0 || sect.x + sect.w > base.w || sect.y + sect.h > base.h) {
		printf("error: section is outside of base image!\n");
		bmp_deinit(&base);
		return 1;
	}

	char **bmps = get_level_bmps("levels/", levelty);
	if (!bmps) {
		printf("warning: no levels bitmaps to update\n");
		bmp_deinit(&base);
		return 1;
	}

	int ret = 1;
	for (int i = 0; bmps[i]; i++) {
		bmp_t bmp;
		if (!bmp_load(&bmp, bmps[i])) {
			printf("error: couldn't load output file %s\n", bmps[i]);
			goto cleanup;
		}
		if (sect.x < 0 || sect.y < 0 || sect.x + sect.w > bmp.w || sect.y + sect.h > bmp.h) {
			printf("error: section is outside of output image %s!\n", bmps[i]);
			goto cleanup;
		}
		
		for (int y = sect.y; y < sect.y + sect.h; y++) {
			for (int x = sect.x; x < sect.x + sect.w; x++) {
				GET_PIXEL(bmp, x, y) = GET_PIXEL(base, x, y);
			}
		}
		
		if (!bmp_save(&bmp, bmps[i])) {
			printf("error: couldn't save output bmp %s\n", bmps[i]);
			goto cleanup;
		}
		bmp_deinit(&bmp);
	}

	ret = 0;
cleanup:
	for (int i = 0; bmps[i]; i++) free(bmps[i]);
	free(bmps);
	bmp_deinit(&base);
	return ret;
}

