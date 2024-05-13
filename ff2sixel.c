#if 0
gcc -s -O2 -o ~/bin/ff2sixel -Wno-unused-result ff2sixel.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2sixel                                                               \n"
		"                                                                              \n"
		"Convert farbfeld image to sixel graphics. ff2sixel reads a farbfeld image from\n"
		"stdin, converts it to sixel graphics and writes the result to stdout.         \n"
		"                                                                              \n"
		"The tool takes no arguments and outputs the sixel data including the escape   \n"
		"codes to begin and end the data.                                              \n"
		"                                                                              \n"
		"The tool does not support colour reduction and will reject the image if it has\n"
		"more than 64 colours. Consider combining with ff-reduce.                      \n"
		"                                                                              \n"
		"Sixel is a raster graphics format used by some DEC terminals and printers. It \n"
		"consists of control codes and data such that, when printed to the terminal, an\n"
		"image will be displayed.                                                      \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2sixel < image.ff                                                      \n"
		"   $ bunzip2 < image.ff.bz2 | ff2sixel                                        \n"
		"\n"
	);
	exit(1);
}

typedef struct {
	char r;
	char g;
	char b;
} Color;

typedef struct {
	char line[16];
	unsigned char count[16];
} Strip;

static Color pal[64];
static int colors=0;
static unsigned int width;
static unsigned int height;
static char*picture;
static size_t insize;
static unsigned short*stripuse;
static Strip*strip;
static int nextcolor;
static int lastcolor;
static unsigned char buf[8];

static char
get_color(int r, int g, int b)
{
	int i;
	r=(100LL*r)/65535;
	g=(100LL*g)/65535;
	b=(100LL*b)/65535;
	for (i=1;i<=colors;i++) if (pal[i].r==r && pal[i].g==g && pal[i].b==b) return i;
	if (colors==63) {
		fprintf(stderr,"Too many colors.");
		exit(1);
	}
	i=++colors;
	pal[i].r=r;
	pal[i].g=g;
	pal[i].b=b;
	return i;
}

static void
do_strip(int c)
{
	int u=stripuse[c]&stripuse[c+1]?:stripuse[c];
	char*p=picture+c*width*6;
	char o[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int s[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int i,j,n;
	if (!u) {
		// This line is completely transparent
		putchar('-');
		return;
	}
	// Fill strip arrays
	memset(strip,0,width*sizeof(Strip));
	for (i=width-1; i>=0; i--) {
		for (j=0; j<6; j++) {
			strip[i].line[p[i+width*j]]|=1<<j;
			s[p[i+width*j]]++;
		}
		for (j=1; j<16; j++) strip[i].count[j]=(i<width-1 && strip[i].line[j]==strip[i+1].line[j])?((strip[i+1].count[j]+1)&255?:1):1;
	}
	for (j=1; j<16; j++) for (i=width-1; i>=0 && !strip[i].line[j]; i--) strip[i].count[j]=0;
	// Last color
	lastcolor=0;
	for (n=0, i=1; i<16; i++) {
		if (stripuse[c]&(1<<i)) j=i,n++;
		if (i!=nextcolor && (u&(1<<i))) lastcolor=i;
	}
	if (!lastcolor) lastcolor=j;
	// Figure out order
	o[0]=nextcolor?:j;
	o[n-1]=lastcolor==o[0]?0:lastcolor;
	s[o[0]]=s[o[n-1]]=s[0]=0;
	if (o[n-1]) n--;
	while (n) {
		for (j=0, i=1; i<16; i++) if (s[i]>s[j]) j=i;
		if (!j) break;
		o[--n]=j;
		s[j]=0;
	}
	// Improve RLE of strips based on order
	//TODO
	// Output the strip
	if (nextcolor!=*o) printf("#%d",*o);
	for (i=0; i<16; i++) if (o[i]) {
		if (i) printf("$#%d",o[i]);
		for (j=0; j<width;) {
			n=strip[j].count[o[i]];
			if (!n) break;
			if (n>2) printf("!%d",n); else if (n==2) putchar(strip[j].line[o[i]]+'?');
			putchar(strip[j].line[o[i]]+'?');
			j+=n;
		}
		nextcolor=o[i];
	}
	// End of this strip; go to next
	putchar('-');
}

int
main(int argc, char **argv)
{
	int i,j;

	if (argc > 1) {
		usage();
	}

	// Load header
	fread(buf,1,8,stdin);
	if (memcmp(buf,"farbfeld",8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	fread(buf,1,8,stdin);
	width=(buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
	height=(buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|buf[7];
	// Convert to indexed colors
	picture=calloc(width,height+6);
	if (!picture) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	for (i=0; i<width*height; i++) {
		fread(buf,1,8,stdin);
		picture[i]=buf[6]&128?get_color((buf[0]<<8)|buf[1],(buf[2]<<8)|buf[3],(buf[4]<<8)|buf[5]):0;
	}
	// Figure out use of colors per strip
	stripuse=calloc(sizeof(unsigned short),(height+5)/6+1);
	for (i=width*height-1;i>=0;--i) if (picture[i]) stripuse[i/(width*6)]|=1<<picture[i];
	// Begin output
	printf("\ePq");
	for (i=1; i<=colors; i++) printf("#%d;2;%d;%d;%d",i,pal[i].r,pal[i].g,pal[i].b);
	// Make each strip
	strip=calloc(sizeof(Strip),width);
	for (i=0; i<(height+5)/6; i++) do_strip(i);
	// Finished
	printf("\e\\");
	return 0;
}
