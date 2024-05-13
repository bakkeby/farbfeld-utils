#if 0
gcc -s -O2 -o ~/bin/jef2ff -Wno-unused-result jef2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[116];
static int width,height,xcur,ycur,tcur,tcount,count;
static char*thread;
static char*pic;

#if 0
static const int pal[256]={
	-1, // (unused)
	//     1         2         3         4         5         6         7         8         9        10
	0x000000, 0xF0F0F0, 0xFFCC00, 0xE6651E, 0xC4E39D, 0x237336, 0x071650, 0x4CB58F, 0xF669A0, 0xFF4720,
	0xE2A188, 0x595B61, 0xE4C35D, 0x6231BD, 0x2F5933, 0xFAB381, 0xF999B7, 0xF09C96, 0xA39166, 0x970533,
	0xAC9CC7, 0x65C2C8, 0xE5E5E5, 0xA0B8CC, 0x98D6BD, 0x0B2F84, 0x98F3FE, 0xB2E1E3, 0xFF0927, 0x14329C,
	0xA80043, 0xFF6600, 0xFF6048, 0xB59474, 0xFDF5B5, 0xF5DB8B, 0xC79732, 0x889B9B, 0xAB5A96, 0xFFBDE3,
	0xC3007E, 0xFF0000, 0xEE71AF, 0x608541, 0x609418, 0xC6EE6B, 0x5BD2B5, 0xFFFF17, 0x04917B, 0x5C2625,
	0xFFFFDC, 0xFF5A27, 0xA76C3D, 0x9C6445, 0xB45A30, 0x481A05, 0x0C8918, 0x70A9E2, 0x1D5478, 0x165FA7,
	0x7FC21C, 0x06480D, 0x843154, 0xFD33A3, 0xFFBBBB, 0xF7F297, 0x00B552, 0xFCF121, 0xE6965A, 0xD7BDA4,
	0xFF9D00, 0xFFBA5E, 0x0257B5, 0x6E3937, 0x540571, 0xCC9900, 0xD0BAB0, 0xE3BE81,
};
#else
#define RGB(r,g,b) (r<<16)|(g<<8)|b
static const int pal[256] = {
		-1, // (unused)
		RGB(0,0,0), // Black
		RGB(255,255,255), // White
		RGB(255,255,23), // Yellow
		RGB(250,160,96), // Orange
		RGB(92,118,73), // Olive Green
		RGB(64,192,48), // Green
		RGB(101,194,200), // Sky
		RGB(172,128,190), // Purple
		RGB(245,188,203), // Pink
		RGB(255,0,0), // Red
		RGB(192,128,0), // Brown
		RGB(0,0,240), // Blue
		RGB(228,195,93), // Gold
		RGB(165,42,42), // Dark Brown
		RGB(213,176,212), // Pale Violet
		RGB(252,242,148), // Pale Yellow
		RGB(240,208,192), // Pale Pink
		RGB(255,192,0), // Peach
		RGB(201,164,128), // Beige
		RGB(155,61,75), // Wine Red
		RGB(160,184,204), // Pale Sky
		RGB(127,194,28), // Yellow Green
		RGB(185,185,185), // Silver Grey
		RGB(160,160,160), // Grey
		RGB(152,214,189), // Pale Aqua
		RGB(184,240,240), // Baby Blue
		RGB(54,139,160), // Powder Blue
		RGB(79,131,171), // Bright Blue
		RGB(56,106,145), // Slate Blue
		RGB(0,32,107), // Nave Blue
		RGB(229,197,202), // Salmon Pink
		RGB(249,103,107), // Coral
		RGB(227,49,31), // Burnt Orange
		RGB(226,161,136), // Cinnamon
		RGB(181,148,116), // Umber
		RGB(228,207,153), // Blonde
		RGB(225,203,0), // Sunflower
		RGB(225,173,212), // Orchid Pink
		RGB(195,0,126), // Peony Purple
		RGB(128,0,75), // Burgundy
		RGB(160,96,176), // Royal Purple
		RGB(192,64,32), // Cardinal Red
		RGB(202,224,192), // Opal Green
		RGB(137,152,86), // Moss Green
		RGB(0,170,0), // Meadow Green
		RGB(33,138,33), // Dark Green
		RGB(93,174,148), // Aquamarine
		RGB(76,191,143), // Emerald Green
		RGB(0,119,114), // Peacock Green
		RGB(112,112,112), // Dark Grey
		RGB(242,255,255), // Ivory White
		RGB(177,88,24), // Hazel
		RGB(203,138,7), // Toast
		RGB(247,146,123), // Salmon
		RGB(152,105,45), // Cocoa Brown
		RGB(162,113,72), // Sienna
		RGB(123,85,74), // Sepia
		RGB(79,57,70), // Dark Sepia
		RGB(82,58,151), // Violet Blue
		RGB(0,0,160), // Blue Ink
		RGB(0,150,222), // Solar Blue
		RGB(178,221,83), // Green Dust
		RGB(250,143,187), // Crimson
		RGB(222,100,158), // Floral Pink
		RGB(181,80,102), // Wine
		RGB(94,87,71), // Olive Drab
		RGB(76,136,31), // Meadow
		RGB(228,220,121), // Mustard
		RGB(203,138,26), // Yellow Ochre
		RGB(198,170,66), // Old Gold
		RGB(236,176,44), // Honeydew
		RGB(248,128,64), // Tangerine
		RGB(255,229,5), // Canary Yellow
		RGB(250,122,122), // Vermillion
		RGB(107,224,0), // Bright Green
		RGB(56,108,174), // Ocean Blue
		RGB(227,196,180), // Beige Grey
		RGB(227,172,129), // Bamboo
};
#endif

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); exit(1); }while(0)

static void
usage(void)
{
	fprintf(stderr,
		"Usage: jef2ff                                                                 \n"
		"                                                                              \n"
		"Convert Janome Embroidery Format image to farbfeld. jef2ff reads an JEF image \n"
		"from stdin, converts it to farbfeld and writes the result to stdout.          \n"
		"                                                                              \n"
		"These are files that are used with the Janome embroidery machine models such  \n"
		"as the Janome Memory Craft range, e.g. MC200, MC300, MC350, MC9500, MC9700,   \n"
		"MC11000 and MC12000.                                                          \n"
		"                                                                              \n"
		"Each pixel represents 0.1mm and places where there are no stitches will be    \n"
		"transparent pixels (use ff-back to add an opaque background). The colours may \n"
		"be wrong in some pictures; it is unsure what exactly is the reason (possibly  \n"
		"different thread numbers from different manufacturers; official files from    \n"
		"Jenome show correct colours).                                                 \n"
		"                                                                              \n"
		"There are no command-line arguments.                                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ jef2ff < image.jef > image.ff                                            \n"
		"   $ jef2ff < image.jef | bzip2 > image.ff.bz2                                \n"
		"\n"
	);
	exit(1);
}

static void
pset(int x, int y)
{
	if (x<0 || x>=width || y<0 || y>=height) return;
	pic[y*width+x]=thread[tcur];
}

static void
draw(int x2, int y2)
{
	int x1=xcur;
	int y1=ycur;
	int dx=abs(x1-x2);
	int dy=abs(y1-y2);
	int x=x1;
	int y=y1;
	int x1i=dx>=dy?0:x2>=x1?1:-1;
	int y1i=dx<dy?0:y2>=y1?1:-1;
	int x2i=dx<dy?0:x2>=x1?1:-1;
	int y2i=dx>=dy?0:y2>=y1?1:-1;
	int n=dx>=dy?dx:dy;
	int den=n;
	int num=n>>1;
	int add=dx<dy?dx:dy;
	int i;
	for (i=0; i<=n; i++) {
		pset(x,y);
		num+=add;
		if(num>=den) {
			num-=den;
			x+=x1i;
			y+=y1i;
		}
		x+=x2i;
		y+=y2i;
	}
	xcur=x2;
	ycur=y2;
}

static void
header(void)
{
	int i,n;
	fread(buf,1,116,stdin);
	// Extents (in units of 0.1mm)
	xcur=buf[36]|(buf[37]<<8)|(buf[38]<<16)|(buf[39]<<24);
	width=xcur+(buf[44]|(buf[45]<<8)|(buf[46]<<16)|(buf[47]<<24));
	ycur=buf[40]|(buf[41]<<8)|(buf[42]<<16)|(buf[43]<<24);
	height=ycur+(buf[48]|(buf[49]<<8)|(buf[50]<<16)|(buf[51]<<24));
	pic=calloc(width,height);
	if (!pic) fatal("Allocation failed\n");
	// Thread count
	tcount=buf[24]|(buf[25]<<8)|(buf[26]<<16)|(buf[27]<<24);
	// Stitch count
	count=buf[28]|(buf[29]<<8)|(buf[30]<<16)|(buf[31]<<24);
	// File offset
	i=buf[0]|(buf[1]<<8)|(buf[2]<<16)|(buf[3]<<24);
	i-=(tcount<<3)+116;
	if (i<0) fatal("Invalid file offset (%d)\n",i);
	thread=malloc(tcount);
	if (!thread) fatal("Allocation failed\n");
	// Thread colours
	for (n=0; n<tcount; n++) {
		fread(buf,1,4,stdin);
		thread[n]=*buf;
	}
	// Thread types (unused)
	for (n=0; n<tcount; n++) fread(buf,1,4,stdin);
	// Skip the JEF+ header (if any)
	while (i--) fgetc(stdin);
}

static void
stitches(void)
{
	signed char x,y,c;
	while (count-->0) {
		x=fgetc(stdin); y=fgetc(stdin);
		if (x==-128) {
			switch(c=y) {
				case 1: tcur=(tcur+1)%tcount; break;
				case 2: x=fgetc(stdin); y=fgetc(stdin); xcur+=x; ycur-=y; --count; break;
				case 16: return;
				default: fatal("Invalid command code (%d)\n",c);
			}
		} else {
			draw(xcur+x,ycur-y);
		}
	}
}

static void
output(void)
{
	int i;
	// Header
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	// Raster
	for (i=0; i<width*height; i++) {
		if (pic[i]) {
			putchar(pal[pic[i]]>>16);
			putchar(pal[pic[i]]>>16);
			putchar(pal[pic[i]]>>8);
			putchar(pal[pic[i]]>>8);
			putchar(pal[pic[i]]);
			putchar(pal[pic[i]]);
			putchar(255); putchar(255);
		} else {
			putchar(0); putchar(0); putchar(0); putchar(0);
			putchar(0); putchar(0); putchar(0); putchar(0);
		}
	}
}

int
main(int argc, char **argv)
{
	if (argc > 1) {
		usage();
	}

	header();
	stitches();
	output();
	return 0;
}
