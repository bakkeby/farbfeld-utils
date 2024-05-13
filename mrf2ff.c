#if 0
gcc -s -O2 -o ~/bin/mrf2ff -Wno-unused-result mrf2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static unsigned char bitval;
static int bitpos=0;
static int width,height,extwidth;
static unsigned char*pic;
static char opt[128];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: mrf2ff <?option?>                                                      \n"
		"                                                                              \n"
		"Convert Monochrome Recursive Format (MRF) image to farbfeld. mrf2ff reads an  \n"
		"MRF image from stdin, converts it to farbfeld and writes the result to stdout.\n"
		"                                                                              \n"
		"MRF is a bi-level raster image file format developed by Russell Marks. MRF    \n"
		"images are always compressed. The compression scheme is somewhat unusual, and \n"
		"involves recursively subdividing tiles until each is a single color.          \n"
		"                                                                              \n"
		"The decoder has one optional argument, which is zero or more letters to       \n"
		"indicate the options:                                                         \n"
		"   a = round up the width and height to a multiple of sixty-four              \n"
		"   c = display colours to indicate the encoding                               \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ mrf2ff c < image.mrf > image.ff                                          \n"
		"   $ mrf2ff ac < image.mrf > image.ff                                         \n"
		"   $ mrf2ff < image.mrf | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

static int
in_bit(void)
{
	int i;
	if (!bitpos) {
		bitval=fgetc(stdin);
		bitpos=128;
	}
	i=bitval&bitpos;
	bitpos>>=1;
	return i?1:0;
}

static void
do_tile(unsigned char *p, int s)
{
	int x,y,z;
	unsigned char*q=p;
	if (s==1 || in_bit()) {
		z=in_bit();
		for (y=0; y<s; y++) {
			for (x=0; x<s; x++) q[x]=z;
			q+=extwidth;
		}
	} else {
		s>>=1;
		do_tile(p,s);
		do_tile(p+s,s);
		do_tile(p+extwidth*s,s);
		do_tile(p+(extwidth+1)*s,s);
		return;
	}
}

static inline void
do_picture(void)
{
	int x,y;
	for (x=0; x<extwidth; x+=64) do_tile(pic+x,64);
	if (height>64) {
		for (y=0; y<64; y++) for (x=0; x<width; x++) fwrite(buf+pic[y*extwidth+x]*8,1,8,stdout);
		height-=64;
	} else {
		for (y=0; y<height; y++) for (x=0; x<width; x++) fwrite(buf+pic[y*extwidth+x]*8,1,8,stdout);
		height=0;
	}
}

static void
do_tile_c(unsigned char *p, int s)
{
	int x,y,z;
	unsigned char*q=p;
	if (s==1 || in_bit()) {
		z=(in_bit()<<7)|s;
		for (y=0; y<s; y++) {
			for (x=0; x<s; x++) q[x]=z;
			q+=extwidth;
		}
	} else {
		s>>=1;
		do_tile_c(p,s);
		do_tile_c(p+s,s);
		do_tile_c(p+extwidth*s,s);
		do_tile_c(p+(extwidth+1)*s,s);
		return;
	}
}

static inline void
do_picture_c(void)
{
	int x,y,z;
	for (x=0; x<extwidth; x+=64) do_tile_c(pic+x,64);
	for (y=0; y<height && y<64; y++) {
		for (x=0; x<width; x++) {
			z=pic[y*extwidth+x];
			buf[0]=buf[2]=buf[4]=z&128;
			if (z&0x55) buf[0]|=64;
			if (z&0x66) buf[2]|=64;
			if (z&0x78) buf[4]|=64;
			buf[1]=buf[0]; buf[3]=buf[2]; buf[5]=buf[4];
			fwrite(buf,1,8,stdout);
		}
	}
	height-=y;
}

int
main(int argc, char **argv)
{
	if ((argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) || argc > 2) {
		usage();
	}

	fread(buf,1,13,stdin);
	if (memcmp(buf,"MRF1",4) || buf[12]) {
		fprintf(stderr,"Unrecognized file format\n");
		return 1;
	}
	width=(buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|buf[7];
	height=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	pic=calloc(64,extwidth=(width+63)&~63);
	if (!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	if (argc>1 && argv[1][0]) {
		int i;
		for (i=0; argv[1][i]; i++) opt[argv[1][i]&127]=1;
	}
	if (opt['a']) {
		width=extwidth;
		height=(height+63)&~63;
		buf[4]=width>>24; buf[5]=width>>16; buf[6]=width>>8; buf[7]=width;
		buf[8]=height>>24; buf[9]=height>>16; buf[10]=height>>8; buf[11]=height;
	}
	fwrite("farbfeld",1,8,stdout);
	fwrite(buf+4,1,8,stdout);
	if (opt['c']) {
		buf[6]=buf[7]=255;
		while (height) do_picture_c();
	} else {
		buf[0]=buf[1]=buf[2]=buf[3]=buf[4]=buf[5]=0;
		buf[6]=buf[7]=buf[8]=buf[9]=buf[10]=buf[11]=buf[12]=buf[13]=buf[14]=buf[15]=255;
		while (height) do_picture();
	}
	return 0;
}
