#if 0
gcc -s -O2 -o ~/bin/ff-text -Wno-unused-result ff-text.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static unsigned char*font;
static int cwidth,cheight,iwidth,iheight,owidth,oheight,ccount,icount,ocount,ismap;
static short map[256];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-text <?+encoding?> <tile width> <tile height> <text> <?text?> ...   \n"
		"                                                                              \n"
		"This farbfeld program is used to render fix-pitch text.                       \n"
		"                                                                              \n"
		"The input is the font picture.                                                \n"
		"                                                                              \n"
		"Fix-pitch text referes to a style of typography where each character occupies \n"
		"the same amount of horizontal space regardless of the character's width - more\n"
		"commonly known as monospaced or fixed-width text.                             \n"
		"                                                                              \n"
		"In this context the \"font\" is an image where the letters are drawn in       \n"
		"designated spaces and their placement is determined by the given character    \n"
		"height and width. The \"rendering\" of these characters is merely a matter of \n"
		"tiling the individual character tiles based on the given input text.          \n"
		"                                                                              \n"
		"The filter takes the following arguments:                                     \n"
		"   - optionally, the font encoding with a plus sign at first and then the     \n"
		"     characters of the font (all as a single argument) - if not specified,    \n"
		"     byte values 0 to 255 are represented in order in the font (although a    \n"
		"     null byte cannot actually be used as a character)                        \n"
		"   - the character tile width                                                 \n"
		"   - the character tile height                                                \n"
		"   - one or more lines of text (one line per argument) - encoding is always   \n"
		"     bytes, rather than UTF-8 or other encodings                              \n"
		"                                                                              \n"
		"The font is encoded as a tile set with any integer number of tiles per row,   \n"
		"with no padding.                                                              \n"
		"                                                                              \n"
		"If some lines of text are longer than others, the remaining lines are left    \n"
		"aligned and are padded on the right with transparent pixels. Any characters   \n"
		"that are not in the specified encoding are also transparent.                  \n"
		"                                                                              \n"
		"Note that the input font picture must have a width and height that fits       \n"
		"exactly with the modulo of the given tile width and height. If there are      \n"
		"leftover pixels then the program will complain.                               \n"
		"                                                                              \n"
		"Tip: it would be possible to generate such a fix-pitch font picture based on  \n"
		"     an actual font by using ff-xdraw.                                        \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-text \"+ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz- .:;\" \\\n"
		"             18 16 \"Hello world\" < fix-pitch-font.ff > image-text.ff        \n"
		"\n"
	);
	exit(1);
}

static void process(const char*s) {
	int n,h,c;
	const char*p;
	for(h=0;h<cheight;h++) {
		p=s;
		c=ocount;
		while(*p) {
			c--;
			n=*p++&255;
			if(ismap) {
				n=map[n];
				if(n==-1) {
					n=cwidth;
					while(n--) putchar(0);
					continue;
				}
			}
			fwrite(font+(n%icount)*cwidth+(h+(n/icount)*cheight)*iwidth,1,cwidth,stdout);
		}
		c*=cwidth;
		while(c--) putchar(0);
	}
}

int main(int argc,char**argv) {
	int i;
	if (argc<3 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	if(argv[1][0]=='+') {
		argv++;
		ismap=1;
		for(i=0;i<256;i++) map[i]=-1;
		for(i=1;argv[0][i];i++) map[argv[0][i]&255]=i-1;
		if(--argc<3) {
			fprintf(stderr,"Too few arguments\n");
			return 0;
		}
	}
	fread(buf,1,16,stdin);
	iwidth=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	iheight=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	font=malloc(iwidth*iheight*8);
	if(!font) {
		fprintf(stderr,"Allocation failed\n");
		return 0;
	}
	fread(font,8,iwidth*iheight,stdin);
	cwidth=strtol(argv[1],0,0);
	cheight=strtol(argv[2],0,0);
	if(!cwidth || !cheight || iwidth%cwidth || iheight%cheight) {
		fprintf(stderr,"Improper tile size\n");
		return 0;
	}
	ocount=0;
	oheight=(argc-3)*cheight;
	for(i=3;i<argc;i++) if(ocount<strlen(argv[i])) ocount=strlen(argv[i]);
	owidth=ocount*cwidth;
	buf[8]=owidth>>24;
	buf[9]=owidth>>16;
	buf[10]=owidth>>8;
	buf[11]=owidth>>0;
	buf[12]=oheight>>24;
	buf[13]=oheight>>16;
	buf[14]=oheight>>8;
	buf[15]=oheight>>0;
	fwrite(buf,1,16,stdout);
	icount=iwidth/cwidth;
	iwidth<<=3;
	cwidth<<=3;
	for(i=3;i<argc;i++) process(argv[i]);
	return 0;
}
