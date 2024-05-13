#if 0
gcc -s -O2 -o ~/bin/pmart2ff -Wno-unused-result pmart2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char d[8];
} Color;

static unsigned char pic[0x8000];
static Color pal[16];
static Color npal[16];
static int nscan;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: pmart2ff                                                               \n"
		"                                                                              \n"
		"Convert Palette Master ART image to farbfeld. pmart2ff reads an ART image from\n"
		"stdin, converts it to farbfeld and writes the result to stdout.               \n"
		"                                                                              \n"
		"Palette Master is an obscure paint program for the Atari ST, developed by     \n"
		"Kevin Cowtan. It uses palette switching to display more than 16 colors at a   \n"
		"time. It has a native .ART image format.                                      \n"
		"                                                                              \n"
		"This decoder takes no arguments.                                              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ pmart2ff < IMAGE.ART > image.ff                                          \n"
		"   $ pmart2ff < IMAGE.ART | bzip2 > image.ff.bz2                              \n"
		"\n"
	);
	exit(1);
}

static inline int
decode_color(int n)
{
	return 0x11*(((n&7)<<1)|((n&8)>>3));
}

static void
readpal(Color *p)
{
	int i,x,y,z;
	for (i=0; i<16; i++) {
		x=fgetc(stdin); y=fgetc(stdin);
		if (!i) nscan=(x<<8)|y;
		p[i].d[0]=p[i].d[1]=decode_color(x);
		p[i].d[2]=p[i].d[4]=decode_color(y>>4);
		p[i].d[4]=p[i].d[5]=decode_color(y);
		p[i].d[6]=p[i].d[7]=255;
	}
	if (p==npal) *npal=*pal;
}

static int
getpixel(const unsigned char *p, int n)
{
	int i=0;
	i|=p[0]&(128>>n)?1:0;
	i|=p[2]&(128>>n)?2:0;
	i|=p[4]&(128>>n)?4:0;
	i|=p[6]&(128>>n)?8:0;
	return i;
}

int
main(int argc, char **argv)
{
	int i,x,y;
	const unsigned char*p=pic;

	if (argc > 1) {
		usage();
	}

	fread(pic,1,0x8000,stdin);
	fwrite("farbfeld\0\0\x01\x40\0\0\x00\xC8",1,16,stdout);
	readpal(pal);
	readpal(npal);
	for (y=0; y<200; y++) {
		while (nscan==y) {
			memcpy(pal,npal,sizeof(pal));
			readpal(npal);
		}
		for (x=0; x<320; x+=16) {
			fwrite(pal[getpixel(p,0)].d,1,8,stdout);
			fwrite(pal[getpixel(p,1)].d,1,8,stdout);
			fwrite(pal[getpixel(p,2)].d,1,8,stdout);
			fwrite(pal[getpixel(p,3)].d,1,8,stdout);
			fwrite(pal[getpixel(p,4)].d,1,8,stdout);
			fwrite(pal[getpixel(p,5)].d,1,8,stdout);
			fwrite(pal[getpixel(p,6)].d,1,8,stdout);
			fwrite(pal[getpixel(p,7)].d,1,8,stdout);
			fwrite(pal[getpixel(p+1,0)].d,1,8,stdout);
			fwrite(pal[getpixel(p+1,1)].d,1,8,stdout);
			fwrite(pal[getpixel(p+1,2)].d,1,8,stdout);
			fwrite(pal[getpixel(p+1,3)].d,1,8,stdout);
			fwrite(pal[getpixel(p+1,4)].d,1,8,stdout);
			fwrite(pal[getpixel(p+1,5)].d,1,8,stdout);
			fwrite(pal[getpixel(p+1,6)].d,1,8,stdout);
			fwrite(pal[getpixel(p+1,7)].d,1,8,stdout);
			p+=8;
		}
	}
	return 0;
}
