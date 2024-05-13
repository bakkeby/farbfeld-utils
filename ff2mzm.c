#if 0
gcc -s -O2 -o ~/bin/ff2mzm -Wno-unused-result ff2mzm.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char d[4];
} Color;

static unsigned char buf[16];
static int width,height,cheight;
static unsigned char*font;
static unsigned char*row;
static unsigned char*pic;
static unsigned char*grid;
static Color pal[16];
static int opt[128];
static int opt2[128];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff2mzm <font> <palette> <?opt#?> <?opt#,#?> ...                        \n"
		"                                                                              \n"
		"Convert farbfeld to MegaZeux image format. ff2mzm reads a farbfeld image from \n"
		"stdin, converts it to MegaZeux image format and writes the result to stdout.  \n"
		"                                                                              \n"
		"MZM is a character-based graphics format used originally in MegaZeux. There   \n"
		"are two formats; \"layer-mode\" and \"board-mode\".                           \n"
		"                                                                              \n"
		"This program outputs a layer-mode MZM (MZM2 format only). The first two       \n"
		"arguments are the font file (which must support seeking) and the palette file;\n"
		"both are required, but in both cases you can give . instead if there is no    \n"
		"file.                                                                         \n"
		"                                                                              \n"
		"Specifying the colours as command-line arguments is not available.            \n"
		"                                                                              \n"
		"The rest of the arguments is each specifies a letter (case sensitive) followed\n"
		"by a number (with nothing in between), and optionally also a comma and another\n"
		"number.                                                                       \n"
		"                                                                              \n"
		"Valid options are:                                                            \n"
		"   c: Minimum free character code (if not specified, the font remains         \n"
		"      unchanged). The second number is the maximum free character code        \n"
		"      (default 255).                                                          \n"
		"   C: Minimum character code to use in output (default 0). The second number  \n"
		"      is the maximum character code to use in output (default 255).           \n"
		"   h: This setting is only used if no font is input (in which case this is    \n"
		"      mandatory). It can range from 1 to 16, giving the font height.          \n"
		"   i: If set to nonzero, it will avoid generating tiles where the high bit of \n"
		"      the background colour is set.                                           \n"
		"   o: If set to nonnegative, outputs a Hamster archive instead of the MZM file\n"
		"      by itself (this setting can be used if you want the changed fonts and   \n"
		"      palettes output as well as the MZM file). The number is used as the base\n"
		"      name for the lumps.                                                     \n"
		"   p: Minimum free colour code (palette remains unchanged if not specified)   \n"
		"      The second number is the maximum free colour code (default 15).         \n"
		"   P: Minimum colour code to use in output (default 0). The second number is  \n"
		"      the maximum colour code to use in output (default 15).                  \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff2mzm . . h16 < image.ff > image.mzm                                    \n"
		"   $ bunzip2 < image.ff.bz2 | ff2mzm pc.font my.palette > image.mzm           \n"
		"\n"
	);
	exit(1);
}

static void *
allocate(int n)
{
	void *a=malloc(n);
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
	if (len>0x1000 || (len&~0x7F00)) {
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
		fread(pal[i].d,1,3,fp);
		pal[i].d[3]=0x3F;
	}
	fclose(fp);
}

static void
load_picture(int m)
{
	int i,j;
	for (i=0; i<m; i++) {
		fread(buf,1,8,stdin);
		buf[0]=buf[0]>>2;
		buf[1]=buf[2]>>2;
		buf[2]=buf[4]>>2;
		for (j=opt['P']; j<=opt2['P'] && j<16; j++)
			if ((j<opt['p'] || j>opt2['p']) && !memcmp(pal[j].d,buf,3)) break;
		if (j>opt2['P']) {
			j=opt['p']++;
			if (j>opt2['p'] || j>15 || j>opt2['P']) {
				fprintf(stderr,"Invalid color in picture\n");
				exit(1);
			}
			memcpy(pal[j].d,buf,3);
		}
		pic[i]=j;
	}
}

static void
make_grid(void)
{
	int scan=width<<3;
	int m=width*height;
	int row=scan*(cheight-1)+8;
	unsigned char*p=pic;
	unsigned char*q=pic;
	unsigned char*g=grid;
	int b,f,i,j,n,x,y;
	for (i=0; i<m; i++) {
		q=p;
		b=f=-1;
		for (y=0; y<cheight; y++) {
			buf[y]=0;
			for (x=0; x<8; x++) {
				if (b==-1) {
					b=q[x];
				} else if (b!=q[x] && f==-1) {
					f=q[x];
				} else if (b!=q[x] && f!=q[x]) {
					fprintf(stderr,"Tile at position %d has more than two colors\n",i);
					exit(1);
				}
				buf[y]<<=1;
				if (f==q[x]) buf[y]|=1;
			}
			q+=scan;
		}
		if (f==-1) f=0;
		if (opt['i'] && (b&8)) {
			j=b; b=f; f=j;
			for (y=0; y<cheight; y++) buf[y]^=255;
		}
		for (j=opt['C']; j<=opt2['C'] && j<256; j++)
			if ((j<opt['c'] || j>opt2['c']) && !memcmp(font+cheight*j,buf,cheight)) goto found;
		if (!opt['i'] || !(f&8)) {
			j=b; b=f; f=j;
			for (y=0; y<cheight; y++) buf[y]^=255;
			for (j=opt['C']; j<=opt2['C'] && j<256; j++)
				if ((j<opt['c'] || j>opt2['c']) && !memcmp(font+cheight*j,buf,cheight)) goto found;
		}
		j=opt['c']++;
		if (j>opt2['c'] || j>255 || j>opt2['C']) {
			fprintf(stderr,"No character for tile at position %d\n",i);
			exit(1);
		}
		memcpy(font+cheight*j,buf,cheight);
		found:
		if (opt['i'] && (b&8)) {
			fprintf(stderr,"Invalid background color at tile position %d\n",i);
			exit(1);
		}
		*g++=j;
		*g++=(b<<4)|f;
		p+=(i+1)%width?8:row;
	}
}

static void
hamarc_head(const char *ext, int size)
{
	printf("%d%s",opt['o'],ext);
	putchar(0);
	putchar(size>>16);
	putchar(size>>24);
	putchar(size);
	putchar(size>>8);
}

int
main(int argc, char **argv)
{
	int i;
	char*s;
	if (argc<3 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	opt['o']=-1; // set nonnegative to output a Hamster archive
	opt['h']=0; // set to character height if no font is loaded
	opt['i']=0; // set to 1 to avoid use of background colours 8-15
	opt['C']=0; opt2['C']=255; // min/max character code to use
	opt['c']=256; opt2['c']=256; // min/max character code to overwrite
	opt['P']=0; opt2['P']=15; // min/max colour to use
	opt['p']=16; opt2['p']=16; // min/max colour to overwrite
	for (i=3; i<argc; i++) if (argv[i][0]) {
		opt[argv[i][0]&127]=strtol(argv[i]+1,&s,0);
		if (s && *s==',') opt2[argv[i][0]&127]=strtol(s+1,0,0);
	}
	fread(buf,1,16,stdin);
	if (memcmp(buf,"farbfeld",8)) {
		fprintf(stderr,"Not farbfeld\n");
		return 1;
	}
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	if (argv[1][0]!='.' || argv[1][1]) {
		load_font(argv[1]);
	} else {
		font=allocate((cheight=opt['h'])<<3);
		memset(font,0,cheight<<3);
	}
	if (argv[2][0]=='.' && !argv[2][1]) {
		for (i=0; i<16; i++) {
			pal[i].d[0]=(i&4?0x2A:0)|(i&8?0x15:0);
			pal[i].d[1]=(i&2?0x2A:0)|(i&8?0x15:0);
			pal[i].d[2]=(i&1?0x2A:0)|(i&8?0x15:0);
			pal[i].d[3]=0x3F;
		}
		pal[6].d[1]=0x15;
	} else {
		load_palette(argv[2]);
	}
	if (width<=0 || height<=0 || (width&7) || height%cheight) {
		fprintf(stderr,"Improper picture size\n");
		return 1;
	}
	pic=allocate(width*height);
	load_picture(width*height);
	width>>=3;
	height/=cheight;
	grid=allocate(width*height);
	make_grid();
	if (opt['o']>=0) {
		hamarc_head(".PAL",0x30);
		for (i=0; i<16; i++) fwrite(pal[i].d,1,3,stdout);
		hamarc_head(".CHR",cheight<<3);
		fwrite(font,8,cheight,stdout);
		hamarc_head(".MZM",width*height*2+16);
	}
	fwrite("MZM2",1,4,stdout);
	putchar(width); putchar(width>>8);
	putchar(height); putchar(height>>8);
	fwrite("\0\0\0\0\0\1\0\0",1,8,stdout);
	fwrite(grid,width<<1,height,stdout);
	return 0;
}
