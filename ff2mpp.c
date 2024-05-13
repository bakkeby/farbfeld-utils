#if 0
gcc -s -O2 -o ~/bin/ff2mpp -Wno-unused-result ff2mpp.c -lm
exit
#endif

/*
	This program is based on "bmp2mpp" software by Zerkman / Sector One.
	This version is public domain, as permitted by WTFPL.

	Original license information:
	 * This program is free software. It comes without any warranty, to
	 * the extent permitted by applicable law. You can redistribute it
	 * and/or modify it under the terms of the Do What The Fuck You Want
	 * To Public License, Version 2, as published by Sam Hocevar.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#define MAX_NCOLORS 64
#define MAX_WIDTH 416
#define MAX_HEIGHT 274
#define MAX_XDELTA 160

#define DEFAULT_OPT_LEVEL 3
#define ANNEAL_INIT_T_RATIO 1

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2mpp                                                                 \n"
		"                                                                              \n"
		"Convert farbfeld to Multi Palette Picture (Atari ST) image. ff2mpp reads a    \n"
		"farbfeld image from stdin, converts it to MPP and writes the result to stdout.\n"
		"                                                                              \n"
		"MPP is a raster graphics format designed to be used with Atari ST and STE     \n"
		"computers. The MPP file format supports different kinds of images. It has a   \n"
		"modular-based structure, making the format easily expandable by adding source \n"
		"or binary plugins into the file viewer.                                       \n"
		"                                                                              \n"
		"Command-line arguments may be either - followed by one or more switches, or   \n"
		"key=value, or a number 0 to 9 to specify the optimisation level (3 by         \n"
		"default).                                                                     \n"
		"                                                                              \n"
		"Possible switches are:                                                        \n"
		"   0 = Select mode 0 (default)                                                \n"
		"   1 = Select mode 1                                                          \n"
		"   2 = Select mode 2                                                          \n"
		"   3 = Select mode 3                                                          \n"
		"   d = Enable double mode. Two pictures are stored, in order that when they   \n"
		"       are averaged (e.g. by rapidly switching between them), the result is   \n"
		"       the original.                                                          \n"
		"   e = Enable STE mode                                                        \n"
		"   h = Enable the extended header (which contains metadata)                   \n"
		"   r = Use raw output format, omitting the header. (This also causes a        \n"
		"       different format for the palette.)                                     \n"
		"   x = Enable extra mode.                                                     \n"
		"   z = Display some extra details about the conversion algorithm on stderr.   \n"
		"                                                                              \n"
		"Possible keys for key=value are:                                              \n"
		"   artt = Set the value of the ARTT field in the extended header.             \n"
		"   conv = Set the value of the CONV field in the extended header (the default \n"
		"          value is \"Farbfeld Utilities\").                                   \n"
		"   ripp = Set the value of the RIPP field in the extended header.             \n"
		"   seed = Set the random number seed (an integer). The default value is 42.   \n"
		"   titl = Set the value of the TITL field in the extended header.             \n"
		"   year = Set the value of the YEAR field in the extended header (the default \n"
		"          value is the current AD year number).                               \n"
		"                                                                              \n"
		"All command-line arguments are optional.                                      \n"
		"                                                                              \n"
		"Mode 0, 1, and 2 are 320x200; mode 3 is 416x274. The input picture must be of \n"
		"the correct size corresponding to the selected mode.                          \n"
		"                                                                              \n"
		"Note: This conversion is lossy; the output is for use with Atari ST, which    \n"
		"does not represent all 16-bits-per-channel RGB colours.                       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2mpp < image.ff > image.mpp                                            \n"
		"   $ bunzip2 < image.ff.bz2 | ff2mpp -2 -x year=\"2023\" > image.mpp          \n"
		"\n"
	);
	exit(1);
}

static void
mem_error(const void *ptr, const char *name, const char* file, int line)
{
	if (!ptr) {
		fprintf(stderr, "%s:%d:allocation for pointer `%s' failed\n",
						file, line, name);
		exit(1);
	}
}

#define MEM_ERROR(ptr) mem_error(ptr, #ptr, __FILE__, __LINE__)

typedef struct {
	int id;
	int ncolors;
	int nfixed;
	int x0;
	int (*xinc)(int);
	int xdelta;
	int width;
	int height;
} Mode;

static int xinc0(int c) { return ((c==15)?88:((c==31)?12:((c==37)?100:4))); }
static int xinc1(int c) { return (((c)&1)?16:4); }
static int xinc2(int c) { return 8; }
static int xinc3(int c) { return ((c==15)?112:((c==31)?12:((c==37)?100:4))); }

Mode modes[4] = {
	{ 0, 54, 0, 33, xinc0, 148, 320, 200 },
	{ 1, 48, 0,  9, xinc1, 160, 320, 200 },
	{ 2, 64, 0,  4, xinc2, 128, 320, 200 },
	{ 3, 54, 6, 69, xinc3, 160, 416, 274 },
};

#define IMAX(x) (16-((x>0)&&(first[x-1]!=first[x])))

#define STR_EX(x) #x
#define STR(x) STR_EX(x)

#define EXTC(a,s) ((a>>s)&0x1f)

static void
write16(void *ptr, unsigned short x)
{
	unsigned char *p = ptr;
	p[0] = x>>8;
	p[1] = x;
}

static void
write32(void *ptr, unsigned int x)
{
	unsigned char *p = ptr;
	p[0] = x>>24;
	p[1] = x>>16;
	p[2] = x>>8;
	p[3] = x;
}

static int
cdist(unsigned short a, unsigned short b)
{
	int dr = EXTC(a,10) - EXTC(b,10);
	int dg = EXTC(a,5) - EXTC(b,5);
	int db = EXTC(a,0) - EXTC(b,0);
/*	if (dr<0) dr = -dr;
	if (dg<0) dg = -dg;
	if (db<0) db = -db;
	int d = dr;
	if (dg>d) d = dg;
	if (db>d) d = db;
	return d*(dr+dg+db); */
	return dr*dr+dg*dg+db*db;
/*	return dr*19+dg*55+db*26; */
}

static int
greedy(unsigned char *ppix, unsigned short *ppal, const Mode *mode,
		const unsigned short *line, int special)
{
	static int first[MAX_WIDTH];
	static int init = 0;
	int i, j, x;
	int cost = 0;

	if (!init) {
		int nx = mode->x0;
		int idx = 0;
		int c = 0;
		for (x=0; x<mode->width; ++x) {
			if (x == nx) {
				++idx;
				nx += mode->xinc(c);
				++c;
			}
			first[x] = idx;
		}
		init=1;
	}

	if (special) {
		memset(ppal+mode->nfixed, -1, (12-mode->nfixed)*sizeof(unsigned short));
		memset(ppal+16, -1, (mode->ncolors-16)*sizeof(unsigned short));
	} else
		memset(ppal+mode->nfixed, -1, (mode->ncolors-mode->nfixed)*sizeof(unsigned short));
	memset(ppix, -1, mode->width);
	if (mode->id != 3)
		ppal[0] = 0;
	ppal[(mode->ncolors-1) & -16] = 0;

	for (i=0; i<mode->width; ++i) {
		int jmax, idx;
		unsigned short *p, c;
		x = ((i*4)+(i/(mode->width/4))+1)%mode->width;
		jmax = IMAX(x);
		p = ppal + first[x];
		c = line[x];
		idx = -1;
		for (j=0; j<jmax; ++j) {
			if (p[j] == c) {
				idx = j;
				break;
			}
		}
		if (idx == -1) {
			for (j=0; j<jmax; ++j) {
				if (p[j] == 0xffff) {
					idx = j;
					p[idx] = c;
					break;
				}
			}
		}
		if (idx == -1) {
			int dist = cdist(c, p[0]);
			idx = 0;
			for (j=1; j<jmax; ++j) {
				int d = cdist(c, p[j]);
				if (d < dist) {
					dist = d;
					idx = j;
				}
			}
			cost += dist;
		}
		ppix[x] = (idx + first[x]) & 0xf;
	}
	for (i=mode->nfixed; i<mode->ncolors+mode->nfixed; ++i)
		if (ppal[i] == 0xffff)
			ppal[i] = 0;
	return cost;
}

struct instance {
	/* instance-independent data */
	int nc;
	int xmax[MAX_NCOLORS];
	int first[MAX_WIDTH];

	/* variable data */
	const unsigned short *line;
};

struct solution {
	unsigned short pal[MAX_NCOLORS];
	unsigned char pix[MAX_WIDTH];
	int eval;
};

struct bbstr {
	/* problem and solution pointers */
	int *ppix;
	unsigned short *ppal;

	/* internal data */
	struct instance inst;

	/* current solution */
	struct solution sol;
	int bound;
	int ttl;
};

static void
instance_init(struct instance *inst, const Mode *mode)
{
	int i, x;
	int nx = mode->x0;
	int nc = 0;
	int c = 0;

	for (x=0; x<mode->width; ++x) {
		if (x == nx) {
			inst->xmax[nc] = x;
			++nc;
			nx += mode->xinc(c);
			++c;
		}
		inst->first[x] = nc;
	}
	for (i=0; i<16; ++i)
		inst->xmax[nc++] = x;
	inst->nc = nc;
}

static void
solution_init(struct solution *sol, const Mode *mode)
{
	memset(sol->pal, -1, mode->ncolors*sizeof(unsigned short));
	memset(sol->pix, -1, mode->width*sizeof(int));
	sol->eval = 0;
}

/* Compute distance array of a solution */
static void
solution_computedist(const Mode *mode, int *dist,
		struct solution *sol, const struct instance *inst)
{
	int x;
	sol->eval = 0;
	for (x=0; x<mode->width; ++x) {
		unsigned short *pal = sol->pal + inst->first[x];
		unsigned short c = inst->line[x];
		unsigned short ca = pal[(sol->pix[x]-inst->first[x]) & 0xf];
		dist[x] = cdist(c, ca);
		sol->eval += dist[x];
	}
}

/* Cost of changing palette entry palidx to color col */
static int
compute_delta(const Mode *mode, const struct solution *sol,
		const struct instance *inst, int *dist, int palidx, unsigned short col)
{
	int delta = 0;
	int x0 = (palidx<16)?0:inst->xmax[palidx-16];
	int x1 = inst->xmax[palidx];
	int x, i;
	const int *first = inst->first;
	for (x=x0; x<x1; ++x) {
		if (sol->pix[x] == (palidx&0xf) || dist[x] != 0) {
			/* cost of changing color palidx to col for pixel x */
			int f = first[x];
			unsigned short c = inst->line[x];
			int cost = cdist(c, col);
			int imax = IMAX(x);
			for (i=0; i<imax; ++i) {
				int d, idx = i+f;
				if ((idx&0xf) == (palidx&0xf))	/* ? */
					continue;
				d = cdist(c, sol->pal[idx]);
				if (d < cost)
					cost = d;
			}
			delta += cost - dist[x];
		}
	}
	return delta;
}

/* Change palette entry palidx to color col */
static void
apply_delta(struct solution *sol, const struct instance *inst,
						int dist[MAX_WIDTH], int palidx, unsigned short col)
{
	int x0 = (palidx<16)?0:inst->xmax[palidx-16];
	int x1 = inst->xmax[palidx];
	int x, i;
	const int *first = inst->first;
	sol->pal[palidx] = col;
	palidx &= 0xf;
	for (x=x0; x<x1; ++x) {
		if (sol->pix[x] == palidx || dist[x] != 0) {
			/* minimize cost of changing color palidx to col for pixel x */
			int f = first[x];
			unsigned short c = inst->line[x];
			int cost = cdist(c, sol->pal[f]);
			int best = 0;
			int imax = IMAX(x);
			for (i=1; i<imax; ++i) {
				int d = cdist(c, sol->pal[i+f]);
				if (d < cost) {
					cost = d;
					best = i;
				}
			}
			sol->pix[x] = (best+f)&0xf;
			sol->eval += cost - dist[x];
			dist[x] = cost;
		}
	}
}

static int
anneal(unsigned char *ppix, unsigned short *ppal, const Mode *mode,
		const unsigned short *line, int bnd, int opt_level, int special)
{
	struct instance inst;
	struct solution sol, best;
	int dist[MAX_WIDTH];
	int colorcnt[MAX_NCOLORS];
	unsigned short colors[MAX_NCOLORS][MAX_XDELTA];
	int colornum[MAX_NCOLORS];
	int i, j;
	int orig_eval, worst_eval;
	int threshold = opt_level * opt_level * mode->ncolors / 16;
	int nval;
	double t;
	int cool_cnt;

	instance_init(&inst, mode);
	inst.line = line;
	memcpy(&sol.pal, ppal, mode->ncolors*sizeof(unsigned short));
	memcpy(&sol.pix, ppix, mode->width);
	solution_computedist(mode, dist, &sol, &inst);
	best = sol;
	orig_eval = best.eval;
	worst_eval = orig_eval;

	/* init color arrays */
	nval = 0;
	for (i=mode->nfixed; i<inst.nc; ++i) {
		unsigned char have[32768/8];
		int x0 = (i<16)?0:inst.xmax[i-16];
		int x1 = inst.xmax[i];
		int x;
		int nh = 0;
		memset(have, 0, sizeof(have));
		for (x=x0; x<x1; ++x) {
			unsigned short c = line[x];
			int mask = 1<<(c&7);
			int pos = c/8;
			if (!(have[pos]&mask)) {
				have[pos] |= mask;
				colors[i][nh++] = c;
			}
		}
		colorcnt[i] = nh;
		if (nh > nval)
			nval = nh;
	}
	for (i=0; i<mode->nfixed; ++i) {
		colors[i][0] = sol.pal[i];
		colorcnt[i] = 1;
	}
	if (special) for(i=12; i<16; ++i) {
		colors[i][0] = sol.pal[i];
		colorcnt[i] = 1;
	}
	for (i=0; i<inst.nc; ++i) {
		colornum[i] = -1;
		for (j=0; j<colorcnt[i]; ++j) {
			if (colors[i][j] == sol.pal[i])
				colornum[i] = j;
		}
	}
	if (mode->id != 3) {
		/* Force left border color to black in non-overscan modes */
		colors[0][0] = 0;
		colorcnt[0] = 1;
		colornum[0] = 0;
	}
	colors[(mode->ncolors-1) & -16][0] = 0;
	colorcnt[(mode->ncolors-1) & -16] = 1;
	colornum[(mode->ncolors-1) & -16] = 0;

	/* simulated annealing loop */
	t = ANNEAL_INIT_T_RATIO*sol.eval;
	cool_cnt = 0;
	for (;;) {
		int palidx, num, delta;
		do {
			palidx = rand()%inst.nc;
		} while (colorcnt[palidx] < 2);
		num = rand()%(colorcnt[palidx]-1);
		num += num >= colornum[palidx];

		delta = compute_delta(mode, &sol, &inst, dist, palidx, colors[palidx][num]);

		if (rand()*(1./RAND_MAX) < exp(-delta/t)) {
			/* update solution */

			apply_delta(&sol, &inst, dist, palidx, colors[palidx][num]);

			if (sol.eval < best.eval) {
				best = sol;
				if (sol.eval == 0)
					break;
			}
		} else {
			int eval = sol.eval + delta;
			if (eval > worst_eval)
				worst_eval = eval;
		}
		++cool_cnt;
		if (cool_cnt == threshold) {
			double tf = 0.02*(worst_eval-best.eval)/(nval*.693-3);
			t = t / (1. + 100.*t/(worst_eval+1));
			if (t < tf)
				break;
			cool_cnt = 0;
		}
	}

	if (best.eval < orig_eval) {
		memcpy(ppal, &best.pal, mode->ncolors*sizeof(unsigned short));
		memcpy(ppix, &best.pix, mode->width);
		return best.eval;
	}
	return orig_eval;
}

static int
file_error(const char *progname, const char *file,  const char *msg)
{
	fprintf(stderr, "%s: %s\n", progname, msg);
	return 1;
}

static int
readle2(const char *ptr)
{
	const signed char *sptr = (const signed char *)ptr;
	const unsigned char *uptr = (const unsigned char *)ptr;
	int x;
	x = uptr[0];
	x |= sptr[1]<<8;
	return x;
}

static int
readle4(const char *ptr)
{
	const signed char *sptr = (const signed char *)ptr;
	const unsigned char *uptr = (const unsigned char *)ptr;
	int x;
	x = uptr[0];
	x |= uptr[1]<<8;
	x |= uptr[2]<<16;
	x |= sptr[3]<<24;
	return x;
}


static void
convert(FILE *fd, const unsigned char *bmp, const Mode *mode, int bits,
						 int flick, int opt, int raw_palette, int err)
{
	unsigned char *pixels, *ppix;
	unsigned short *palette;
	int i, x, y;
	int penalty0 = 0, penalty1 = 0;
	int npal, nbits, bpp;
	unsigned short *bitmap, *pbitmap;
	unsigned int tmpcol;
	pixels = malloc(mode->width*mode->height);
	palette = malloc(mode->height*(mode->ncolors-mode->nfixed)*sizeof(unsigned short));
	bitmap = malloc(mode->width/4*(mode->height-1)*sizeof(unsigned short));
	MEM_ERROR(pixels);
	MEM_ERROR(palette);
	MEM_ERROR(bitmap);

	memset(palette, 0, (mode->ncolors-mode->nfixed)*sizeof(palette[0]));
	for (y=1; y<mode->height; ++y) {
		const unsigned char *pbmp = &bmp[mode->width*3*(mode->height-1-y)];
		unsigned short *ppal = &palette[y*(mode->ncolors-mode->nfixed)-mode->nfixed];
		unsigned short line[MAX_WIDTH];
		int val, special = 0;

		for (x=0; x<mode->width; ++x) {
			int r, g, b, t;
			int mask = 0;
			if (flick && ((flick^y)&1))
				mask = 1<<(7-bits);
			t = *pbmp++;
			b = t>>(8-bits);
			b += b < (1<<bits)-1 && (t&mask) != 0;
			t = *pbmp++;
			g = t>>(8-bits);
			g += g < (1<<bits)-1 && (t&mask) != 0;
			t = *pbmp++;
			r = t>>(8-bits);
			r += r < (1<<bits)-1 && (t&mask) != 0;
			line[x] = (r << 10) | (g << 5) | b;
		}
		if (y==229) {
			memcpy(ppal+12, ppal-4, 4*sizeof(unsigned short));
			special = 1;
		}
		val = greedy(pixels+y*mode->width, ppal, mode, line, special);
		penalty0 += val;
		if (val && opt) {
			val = anneal(pixels+y*mode->width, ppal, mode, line, val, opt, special);
			penalty1 += val;
		}
		if (val && err)
			fprintf(stderr,"y=%d error=%d\n", y, val);
		if (opt) {
			char buf[16];
			if (penalty1)
				sprintf(buf, "%.02f", (double)penalty0/penalty1);
			else
				strcpy(buf, "*");
			if(err) fprintf(stderr,"y=%d gain=%s\n", y, buf);
			fflush(stdout);
		}
	}
	if (opt) {
		char buf[16];
		if (penalty1)
			sprintf(buf, "%.02f", (double)penalty0/penalty1);
		else
			strcpy(buf, "inf");
		if(err) fprintf(stderr,"Error penalty: initial %d, final %d. Gain ratio: %s\n",
			penalty0, penalty1, buf);
	}
	else if (err)
		fprintf(stderr,"Error penalty: %d.\n", penalty0);

	ppix = &pixels[mode->width];
	pbitmap = bitmap;
	npal = 0;
	nbits = 0;
	bpp = 3*bits;
	tmpcol = 0;
	for (y=1; y<mode->height; ++y) {
		for (x=0; x<mode->width; x+=16) {
			unsigned short b0=0, b1=0, b2=0, b3=0;
			for (i=0; i<16; ++i) {
				int c = *ppix++;
				b0 |= (c&1) << (15-i);
				b1 |= ((c&2)>>1) << (15-i);
				b2 |= ((c&4)>>2) << (15-i);
				b3 |= ((c&8)>>3) << (15-i);
			}
			write16(pbitmap+0, b0);
			write16(pbitmap+1, b1);
			write16(pbitmap+2, b2);
			write16(pbitmap+3, b3);
			pbitmap += 4;
		}
		for (x=0; x<(mode->ncolors-mode->nfixed); ++x) {
			unsigned short c = palette[y*(mode->ncolors-mode->nfixed)+x], ex;
			switch (bits) {
				case 3:
					if (raw_palette)
						c = ((c>>10)&0x7)<<8 | ((c>>5)&0x7)<<4 | (c&0x7);
					else
						c = ((c>>10)&0x7)<<6 | ((c>>5)&0x7)<<3 | (c&0x7);
					break;
				case 4:
					c = ((c>>10)&0xf)<<8 | ((c>>5)&0xf)<<4 | (c&0xf);
					c = ((c&0xeee)>>1) | ((c&0x111)<<3);
					break;
				case 5:
					ex = ((c>>10)&1)<<14 | ((c>>5)&1)<<13 | (c&1)<<12;
					c = ((c>>11)&0xf)<<8 | ((c>>6)&0xf)<<4 | ((c>>1)&0xf);
					c = ex | ((c&0xeee)>>1) | ((c&0x111)<<3);
					break;
				default:
					fprintf(stderr, "wrong number of bits\n");
					abort();
			}
			if (raw_palette)
				write16(&palette[y*(mode->ncolors-mode->nfixed)+x], c);
			else if (mode->id != 3 && (x == 0 || x == ((mode->ncolors-1) & -16)))
				continue;
			else if (mode->id == 2 && x >= 56)
				break;
			else {
				nbits += bpp;
				tmpcol = (tmpcol << bpp) | c;
				if (nbits >= 16) {
					nbits -= 16;
					write16(&palette[npal++], tmpcol >> nbits);
				}
			}
		}
		if (raw_palette && mode->id == 0) {
			/* specific color reordering for mode 0 */
			unsigned short tmp[6];
			memcpy(tmp, palette+y*mode->ncolors+48, 6*2);
			memmove(palette+y*mode->ncolors+22, palette+y*mode->ncolors+16, 32*2);
			memcpy(palette+y*mode->ncolors+16, tmp, 6*2);
		}
	}

	if (nbits != 0)
		write16(&palette[npal++], tmpcol << (16-nbits));

	if (raw_palette) {
		fwrite(bitmap, 2, (mode->width/4)*(mode->height-1), fd);
		fwrite(palette+(mode->ncolors-mode->nfixed), 2*(mode->ncolors-mode->nfixed),
				(mode->height-1), fd);
	}
	else {
		fwrite(palette, 2, npal, fd);
		fwrite(bitmap, 2, (mode->width/4)*(mode->height-1), fd);
	}
	free(bitmap);
	free(palette);
	free(pixels);
}

int
get_year()
{
	struct tm *tm;
	time_t today = time(NULL);
	tm = localtime(&today);
	return tm->tm_year + 1900;
}

int
main(int argc, char **argv)
{
	unsigned char header[34];
	char outfilenamebuf[512], *p, *filename, *outfilename;
	FILE *fd;
	int i,j;
	int bits;
	int opt = DEFAULT_OPT_LEVEL;
	const Mode *mode = &modes[0];
	int ste = 0;
	int extra = 0;
	int doubl = 0;
	int err = 0;
	int raw_palette = 0;
	int width, height, offset, bpp, compression;
	int randseed = 42;
	unsigned char *bmp;
	char yearbuf[8];
	int mpph = 0;
	const char *titl = "";
	const char *artt = "";
	const char *ripp = "";
	const char *conv = "Farbfeld Utilities";
	const char *year = yearbuf;

	sprintf(yearbuf, "%d", get_year());

	for (i=1; i<argc; ++i) {
		if (argv[i][0]=='-') {
			char*x=argv[i]+1;
			while(*x) switch(*x++) {
				case '0': mode=modes+0; break;
				case '1': mode=modes+1; break;
				case '2': mode=modes+2; break;
				case '3': mode=modes+3; break;
				case 'd': doubl=1; break;
				case 'e': ste=1; break;
				case 'h': mpph=1; break;
				case 'r': raw_palette=1; break;
				case 'x': extra=1; break;
				case 'z': err=1; break;
				default: usage(); break;
			}
		}
		else if (argv[i][0]>='0' && argv[i][0]<='9')
			opt=argv[i][0]-'0';
		else if (!strncasecmp(argv[i], "seed=", 5))
			randseed = strtol(argv[i]+5,0,0);
		else if (!strncasecmp(argv[i], "titl=", 5))
			titl = argv[i]+5;
		else if (!strncasecmp(argv[i], "artt=", 5))
			artt = argv[i]+5;
		else if (!strncasecmp(argv[i], "ripp=", 5))
			ripp = argv[i]+5;
		else if (!strncasecmp(argv[i], "conv=", 5))
			conv = argv[i]+5;
		else if (!strncasecmp(argv[i], "year=", 5))
			year = argv[i]+5;
		else {
			usage();
		}
	}

	fd = stdin;

	/* Analyze file header */
	fread(header, 16, 1, fd);
	if (memcmp(header, "farbfeld", 8))
		return file_error(argv[0], filename, "Not farbfeld");
	width = (header[8]<<24)|(header[9]<<16)|(header[10]<<8)|header[11];
	height = (header[12]<<24)|(header[13]<<16)|(header[14]<<8)|header[15];
	bpp = 24;
	compression = 0;
	if (width != mode->width || height != mode->height) {
		fprintf(stderr, "Image size must be %dx%d\n", mode->width, mode->height);
		return 1;
	}
	bmp = malloc(mode->width*mode->height*3);
	MEM_ERROR(bmp);
	for(i=height-1;i>=0;i--) for(j=0;j<width;j++) {
		bmp[(i*width+j)*3+2]=fgetc(fd); fgetc(fd);
		bmp[(i*width+j)*3+1]=fgetc(fd); fgetc(fd);
		bmp[(i*width+j)*3+0]=fgetc(fd); fgetc(fd);
		fgetc(fd); fgetc(fd);
	}
	//fread(bmp, 3, mode->width*mode->height, fd);

	fd = stdout;
	if (!raw_palette) {
		static char header[12] = "MPP\0\0\0\0\0\0\0\0";
		header[3] = mode->id;
		header[4] = doubl<<2 | extra<<1 | ste;
		if (mpph) {
			static struct { const char *head, *val; } mpph_items[] = {
				{ "TITL", NULL },
				{ "ARTT", NULL },
				{ "RIPP", NULL },
				{ "CONV", NULL },
				{ "YEAR", NULL }
			};
			int len, xtra = 0, all = 8;
			mpph_items[0].val = titl;
			mpph_items[1].val = artt;
			mpph_items[2].val = ripp;
			mpph_items[3].val = conv;
			mpph_items[4].val = year;
			for (i = 0; i < sizeof(mpph_items)/sizeof(mpph_items[0]); ++i) {
				len = strlen(mpph_items[i].val) + 1;
				if (len > 1)
					all += 4 + len;
			}
			all = (all+1)&-2;
			write32(header+8, all);
			fwrite(header, 12, 1, fd);
			fwrite("MPPH", 4, 1, fd);
			for (i = 0; i < sizeof(mpph_items)/sizeof(mpph_items[0]); ++i) {
				len = strlen(mpph_items[i].val) + 1;
				if (len > 1) {
					xtra += len;
					fwrite(mpph_items[i].head, 4, 1, fd);
					fwrite(mpph_items[i].val, len, 1, fd);
				}
			}
			xtra &= 1;
			fwrite("\0HPPM" + (1-xtra), 4 + xtra, 1, fd);
		} else
			fwrite(header, 12, 1, fd);
	}
	srand(randseed);
	bits = 3 + ste + extra;
	if (doubl) {
		convert(fd, bmp, mode, bits, 1, opt, raw_palette, err);
		convert(fd, bmp, mode, bits, 2, opt, raw_palette, err);
	}
	else
		convert(fd, bmp, mode, bits, 0, opt, raw_palette, err);

	//printf("Successfully written to output file `%s'.\n", outfilename);
	free(bmp);

	return 0;
}
