#if 0
gcc -s -O2 -o ~/bin/mzm2ff -Wno-unused-result mzm2ff.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char d[8];
} Color;

static unsigned char buf[16];
static int width,height,cheight;
static unsigned char*font;
static unsigned char*row;
static Color pal[16];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: mzm2ff <font> <?colour?> ... <?colour?>                                \n"
		"                                                                              \n"
		"Convert MegaZeux image format to farbfeld. mzm2ff reads an MZM image from     \n"
		"stdin, converts it to farbfeld and writes the result to stdout.               \n"
		"                                                                              \n"
		"This program reads a layer-mode MZM of the format used by MegaZeux. Note that \n"
		"board-mode is not implemented. Both MZM2 and MZM3 formats can be read.        \n"
		"                                                                              \n"
		"The first argument is a name of the font file. The font file must be in PC    \n"
		"format, and it must be complete and have the correct file size. Additionally  \n"
		"the file must support fseek() and ftell()). The remaining arguments are       \n"
		"optional and specify the palette.                                             \n"
		"                                                                              \n"
		"They may be one of:                                                           \n"
		"   - omitted, to use the default PC colours                                   \n"
		"   - the name of a MegaZeux palette file (unlike the font file, this does not \n"
		"     need to be seekable)                                                     \n"
		"   - sixteen colours                                                          \n"
		"                                                                              \n"
		"The colours can be defined through one of the following patterns:             \n"
		"   - rrggbb            three pairs of 8-bit hex values                        \n"
		"   - rrggbbaa          four pairs of 8-bit hex values                         \n"
		"   - rrrrggggbbbb      three pairs of 16-bit hex values                       \n"
		"   - rrrrggggbbbbaaaa  four pairs of 16-bit hex values                        \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ mzm2ff pc.font < image.mzm > image.ff                                    \n"
		"   $ mzm2ff pc.font < image.mzm | bzip2 > image.ff.bz2                        \n"
		"\n"
	);
	exit(1);
}

static void *
allocate(int n)
{
	void*a=malloc(n);
	if (!a) {
		fprintf(stderr,"Allocation failed\n");
		exit(1);
	}
	return a;
}

static void
load_font(const char *name)
{
	FILE*fp=fopen(name,"r");
	int len;
	if (!fp) {
		perror(name);
		exit(1);
	}
	fseek(fp,0,SEEK_END);
	len=ftell(fp);
	fseek(fp,0,SEEK_SET);
	if (len&~0x7F00) {
		fprintf(stderr,"Invalid font\n");
		fclose(fp);
		exit(1);
	}
	font=allocate(len);
	cheight=len>>8;
	fread(font,1,len,fp);
	fclose(fp);
}

static void
load_palette(const char *name)
{
	FILE*fp=fopen(name,"r");
	int i;
	if (!fp) {
		perror(name);
		exit(1);
	}
	for (i=0; i<16; i++) {
		fread(buf,1,3,fp);
		pal[i].d[0]=pal[i].d[1]=(buf[0]*0x41)>>4;
		pal[i].d[2]=pal[i].d[3]=(buf[1]*0x41)>>4;
		pal[i].d[4]=pal[i].d[5]=(buf[2]*0x41)>>4;
		pal[i].d[6]=pal[i].d[7]=0xFF;
	}
	fclose(fp);
}

static void
out_row(const unsigned char *f)
{
	int i,j,co,ch,p;
	for (i=0; i<width; i++) {
		ch=row[i+i]; co=row[i+i+1];
		p=f[cheight*ch];
		for (j=0; j<8; j++) fwrite(pal[p&(128>>j)?co&15:co>>4].d,1,8,stdout);
	}
}

int
main(int argc, char **argv)
{
	int i;
	if (argc > 1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	} else if (argc==2) {
		for (i=0; i<16; i++) {
			pal[i].d[0]=pal[i].d[1]=(i&4?0xAA:0)|(i&8?0x55:0);
			pal[i].d[2]=pal[i].d[3]=(i&2?0xAA:0)|(i&8?0x55:0);
			pal[i].d[4]=pal[i].d[5]=(i&1?0xAA:0)|(i&8?0x55:0);
			pal[i].d[6]=pal[i].d[7]=0xFF;
		}
		pal[6].d[2]=pal[6].d[3]=0x55;
	} else if (argc==3) {
		load_palette(argv[2]);
	} else if (argc==18) {
		for (i=0; i<16; i++) {
			switch(strlen(argv[i+3])) {
				case 6:
					sscanf(argv[i+3],"%2hhX%2hhX%2hhX",pal[i].d+0,pal[i].d+2,pal[i].d+4);
					pal[i].d[1]=pal[i].d[0];
					pal[i].d[3]=pal[i].d[2];
					pal[i].d[5]=pal[i].d[4];
					pal[i].d[7]=pal[i].d[6]=255;
					break;
				case 8:
					sscanf(argv[i+3],"%2hhX%2hhX%2hhX%2hhX",pal[i].d+0,pal[i].d+2,pal[i].d+4,pal[i].d+6);
					pal[i].d[1]=pal[i].d[0];
					pal[i].d[3]=pal[i].d[2];
					pal[i].d[5]=pal[i].d[4];
					pal[i].d[7]=pal[i].d[6];
					break;
				case 12:
					sscanf(argv[i+3],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",pal[i].d+0,pal[i].d+1,pal[i].d+2,pal[i].d+3,pal[i].d+4,pal[i].d+5);
					pal[i].d[7]=pal[i].d[6]=255;
					break;
				case 16:
					sscanf(argv[i+3],"%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",pal[i].d+0,pal[i].d+1,pal[i].d+2,pal[i].d+3,pal[i].d+4,pal[i].d+5,pal[i].d+6,pal[i].d+7);
					break;
				default:
					fprintf(stderr,"Invalid color\n");
					return 1;
			}
		}
	} else {
		usage();
		return 1;
	}
	fread(buf,1,16,stdin);
	if (buf[0]!='M' || buf[1]!='Z' || buf[2]!='M' || buf[13]!=1 || (buf[3]!='2' && buf[3]!='3')) {
		fprintf(stderr,"Unrecognized file format\n");
		return 1;
	}
	width=buf[4]|(buf[5]>>8);
	height=buf[6]|(buf[7]>>8);
	if (buf[3]=='3') fread(buf,1,4,stdin);
	load_font(argv[1]);
	row=allocate(width<<1);
	fwrite("farbfeld",1,9,stdout);
	putchar(width>>13); putchar(width>>5); putchar(width<<3);
	i=height*cheight;
	putchar(i>>24); putchar(i>>16); putchar(i>>8); putchar(i);
	while (height--) {
		fread(row,2,width,stdin);
		for (i=0; i<cheight; i++) out_row(font+i);
	}
	return 0;
}
