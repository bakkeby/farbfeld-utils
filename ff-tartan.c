#if 0
gcc -s -O2 -o ~/bin/ff-tartan -Wno-unused-result ff-tartan.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned short d[4];
	unsigned short c;
} Color;

static char mix[64];
static char nmix;
static int width,height;
static Color col[64];
static char ncol;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-tartan <width> <height> <mixing list> <thread> ... <?thread?>       \n"
		"                                                                              \n"
		"This farbfeld program generates a tartan pattern.                             \n"
		"                                                                              \n"
		"This program does not read from standard in.                                  \n"
		"                                                                              \n"
		"Notations like \"K4 R24 K24 Y4\" are not implemented. A single code in this   \n"
		"pattern typically have multiple RGB colours associated with it which makes it \n"
		"problematic to implement.                                                     \n"
		"                                                                              \n"
		"The program takes four or more arguments.                                     \n"
		"                                                                              \n"
		"The command line arguments are:                                               \n"
		"   - width of the picture                                                     \n"
		"   - height of the picture                                                    \n"
		"   - mixing list                                                              \n"
		"   - the remaining arguments are the thread counts (up to 64)                 \n"
		"                                                                              \n"
		"The mixing list consists of digits 0 to 8 to define the mixing levels of the  \n"
		"warp/weft, here are some descriptions of each:                                \n"
		"   0 -- hard vertical lines (no horizontal mixing)                            \n"
		"   1 -- 25%% mixed horizontal lines                                           \n"
		"   2 -- 50%% mixed horizontal lines                                           \n"
		"   3 -- 75%% mixed horizontal lines                                           \n"
		"   4 -- evenly mixed colours                                                  \n"
		"   5 -- 75%% mixed vertical lines                                             \n"
		"   6 -- 50%% mixed vertical lines                                             \n"
		"   7 -- 25%% mixed vertical lines                                             \n"
		"   8 -- hard horizontal lines (no vertical mixing)                            \n"
		"                                                                              \n"
		"The mixing list is a list of digits with no separators, e.g. 4014 can be used.\n"
		"                                                                              \n"
		"Each thread count argument is given the number of pixels and then \"x\" and   \n"
		"then the colour of the thread.                                                \n"
		"                                                                              \n"
		"The colours can be defined through one of the following patterns:             \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrggbbaa          four pairs of 8-bit hex values                           \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"   rrrrggggbbbbaaaa  four pairs of 16-bit hex values                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-tartan 640 480 4014 50x906060 100x609060 50x606090 30xFF2250 > tt.ff  \n"
		"\n"
	);
	exit(1);
}

static void parse_color(unsigned short*o,const char*s) {
	switch(strlen(s)) {
		case 6:
			sscanf(s,"%2hX%2hX%2hX",o+0,o+1,o+2);
			o[0]*=0x101;
			o[1]*=0x101;
			o[2]*=0x101;
			o[3]=0xFFFF;
			break;
		case 8:
			sscanf(s,"%2hX%2hX%2hX%2hX",o+0,o+1,o+2,o+3);
			o[0]*=0x101;
			o[1]*=0x101;
			o[2]*=0x101;
			o[3]*=0x101;
			break;
		case 12:
			sscanf(s,"%4hX%4hX%4hX",o+0,o+1,o+2);
			o[3]=0xFFFF;
			break;
		case 16:
			sscanf(s,"%4hX%4hX%4hX%4hX",o+0,o+1,o+2,o+3);
			break;
		default:
			fprintf(stderr,"Improper color format: %s\n",s);
			exit(1);
	}
}

static inline void do_channel(int c,int x,int y,int m) {
	m=((8-m)*col[x].d[c]+m*col[y].d[c])>>3;
	putchar(m>>8); putchar(m);
}

int main(int argc,char**argv) {
	int i,x,xp,xq,y,yp,yq;
	char*s;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<5 || argc>68) {
		fprintf(stderr,"Too %s arguments\n",argc<5?"few":"many");
		return 1;
	}
	width=strtol(argv[1],0,10);
	height=strtol(argv[2],0,10);
	if(!argv[3][0] || strlen(argv[3])>64) {
		fprintf(stderr,"Invalid mix list\n");
		return 1;
	}
	nmix=strlen(argv[3]);
	for(i=0;i<nmix;i++) mix[i]=argv[3][i]-'0';
	ncol=argc-4;
	for(i=0;i<ncol;i++) {
		col[i].c=strtol(argv[i+4],&s,10);
		if(!col[i].c || *s!='x') {
			fprintf(stderr,"Syntax error: %s\n",argv[i+4]);
			return 1;
		}
		parse_color(col[i].d,s+1);
	}
	fwrite("farbfeld",1,8,stdout);
	putchar(width>>24); putchar(width>>16); putchar(width>>8); putchar(width);
	putchar(height>>24); putchar(height>>16); putchar(height>>8); putchar(height);
	for(y=yp=yq=0;y<height;y++) {
		for(x=xp=xq=0;x<width;x++) {
			do_channel(0,xp,yp,mix[(x+y)%nmix]);
			do_channel(1,xp,yp,mix[(x+y)%nmix]);
			do_channel(2,xp,yp,mix[(x+y)%nmix]);
			do_channel(3,xp,yp,mix[(x+y)%nmix]);
			if(xq++==col[xp].c) xp=(xp+1)%ncol,xq=0;
		}
		if(yq++==col[yp].c) yp=(yp+1)%ncol,yq=0;
	}
	return 0;
}
