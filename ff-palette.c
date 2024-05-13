#if 0
gcc -s -O2 -o ~/bin/ff-palette -Wno-unused-result ff-palette.c
exit
#endif

// Currently this program is not very good but later we can make modes that are improved.
// Please notify me if you have a better algorithm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(...) do { fprintf(stderr,__VA_ARGS__); putchar('\n'); exit(1); } while(0)

typedef struct {
	unsigned short c[3];
} Color;

typedef void(*QuantFunc)(void);

static char mode;
static int qarg[16];
static int fixcount,totalcount;
static int picsize;
static Color palette[1024];
static Color curpix;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-palette <mode> <palette size> <?colour?> <?colour?>                 \n"
		"                                                                              \n"
		"This farbfeld program is used to generate a suitable palette for a picture.   \n"
		"                                                                              \n"
		"The output of this program is a textual representation of an RGB palette.     \n"
		"                                                                              \n"
		"The alpha channel is not included in the output.                              \n"
		"                                                                              \n"
		"The filter takes at least two arguments:                                      \n"
		"   mode - the method used to generate the palette, followed by comma separated\n"
		"          list of options (with a leading comma)                              \n"
		"   size - the number of colours to produce for the palette                    \n"
		"   colour - zero or more colours to manually add to the palette, referred to  \n"
		"            as palette fixes - manually specified colours will be included in \n"
		"            addition to the number of colours that the program is asked to    \n"
		"            produce                                                           \n"
		"                                                                              \n"
		"The available modes are:                                                      \n"
		"   p -- generates the palette                                                 \n"
		"                                                                              \n"
		"Additionally the p mode must be followed by a comma and up to five values:    \n"
		"   box value 1 - The box values must be integers and greater than zero,       \n"
		"   box value 2   otherwise a box size out of range error will occur. These    \n"
		"   box value 3   seem to controls the pixels that are to be read to work out  \n"
		"                 the palette.                                                 \n"
		"   deduction - unclear what this is for; it is a fixed value that is deducted \n"
		"               from the value of every palette fix, but doesn't seem to affect\n"
		"               the output                                                     \n"
		"   repeat    - how many times the algorithm runs to work out the best colours \n"
		"                                                                              \n"
		"The colours can be defined using any of the following patterns:               \n"
		"   hh                single pair of 8-bit hex values; the same value is used  \n"
		"                     for all channels                                         \n"
		"   hhhh              single pair of 16-bit hex values; the same value is used \n"
		"                     for all channels                                         \n"
		"   rrggbb            three pairs of 8-bit hex values                          \n"
		"   rrrrggggbbbb      three pairs of 16-bit hex values                         \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-palette p,9,3,3,9,2 24 906060 < image.ff                              \n"
		"\n"
	);
	exit(1);
}

static void parse_color(const char*s,unsigned short*c) {
	switch(strlen(s)) {
		case 2:
			sscanf(s,"%2hx",c);
			*c*=0x101;
			c[1]=c[2]=*c;
			break;
		case 4:
			sscanf(s,"%4hx",c);
			c[1]=c[2]=*c;
			break;
		case 6:
			sscanf(s,"%2hx%2hx%2hx",c,c+1,c+2);
			c[0]*=0x101;
			c[1]*=0x101;
			c[2]*=0x101;
			break;
		case 12:
			sscanf(s,"%4hx%4hx%4hx",c,c+1,c+2);
			break;
		default:
			fatal("Invalid color format (%d)",(int)strlen(s));
	}
}

static int readpixel(void) {
	static unsigned char buf[8];
	for(;;) {
		if(!fread(buf,1,8,stdin)) return 0;
		if(!(buf[6]&128)) {
			picsize--;
			continue;
		}
		curpix.c[0]=(buf[0]<<8)|buf[1];
		curpix.c[1]=(buf[2]<<8)|buf[3];
		curpix.c[2]=(buf[4]<<8)|buf[5];
		return 1;
	}
}

static void func_p(void) {
	typedef struct {
		long long s[3];
		int n;
		int v;
	} Node;
	int i,j,k;
	int ts=qarg[0]*qarg[1]*qarg[2];
	Node*list=calloc(sizeof(Node),ts);
	Node*t;
	int nodecmp(const void*a,const void*b) {
		const Node*x=a;
		const Node*y=b;
		return x->n&&y->n?(x->v>=y->v?-1:1):x->n?-1:1;
	}
	if(ts<=0 || qarg[0]<0 || qarg[1]<0 || qarg[2]<0) fatal("Box size out of range");
	if(!list) fatal("Allocation failed");
	for(i=0;i<fixcount;i++) {
		curpix=palette[i];
		t=list+((qarg[0]*curpix.c[0])>>16)+qarg[0]*((qarg[1]*curpix.c[1])>>16)+qarg[0]*qarg[1]*((qarg[2]*curpix.c[2])>>16);
		t->v-=qarg[3];
	}
	while(readpixel()) {
		t=list+((qarg[0]*curpix.c[0])>>16)+qarg[0]*((qarg[1]*curpix.c[1])>>16)+qarg[0]*qarg[1]*((qarg[2]*curpix.c[2])>>16);
		t->n++;
		t->v++;
		t->s[0]+=curpix.c[0];
		t->s[1]+=curpix.c[1];
		t->s[2]+=curpix.c[2];
	}
	qsort(list,ts,sizeof(Node),nodecmp);
	again:
	for(j=0;j<totalcount-fixcount && j<ts;j++) {
		t=list+j;
		if(!t->n) break;
		for(i=0;i<fixcount;i++) {
			k=(abs(palette[i].c[0]-t->s[0]/t->n)+abs(palette[i].c[1]-t->s[1]/t->n)+abs(palette[i].c[2]-t->s[2]/t->n))>>7;
			if(!k) {
				t->n=0;
				goto skip1;
			}
			if(k<qarg[4]) {
				t->v-=qarg[4]-k;
				t->s[0]-=palette[i].c[0]*(qarg[4]-k);
				t->s[1]-=palette[i].c[1]*(qarg[4]-k);
				t->s[2]-=palette[i].c[2]*(qarg[4]-k);
				t->n-=qarg[4]-k;
				if(t->n<=0) {
					t->n=0;
					goto skip1;
				}
			}
		}
		for(i=0;i<j;i++) {
			k=(abs((list[i].s[0]-t->s[0])/t->n)+abs((list[i].s[1]-t->s[1])/t->n)+abs((list[i].s[2]-t->s[2])/t->n))>>7;
			if(k<qarg[4]) list[i].v-=qarg[4]-k;
		}
		skip1: ;
	}
	qsort(list,ts,sizeof(Node),nodecmp);
	if(qarg[4]>>=1) goto again;
	for(i=k=0;i<ts && list[i].n && k<totalcount-fixcount;i++) k++;
	if(!k) return;
	for(i=0;i<k;i++) for(j=0;j<3;j++) list[i].s[j]/=list[i].n,palette[i+fixcount].c[j]=list[i].s[j]<0?0:list[i].s[j]>65535?65535:list[i].s[j];
}

static const QuantFunc funcs[128]={
	['p']=func_p,
};

static void read_header(void) {
	unsigned char buf[16];
	fread(buf,1,16,stdin);
	picsize=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	picsize*=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
}

int main(int argc,char**argv) {
	int i;
	char*s;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc<3) fatal("Too few arguments");
	mode=argv[1][0]&127;
	if(!funcs[mode]) fatal("Invalid mode");
	fixcount=argc-3;
	totalcount=fixcount+strtol(argv[2],0,0);
	if(totalcount>1024) fatal("Too many arguments");
	if(totalcount<fixcount) fatal("Invalid palette size");
	s=argv[1]+1; i=0; while(i<16 && *s==',') qarg[i++]=strtol(s+1,&s,0);
	read_header();
	for(i=3;i<argc;i++) parse_color(argv[i],palette[i-3].c);
	funcs[mode]();
	for(i=0;i<totalcount;i++) printf("%04X%04X%04X\n",palette[i].c[0],palette[i].c[1],palette[i].c[2]);
	return 0;
}

