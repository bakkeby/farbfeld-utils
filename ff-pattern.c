#if 0
gcc -s -O2 -o ~/bin/ff-pattern -Wno-unused-result ff-pattern.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); fputc('\n',stderr); exit(1); }while(0)

typedef struct {
	unsigned char c[8];
	int w,h;
	unsigned char*data;
} Picture;

static unsigned char buf[16];
static int width,height,npict;
static Picture*pict;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-pattern <?colour?> <?image.ff?> <?colour?> <?image2.ff?> ...        \n"
		"                                                                              \n"
		"This farbfeld filter is used to convert solid colours into patterns.          \n"
		"                                                                              \n"
		"Command-line arguments alternate colours and pictures.                        \n"
		"                                                                              \n"
		"Any pixel of one of the specified colours will be replaced with a pixel from  \n"
		"that picture; the X and Y coordinates are modulo the size of that picture in  \n"
		"order to determine what colour the resulting pixel will be.                   \n"
		"                                                                              \n"
		"The colour can be defined through one of the following patterns:              \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"This is perhaps better explained with a practical example.                    \n"
		"                                                                              \n"
		"First we can generate a chess board that has the colour DDDDDD for white and  \n"
		"222222 for black:                                                             \n"
		"   $ ff-chess 8 8 64 DDDDDD 222222 > chess.ff                                 \n"
		"                                                                              \n"
		"Then we can use ff-pattern to fill e.g. the black squares with a parts from   \n"
		"another image by passing the colour of the black squares and an image file:   \n"
		"   $ ff-pattern 222222 bananas.ff < chess.ff > image.ff                       \n"
		"                                                                              \n"
		"We could also fill both the white and black squares by passing more arguments:\n"
		"   $ ff-pattern 222222 bananas.ff DDDDDD carrots.ff < chess.ff > image.ff     \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-pattern 906060 other.ff < image.ff > image-pattern.ff                 \n"
		"\n"
	);
	exit(1);
}

static inline void load_picture(Picture*p,const char*fn) {
	FILE*fp=fopen(fn,"r");
	if(!fp) {
		perror(fn);
		exit(1);
	}
	fread(buf,1,16,fp);
	p->w=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	p->h=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	p->data=malloc(p->w*p->h*8);
	if(!p->data) fatal("Allocation failed");
	fread(p->data,p->w*p->h,8,fp);
	fclose(fp);
}

static inline unsigned char*do_pixel(int x,int y) {
	int i;
	for(i=0;i<npict;i++) if(!memcmp(pict[i].c,buf,8)) return pict[i].data+8*(x%pict[i].w+(y%pict[i].h)*pict[i].w);
	return buf;
}

#define SC(a,b) sscanf(argv[i+i+1]+b,"%2hhX",pict[i].c+a)

int main(int argc,char**argv) {
	int i,j;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(!(argc&1)) fatal("Incorrect number of arguments");
	pict=malloc((npict=argc>>1)*sizeof(Picture));
	if(npict && !pict) fatal("Allocation failed");
	for(i=0;i<npict;i++) {
		pict[i].c[6]=pict[i].c[7]=255;
		j=strlen(argv[i+i+1]);
		if(j==6) SC(0,0),SC(1,0),SC(2,2),SC(3,2),SC(4,4),SC(5,4);
		else if(j==8) SC(0,0),SC(1,0),SC(2,2),SC(3,2),SC(4,4),SC(5,4),SC(6,6),SC(7,6);
		else if(j==12) SC(0,0),SC(1,2),SC(2,4),SC(3,6),SC(4,8),SC(5,10);
		else if(j==16) SC(0,0),SC(1,2),SC(2,4),SC(3,6),SC(4,8),SC(5,10),SC(6,12),SC(7,14);
		else fatal("Invalid color");
		load_picture(pict+i,argv[i+i+2]);
	}
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	fwrite(buf,1,16,stdout);
	for(i=j=0;j!=height;) {
		fread(buf,1,8,stdin);
		fwrite(do_pixel(i,j),1,8,stdout);
		if(++i==width) i=0,++j;
	}
	return 0;
}
