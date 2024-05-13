#if 0
gcc -s -O2 -o ~/bin/ff-critter -Wno-unused-result ff-critter.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do{ fprintf(stderr,__VA_ARGS__); exit(1); }while(0)

typedef struct {
	unsigned char a[8];
} Pixel;

static unsigned char buf[16];
static int width,height,steps,nrule;
static Pixel*pic;
static const char*rule;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-critter <rule string> <steps>                                       \n"
		"                                                                              \n"
		"This program implements the Critter, Tron and other rules of Margolus block   \n"
		"cellular automata.                                                            \n"
		"                                                                              \n"
		"The filter requires two arguments:                                            \n"
		"   - the rule string                                                          \n"
		"   - number of steps                                                          \n"
		"                                                                              \n"
		"The first argument is the rule string, which is a string of digits 0 to 7,    \n"
		"which can be any length at least one. Which rule is applicable depend on the  \n"
		"pixel value, where the first one is for black, last one for white, and others \n"
		"for in between.                                                               \n"
		"                                                                              \n"
		"The second argument is how many steps to process.                             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-critter 201471 4 < image.ff > image-critter.ff                        \n"
		"   $ ff-critter 7 252 < image.ff > image-critter.ff                           \n"
		"   $ ff-critter 606 15 < image.ff > image-critter.ff                           \n"
		"\n"
	);
	exit(1);
}

static void process(int s) {
	int i,j,x,y;
	unsigned long long z;
	Pixel*p[4];
	Pixel q[4];
	for(y=s;y<height;y+=2) for(x=s;x<width;x+=2) {
		p[0]=pic+y*width+x;
		p[1]=pic+y*width+(x==width-1?0:x+1);
		p[2]=pic+(y==height-1?0:y+1)*width+x;
		p[3]=pic+(y==height-1?0:y+1)*width+(x==width-1?0:x+1);
		z=(p[0]->a[0]+p[1]->a[2]+p[2]->a[4]+p[3]->a[6])<<8;
		z|=p[0]->a[1]+p[1]->a[3]+p[2]->a[5]+p[3]->a[7];
		z*=nrule-1LL;
		i=rule[z/(0xFFFF*4LL)];
		if(i&1) for(j=0;j<8;j++) p[0]->a[j]^=255,p[1]->a[j]^=255,p[2]->a[j]^=255,p[3]->a[j]^=255;
		if(i&2) {
			q[0]=*p[0],q[1]=*p[1],q[2]=*p[2],q[3]=*p[3];
			*p[0]=q[3],*p[1]=q[2],*p[2]=q[1],*p[3]=q[0];
		}
		if(i&4) p[0]->a[6]^=255,p[0]->a[7]^=255;
	}
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc!=3) fatal("Incorrect number of arguments\n");
	nrule=strlen(rule=argv[1]);
	if(!nrule) fatal("Invalid rule\n");
	steps=strtol(argv[2],0,0);
	if(steps<0) fatal("Incorrect number of steps\n");
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	pic=malloc(width*height*8);
	if(!pic) fatal("Allocation failed\n");
	fread(pic,width,height<<3,stdin);
	while(steps--) process(steps&1);
	fwrite(buf,1,16,stdout);
	fwrite(pic,width,height<<3,stdout);
	return 0;
}
