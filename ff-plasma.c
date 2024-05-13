#if 0
gcc -s -O2 -o ~/bin/ff-plasma -Wno-unused-result ff-plasma.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned short c[4];
} Pixel;

static unsigned char buf[16];
static int width,height;
static Pixel*pic;
static double ratio=1.0;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-plasma <seed> <depth> <?depth per pass?> <?opacity?>                \n"
		"                                                                              \n"
		"This farbfeld filter is used to apply a randomized plasma effect to a picture,\n"
		"with varying colours.                                                         \n"
		"                                                                              \n"
		"The filter takes two, three or four arguments:                                \n"
		"   - random number seed                                                       \n"
		"   - total depth, can be 0 to calculate it based on the picture size          \n"
		"   - amount of extra depth per pass; the default value is 1 and the minimum   \n"
		"     value is 0                                                               \n"
		"   - opacity (floating point number in the range 0 to 1.0); the default value \n"
		"     is 1.0 which means fully opaque                                          \n"
		"                                                                              \n"
		"The alpha channel of the input contains the probability that a pixel will be  \n"
		"changed. The alpha channel is left unchanged on output.                       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-plasma < image.ff > image-plasma.ff                                   \n"
		"   $ ff-plasma 23 0 1 0.7 < image.ff > image-plasma.ff                        \n"
		"\n"
	);
	exit(1);
}

static inline unsigned short*getpixel(int x,int y) {
	return pic[(x<0?0:x>=width?width-1:x)+width*(y<0?0:y>=height?height-1:y)].c;
}

static inline void make_pixel(unsigned short*u,unsigned short*v,unsigned short*w,double fac) {
	double r;
	int x,i;
	if(w[3]!=65535 && (random()&65535)>w[3]) return;
	for(i=0;i<3;i++) {
		r=random()/(double)RAND_MAX;
		x=(u[i]+v[i])*0.5+fac*(r-0.5);
		x=ratio*x+(1.0-ratio)*w[i];
		w[i]=x<0?0:x>65535?65535:x;
	}
}

static void recursive(int x1,int y1,int x2,int y2,double fac,int depth) {
	int xm=(x1+x2)>>1;
	int ym=(y1+y2)>>1;
	if(depth) {
		recursive(x1,y1,xm,ym,fac,depth-1);
		recursive(x1,ym,xm,y2,fac,depth-1);
		recursive(xm,y1,x2,ym,fac,depth-1);
		recursive(xm,ym,x2,y2,fac,depth-1);
		return;
	}
	if(x1!=xm || y1!=ym) {
		make_pixel(getpixel(x1,y1),getpixel(x1,y2),getpixel(x1,ym),fac);
		if(x1!=x2) make_pixel(getpixel(x2,y1),getpixel(x2,y2),getpixel(x2,ym),fac);
	}
	if(y1!=ym || y2!=ym) {
		if(x1!=xm || y1!=ym) make_pixel(getpixel(x1,y2),getpixel(x2,y2),getpixel(xm,y2),fac);
		if(y1!=y2) make_pixel(getpixel(x1,y1),getpixel(x2,y1),getpixel(xm,y1),fac);
	}
	if(x1!=x2 || y1!=y2) make_pixel(getpixel(x1,y1),getpixel(x2,y2),getpixel(xm,ym),fac);
}

static void do_plasma(int depth,int start) {
	int i,d;
	if(!depth) {
		i=(width>height?width:height)>>1;
		while(i) depth++,i>>=1;
	}
	for(d=0;d<depth;d++) recursive(0,0,width-1,height-1,32767.5/(d+1),d+start);
}

int main(int argc,char**argv) {
	int i;
	if (argc<3 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	if(argc>4) ratio=strtod(argv[4],0);
	srandom(strtol(argv[1],0,0));
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	pic=malloc(width*height*sizeof(Pixel));
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	i=width*height;
	while(i--) {
		fread(buf,1,8,stdin);
		pic[i].c[0]=(buf[0]<<8)|buf[1];
		pic[i].c[1]=(buf[2]<<8)|buf[3];
		pic[i].c[2]=(buf[4]<<8)|buf[5];
		pic[i].c[3]=(buf[6]<<8)|buf[7];
	}
	do_plasma(strtol(argv[2],0,0),argc>3?strtol(argv[3],0,0):1);
	i=width*height;
	while(i--) {
		buf[0]=pic[i].c[0]>>8; buf[1]=pic[i].c[0];
		buf[2]=pic[i].c[1]>>8; buf[3]=pic[i].c[1];
		buf[4]=pic[i].c[2]>>8; buf[5]=pic[i].c[2];
		buf[6]=pic[i].c[3]>>8; buf[7]=pic[i].c[3];
		fwrite(buf,1,8,stdout);
	}
	return 0;
}

