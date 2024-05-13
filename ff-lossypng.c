#if 0
gcc -s -O2 -o ~/bin/ff-lossypng -Wno-unused-result ff-lossypng.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char d[4];
} Pixel;

typedef struct {
	int d[3*4];
} ColorError;

static int width,height;
static unsigned char buf[16];
static int opt[128];
static Pixel*row;
static Pixel prev,cur;
static ColorError*colorError;
static int diffusion[4];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-lossypng <?opt=value?> <?opt=value?> ...                            \n"
		"                                                                              \n"
		"This farbfeld program reduces the quality of an image in an attempt to allow  \n"
		"for smaller PNG files by improved compression.                                \n"
		"                                                                              \n"
		"Note that this program does not perform PNG encoding, it just handles the     \n"
		"quality reduction.                                                            \n"
		"                                                                              \n"
		"When encoding the image as PNG you should add the q3 option to ff2png for     \n"
		"improved compression.                                                         \n"
		"                                                                              \n"
		"Options are a letter and equal sign and a number; no options are required.    \n"
		"                                                                              \n"
		"The available options are:                                                    \n"
		"   a -- if specified and nonzero, tries to bring pixels closer to the average;\n"
		"        higher numbers                                                        \n"
		"   d -- a parameter used for rounding the amount of diffusion, which is       \n"
		"        normally 16                                                           \n"
		"   h -- a parameter used for rounding - if zero or unspecified, then it will  \n"
		"        use half of q by default                                              \n"
		"   q -- quality setting - higher numbers result in worse quality but improved \n"
		"        compression (the default setting is 6)                                \n"
		"                                                                              \n"
		"Additionally, regardless of the options selected, the picture is reduced to   \n"
		"8-bits per channel.                                                           \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-lossypng < image.ff | ff2png q3 > image-lossy.png                     \n"
		"\n"
	);
	exit(1);
}

static void diffuseColorDeltas(int x) {
	int i;
	for(i=0;i<4;i++) {
		diffusion[i]=2*colorError[x-1].d[i+8];
		diffusion[i]+=3*colorError[x].d[i+8];
		diffusion[i]+=2*colorError[x+1].d[i+8];
		diffusion[i]+=2*colorError[x-2].d[i+4];
		diffusion[i]+=4*colorError[x-1].d[i+4];
		diffusion[i]+=5*colorError[x].d[i+4];
		diffusion[i]+=4*colorError[x+1].d[i+4];
		diffusion[i]+=2*colorError[x+2].d[i+4];
		diffusion[i]+=3*colorError[x-2].d[i];
		diffusion[i]+=5*colorError[x-1].d[i];
		diffusion[i]+=(diffusion[i]<0?-opt['d']:opt['d']);
		diffusion[i]/=32;
	}
}

static void process(void) {
	int x,y,c,v,a;
	for(y=0;y<height;y++) {
		prev.d[0]=prev.d[1]=prev.d[2]=prev.d[3]=0;
		for(x=0;x<width;x++) {
			fread(buf,1,8,stdin);
			cur.d[0]=buf[0];
			cur.d[1]=buf[2];
			cur.d[2]=buf[4];
			cur.d[3]=buf[6];
			diffuseColorDeltas(x+4);
			for(c=0;c<4;c++) {
				v=cur.d[c];
				if(v>0 && v<255) {
					a=(row[x].d[c]+prev.d[c])/2;
					v+=diffusion[c]+opt['h']-a;
					v=(v-(v%opt['q']))+a;
					if(opt['a'] && x && y) {
						if(v>a+opt['a']) v=a+opt['a'];
						else if(v<a-opt['a']) v=a-opt['a'];
					}
					if(v<0 || v>255 || v-cur.d[c]>opt['m'] || cur.d[c]-v>opt['m']) v=cur.d[c];
					colorError[x+2].d[c]=cur.d[c]-v;
				} else {
					colorError[x+2].d[c]=0;
				}
				cur.d[c]=v;
				putchar(v);
				putchar(v);
			}
			row[x]=prev=cur;
		}
		for(x=0;x<width+4;x++) {
			for(c=7;c>=0;c--) colorError[x].d[c+4]=colorError[x].d[c];
		}
	}
}

int main(int argc,char**argv) {
	int i;
	opt['d']=16;
	opt['m']=512;
	opt['q']=6;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	for(i=1;i<argc;i++) {
		if(argv[i][0] && argv[i][1]=='=') opt[argv[i][0]&127]=strtol(argv[i]+2,0,0);
	}
	if(opt['q']<1) opt['q']=1;
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	row=calloc(width,4);
	colorError=calloc(width+4,sizeof(ColorError));
	if(!row || !colorError) goto nomem;
	fwrite(buf,1,16,stdout);
	if(!opt['h']) opt['h']=opt['q']>>1;
	process();
	return 0;
	nomem:
	fprintf(stderr,"Allocation failed\n");
	return 1;
}

// This program is based on:
//   https://github.com/foobaz/lossypng/blob/master/lossypng/lib.go
// The original program and this program are both public domain.
