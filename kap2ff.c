#if 0
gcc -s -O2 -o ~/bin/kap2ff -Wno-unused-result kap2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char c[8];
} Color;

static int width,height,depth;
static Color pal[128];
static char palname[3]="RGB";

static void
usage(void)
{
	fprintf(stderr,
		"Usage: kap2ff <?palette?>                                                     \n"
		"                                                                              \n"
		"Convert BSB/KAP nautical charts to farbfeld. kap2ff reads a KAP chart file    \n"
		"from stdin, converts it to farbfeld and writes the result to stdout.          \n"
		"                                                                              \n"
		"BSB (or BSB/KAP) is a file format used primarily for the storage and          \n"
		"transmission of nautical charts. Although the format is commonly known as     \n"
		"\"BSB\", the main file extension is actually .kap (or sometimes .cap), though \n"
		".bsb files can also optionally be used. The main image data (as raster images)\n"
		"is found in .kap files, possibly with multiple images in separate files being \n"
		"part of a multi-page chart. The .bsb file, if present, has additional data    \n"
		"pertaining to the chart as a whole. Additional metadata is contained in       \n"
		"associated text files, generally stored with .bsb.txt and .kap.txt double     \n"
		"extensions, or else names ending in \"_bsb.txt\" and \"_kap.txt\".            \n"
		"                                                                              \n"
		"This decoder only supports .kap files.                                        \n"
		"                                                                              \n"
		"One optional three letter command-line argument is possible.                  \n"
		"   - if specified, it is the name of the palette to use                       \n"
		"   - if not specified, \"RGB\" is used by default                             \n"
		"                                                                              \n"
		"Palette options: RGB, DAY, DSK, NGT, NGR, GRY, PRC, PRG                       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ kap2ff < chart.kap > image.ff                                            \n"
		"   $ kap2ff DSK < chart.kap | bzip2 > image.ff.bz2                            \n"
		"\n"
	);
	exit(1);
}

static void
read_header(void)
{
	int c,c1,c2,c3,f,i;
	int s=0;
	c1=c2=c3=0;
	while (!feof(stdin)) {
		c=fgetc(stdin);
		if (c==0x1A) {
			fgetc(stdin); // skip null after end of header signal
			return;
		}
		switch(s) {
			case 0: // Ready
				if (c=='!') {
					s=1;
				} else if (c==' ') {
					if (fgetc(stdin)!=' ') goto error;
					if (fgetc(stdin)!=' ') goto error;
					if (fgetc(stdin)!=' ') goto error;
					if (c1=='B' && c2=='S' && c3=='B') s=2;
					else if (c1=='N' && c2=='O' && c3=='S') s=2;
					else if (c1==palname[0] && c2==palname[1] && c3==palname[2]) s=3;
					else s=1;
				} else if (c!='\r' && c!='\n') {
					c1=c;
					c2=fgetc(stdin);
					c3=fgetc(stdin);
					if (fgetc(stdin)!='/') goto error;
					f=i=0;
					if (c1=='B' && c2=='S' && c3=='B') s=2;
					else if (c1=='N' && c2=='O' && c3=='S') s=2;
					else if (c1==palname[0] && c2==palname[1] && c3==palname[2]) s=3;
					else s=1;
				}
				break;
			case 1: // Comment
				if (c=='\r' || c=='\n') s=0;
				break;
			case 2: // Beginning of field in "BSB" (or "NOS") block
				if (f) {
					if (c>='0' && c<='9') height=10*height+c-'0';
					if (c==',') f=0; else if (c=='\r' || c=='\n') f=s=0;
				} else {
					if (c=='\r' || c=='\n') s=0; else if (c=='R') s=4; else if (c!=',') s=5;
				}
				break;
			case 3: // "RGB" block
				if (i<0 || i>127) goto error;
				if (c=='\r' || c=='\n') {
					++f;
					s=0;
				} else if (c==',') {
					++f;
				} else if (c>='0' && c<='9') {
					if (!f) i=10*i+c-'0';
					else if (f==1) pal[i].c[0]=10*pal[i].c[0]+c-'0',pal[i].c[1]=pal[i].c[0];
					else if (f==2) pal[i].c[2]=10*pal[i].c[2]+c-'0',pal[i].c[3]=pal[i].c[2];
					else if (f==3) pal[i].c[4]=10*pal[i].c[4]+c-'0',pal[i].c[5]=pal[i].c[4];
					if (f) pal[i].c[6]=pal[i].c[7]=255;
				}
				break;
			case 4: // "BSB" block; found ",R"
				if (c=='\r' || c=='\n') s=0; else if (c==',') s=2; else if (c=='A') s=6; else s=5;
				break;
			case 5: // "BSB" block; unknown field
				if (c=='\r' || c=='\n') s=0; else if (c==',') s=2;
				break;
			case 6: // "BSB" block; found ",RA"
				if (c=='\r' || c=='\n') s=0; else if (c==',') s=2; else if (c=='=') s=7; else s=5;
				break;
			case 7: // "BSB" block; found ",RA="
				f=1;
				if (c>='0' && c<='9') width=10*width+c-'0';
				else if (c==',') s=2;
				else if (c=='\r' || c=='\n') s=0;
				break;
		}
	}
	error:
	fprintf(stderr,"Error reading header\n");
	exit(1);
}

static void
write_header(void)
{
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24);
	putchar(width>>16);
	putchar(width>>8);
	putchar(width);
	putchar(height>>24);
	putchar(height>>16);
	putchar(height>>8);
	putchar(height);
}

static inline int
read_vlq(int c)
{
	int n=0;
	while ((c&128) && !feof(stdin)) {
		n|=c&127;
		n<<=7;
		c=fgetc(stdin);
	}
	return n|c;
}

static inline void
copy_raster(void)
{
	int c,n,i;
	// Row number (not used)
	for (;;) {
		c=fgetc(stdin);
		if (c==EOF) goto error;
		if (c<128) break;
	}
	// Pixels
	for (;;) {
		c=fgetc(stdin);
		if (c==EOF) goto error;
		if (!c) return;
		i=(c&127)>>(7-depth);
		n=read_vlq(c&~(i<<(7-depth)))+1;
		while (n--) fwrite(pal[i].c,1,8,stdout);
	}
	error:
	fprintf(stderr,"Error reading raster\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	argc>1 && argv[1][0] && (palname[0]=argv[1][0]) && (palname[1]=argv[1][1]) && (palname[2]=argv[1][2]);
	read_header();
#if 0
	if (argc>2 && argv[2][0]) {
		width=8;
		height=7;
		write_header();
		fwrite(pal,8,128,stdout);
		return 0;
	}
#endif
	if (width<=0 || height<=0) {
		fprintf(stderr,"Did not find valid width/height\n");
		return 1;
	}
	write_header();
	depth=fgetc(stdin);
	if (depth<1 || depth>7) {
		fprintf(stderr,"Invalid depth (%d)\n",depth);
		return 1;
	}
	while (height--) copy_raster();
	return 0;
}
