#if 0
gcc -s -O2 -o ~/bin/ff2mrf -Wno-unused-result ff2mrf.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static unsigned char bitval=0;
static int bitpos=128;
static int width,height,extwidth;
static unsigned char*pic;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2mrf                                                                 \n"
		"                                                                              \n"
		"Convert farbfeld to Monochrome Recursive Format (MRF) image. ff2mrf reads a   \n"
		"farbfeld image from stdin, converts it to MRF and writes the result to stdout.\n"
		"                                                                              \n"
		"MRF is a bi-level raster image file format developed by Russell Marks. MRF    \n"
		"images are always compressed. The compression scheme is somewhat unusual, and \n"
		"involves recursively subdividing tiles until each is a single color.          \n"
		"                                                                              \n"
		"The encoder takes no arguments. Only the high bit of the red channel of the   \n"
		"input is used.                                                                \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2mrf < image.ff > image.mrf                                            \n"
		"   $ bunzip2 < image.ff.bz2 | ff2mrf > image.mrf                              \n"
		"\n"
	);
	exit(1);
}

static void
out_bit(int x)
{
	if (x) bitval|=bitpos;
	bitpos>>=1;
	if (!bitpos) {
		putchar(bitval);
		bitval=0;
		bitpos=128;
	}
}

static inline void
optimize_h(void)
{
	int y;
	unsigned char*p=pic+width;
	for (y=0; y<64; y++) memset(p,2,63&-width),p+=extwidth;
}

static inline void
optimize_v(void)
{
	memset(pic+height*extwidth,2,extwidth*(64-height));
}

static void
do_tile(const unsigned char *p, int s)
{
	int x,y;
	const unsigned char*q=p;
	if (s!=1) {
		for (y=0; y<s; y++) {
			for (x=0; x<s; x++) if (q[x]!=*p && q[x]!=2) {
				out_bit(0);
				s>>=1;
				do_tile(p,s);
				do_tile(p+s,s);
				do_tile(p+extwidth*s,s);
				do_tile(p+(extwidth+1)*s,s);
				return;
			}
			q+=extwidth;
		}
		out_bit(1);
	}
	out_bit(*p);
}

static inline void
do_picture(void)
{
	int x,y;
	if (height>=64) {
		for (y=0; y<64; y++) for (x=0; x<width; x++) {
			fread(buf,1,8,stdin);
			pic[y*extwidth+x]=*buf&128?1:0;
		}
		height-=64;
	} else {
		for (y=0; y<height; y++) for (x=0; x<width; x++) {
			fread(buf,1,8,stdin);
			pic[y*extwidth+x]=*buf&128?1:0;
		}
		optimize_v();
		height=0;
	}
	if (width&63) optimize_h();
	for (x=0; x<extwidth; x+=64) do_tile(pic+x,64);
}

int
main(int argc, char **argv)
{
	if (argc > 1) {
		usage();
	}

	fread(buf,1,16,stdin);
	if (memcmp(buf,"farbfeld",8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	pic=calloc(64,extwidth=(width+63)&~63);
	if (!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite("MRF1",1,4,stdout);
	fwrite(buf+8,1,8,stdout);
	putchar(0);
	while (height) do_picture();
	if (bitpos!=128) putchar(bitval);
	return 0;
}
