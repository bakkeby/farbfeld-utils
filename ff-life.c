#if 0
gcc -s -O2 -o ~/bin/ff-life -Wno-unused-result ff-life.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,height,size,steps;
static int rule[2];
static signed char*pic;
static signed char*pic2;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-life <?rule?> <?steps?>                                             \n"
		"                                                                              \n"
		"This farbfeld program generates life-like cellular automata.                  \n"
		"                                                                              \n"
		"The program takes up to two arguments:                                        \n"
		"   - rule to use, in BS notation (default is B3/S23 which is Game of Life)    \n"
		"   - the number of steps (default 1)                                          \n"
		"                                                                              \n"
		"Black cells are off, white cells are on, and transparent cells are permanently\n"
		"off.                                                                          \n"
		"                                                                              \n"
		"See the man page for a practical example.                                     \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-life B678/S345678 20 < data.ff > life.ff                              \n"
		"\n"
	);
	exit(1);
}

static inline void read_picture(void) {
	int i;
	for(i=0;i<size;i++) {
		fread(buf,1,8,stdin);
		if(buf[6]) {
			pic[i]=(buf[0]?4:0)|(buf[2]?2:0)|(buf[4]?1:0);
		} else {
			pic[i]=-1;
		}
	}
}

#define ADJ(xd,yd) if((j=pic[(x+xd+width)%width+((y+yd+height)%height)*width])>0) n++,c^=j,d|=j;
static inline void do_steps(void) {
	int i,j,n,c,d,x,y;
	for(y=i=0;y<height;y++) for(x=0;x<width;x++,i++) {
		if(pic[i]<0) {
			pic2[i]=-1;
		} else {
			n=0;
			c=d=pic[i];
			ADJ(-1,-1); ADJ(0,-1);  ADJ(1,-1);
			ADJ(-1,0);              ADJ(1,0);
			ADJ(-1,1);  ADJ(0,1);   ADJ(1,1);
			pic2[i]=rule[pic[i]?0:1]&(1<<n)?c&7?:(d?:7):0;
		}
	}
	memcpy(pic,pic2,size);
}

static inline void write_picture(void) {
	int i;
	for(i=0;i<size;i++) {
		if(pic[i]<0) {
			buf[0]=buf[1]=buf[2]=buf[3]=buf[4]=buf[5]=buf[6]=buf[7]=0;
		} else {
			buf[0]=buf[1]=pic[i]&4?255:0;
			buf[2]=buf[3]=pic[i]&2?255:0;
			buf[4]=buf[5]=pic[i]&1?255:0;
			buf[6]=buf[7]=255;
		}
		fwrite(buf,1,8,stdout);
	}
}

int main(int argc,char**argv) {
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	steps=argc>2?strtol(argv[2],0,0):1;
	if(argc>1 && argv[1][0]) {
		int i;
		int c=0;
		for(i=0;argv[1][i];i++) switch(argv[1][i]) {
			case 'L': goto life;
			case 'B': case '/': c=1; break;
			case 'S': c=0; break;
			case '0' ... '8': rule[c]|=1<<(argv[1][i]-'0');
		}
	} else {
		life:
		rule[1]=0010;
		rule[0]=0014;
	}
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	size=width*height;
	pic=malloc(size);
	pic2=malloc(size);
	if(!pic || !pic2) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	read_picture();
	while(steps--) do_steps();
	write_picture();
	return 0;
}
