#if 0
gcc -s -O2 -o ~/bin/neo2ff -Wno-unused-result neo2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char x[8];
} Color;

static unsigned char head[128];
static Color pal[16];
static unsigned char pic[32000];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: neo2ff <?colour animation number?>                                     \n"
		"                                                                              \n"
		"Convert NEOchrome image to farbfeld. neo2ff reads a NEO image from stdin,     \n"
		"converts it to farbfeld and writes the result to stdout.                      \n"
		"                                                                              \n"
		"NEOchrome format is a raster image file format associated with the NEOchrome  \n"
		"graphics editor for Atari ST.                                                 \n"
		"                                                                              \n"
		"This decoder optionally takes one argument, which is the colour animation     \n"
		"number; if omitted; zero is assumed.                                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ neo2ff < IMAGE.NEO > image.ff                                            \n"
		"   $ neo2ff < IMAGE.NEO | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

static void
cycle_palette(int n)
{
	int a=head[49]>>4;
	int b=head[49]&15;
	int c=(b-a)<<1;
	int d=(a<<1)+4;
	while (n<0) n+=b-a+1;
	while (n--) {
		memmove(head+d+2,head+d,c);
		memcpy(head+d,head+c+d,2);
	}
}

static void
do_palette(void)
{
	int i;
	unsigned char*p=head+4;
	for (i=0; i<16; i++) {
		int r=((p[0]&7)*0x9249)>>2;
		int g=(((p[1]>>4)&7)*0x9249)>>2;
		int b=((p[1]&7)*0x9249)>>2;
		pal[i].x[0]=r>>8; pal[i].x[1]=r;
		pal[i].x[2]=g>>8; pal[i].x[3]=g;
		pal[i].x[4]=b>>8; pal[i].x[5]=b;
		pal[i].x[6]=pal[i].x[7]=255;
		p+=2;
	}
}

static void
do_picture_low(void)
{
	unsigned char*p=pic;
	int n,i,x;
	for (n=0; n<4000; n++) {
		for(x=0; x<16; x++) {
			i=0;
			if (p[(x>>3)+0]&(128>>(x&7))) i|=1;
			if (p[(x>>3)+2]&(128>>(x&7))) i|=2;
			if (p[(x>>3)+4]&(128>>(x&7))) i|=4;
			if (p[(x>>3)+6]&(128>>(x&7))) i|=8;
			fwrite(pal[i].x,1,8,stdout);
		}
		p+=8;
	}
}

static void
do_picture_med(void)
{
	unsigned char*p=pic;
	int n,i,x;
	for (n=0; n<8000; n++) {
		for (x=0; x<16; x++) {
			i=0;
			if (p[(x>>3)+0]&(128>>(x&7))) i|=1;
			if (p[(x>>3)+2]&(128>>(x&7))) i|=2;
			fwrite(pal[i].x,1,8,stdout);
		}
		p+=4;
	}
}

int
main(int argc, char **argv)
{
	if ((argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) || argc > 2)
		usage();

	fread(head,1,128,stdin);
	fread(pic,1,32000,stdin);
	if (argc>1 && argv[1][0] && (head[48]&128)) cycle_palette(strtol(argv[1],0,0));
	do_palette();
	switch(head[3]) {
		case 0:
			fwrite("farbfeld\0\0\x01\x40\0\0\x00\xC8",1,16,stdout);
			do_picture_low();
			break;
		case 1:
			fwrite("farbfeld\0\0\x02\x80\0\0\x00\xC8",1,16,stdout);
			do_picture_med();
			break;
		default:
			fprintf(stderr,"Invalid screen mode %d\n",head[3]);
	}
	return 0;
}
