#if 0
gcc -s -O2 -o ~/bin/ff-cloud -Wno-unused-result ff-cloud.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned short c[4];
} Pixel;
typedef double(*SmoothFunc)(double x,double y,int c);

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-cloud <?zoom levels?> <?smoothing type?> <?exponent?> <?options?>   \n"
		"                                                                              \n"
		"This farbfeld filter applies turbulence to random data to make clouds.        \n"
		"                                                                              \n"
		"Requires a picture containing random data as input which is used to generate  \n"
		"turbulence in order to create clouds. Optionally various filters can be       \n"
		"applied to preprocess the random data for special effects, if desired.        \n"
		"                                                                              \n"
		"The input can be grey scale or colours. Normally it will cycle through the    \n"
		"channels for each zoom level. The output is always grey scale unless the c    \n"
		"option is specified.                                                          \n"
		"                                                                              \n"
		"The filter takes the following optional arguments:                            \n"
		"   zoom levels -- number of zoom levels, positive integer (defaults to 1)     \n"
		"   smoothing function type -- from 0 to 6, outlined below (defaults to 2)     \n"
		"   exponent -- a floating number (defaults to 1.0)                            \n"
		"   options -- one letter each (no default value)                              \n"
		"                                                                              \n"
		"The possible smoothing function types are:                                    \n"
		"   0 -- no smoothing; just truncate the coordinates to an integer, resulting  \n"
		"        blocky output                                                         \n"
		"   1 -- linear interpolation                                                  \n"
		"   2 -- cubic interpolation                                                   \n"
		"   3 -- similar to 2 but using different factors per zoom level               \n"
		"   4 -- linear interpolation, but ignores zoom level and always uses all      \n"
		"        channels                                                              \n"
		"   5 -- uses positive cubic for red input, negative cubic for green input, and\n"
		"        positive linear for blue input.                                       \n"
		"   6 -- cubic interpolation, but ignores zoom level and always uses all       \n"
		"        channels (except alpha)                                               \n"
		"                                                                              \n"
		"Options:                                                                      \n"
		"   c -- use colours output. Positive samples are red, negative samples are    \n"
		"        green, and far negative samples are cyan                              \n"
		"   i -- some smoothing function types will use this to shift the zoom level   \n"
		"        numbers for each pixel, resulting in a striped output                 \n"
		"   j -- lock the channel                                                      \n"
		"   r -- adjust the zooming levels according to the alpha channel              \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-cloud < random.ff > image-cloud.ff                                    \n"
		"   $ ff-cloud 3 4 0.92 ci < random.ff > image-cloud.ff                        \n"
		"\n"
	);
	exit(1);
}

static unsigned char buf[16];
static int width,height,smoothfunc,zoom;
static double exponent;
static Pixel*pic;
static SmoothFunc fsmooth;
static char option[128];

static inline unsigned short*getpixel(int x,int y) {
	return pic[((x+width)%width)+width*((y+height)%height)].c;
}

static double smooth0(double x,double y,int c) {
	return getpixel(x,y)[c];
}

static double smooth1(double x,double y,int c) {
	int X=floor(x);
	int Y=floor(y);
	double Xf=x-X;
	double Yf=y-Y;
	return Xf*Yf*getpixel(X,Y)[c]+(1.0-Xf)*Yf*getpixel(X-1,Y)[c]+Xf*(1.0-Yf)*getpixel(X,Y-1)[c]+(1.0-Xf)*(1.0-Yf)*getpixel(X-1,Y-1)[c];
}

static inline double cubic(double a,double b,double c,double d,double x) {
	return b+0.5*x*(c-a+x*(2.0*a-5.0*b+4.0*c-d+x*(3.0*(b-c)+d-a)));
}

static double smooth2(double x,double y,int c) {
	int X=floor(x);
	int Y=floor(y);
	double Xf=x-X;
	double Yf=y-Y;
	return cubic(
		cubic(getpixel(X-1,Y-1)[c],getpixel(X-1,Y)[c],getpixel(X-1,Y+1)[c],getpixel(X-1,Y+2)[c],Yf),
		cubic(getpixel(X,Y-1)[c],getpixel(X,Y)[c],getpixel(X,Y+1)[c],getpixel(X,Y+2)[c],Yf),
		cubic(getpixel(X+1,Y-1)[c],getpixel(X+1,Y)[c],getpixel(X+1,Y+1)[c],getpixel(X+1,Y+2)[c],Yf),
		cubic(getpixel(X+2,Y-1)[c],getpixel(X+2,Y)[c],getpixel(X+2,Y+1)[c],getpixel(X+2,Y+2)[c],Yf),
	Xf);
}

static double smooth3(double x,double y,int c) {
	return smooth2(x,y,c)*(c&1?-1.0:c&2?1.5:1.0);
}

static double smooth4(double x,double y,int c) {
	int X=floor(x);
	int Y=floor(y);
	double Xf=x-X;
	double Yf=y-Y;
	return Xf*Yf*getpixel(X,Y)[0]+(1.0-Xf)*Yf*getpixel(X-1,Y)[1]+Xf*(1.0-Yf)*getpixel(X,Y-1)[2]+(1.0-Xf)*(1.0-Yf)*getpixel(X-1,Y-1)[3];
}

static double smooth5(double x,double y,int c) {
	return smooth2(x,y,0)-2.0*smooth2(x,y,1)+0.5*smooth1(x,y,2);
}

static double smooth6(double x,double y,int c) {
	int X=floor(x);
	int Y=floor(y);
	double Xf=x-X;
	double Yf=y-Y;
	return cubic(
		cubic(getpixel(X-1,Y-1)[0],getpixel(X-1,Y)[1],getpixel(X-1,Y+1)[1],getpixel(X-1,Y+2)[0],Yf),
		cubic(getpixel(X,Y-1)[1],getpixel(X,Y)[2],getpixel(X,Y+1)[2],getpixel(X,Y+2)[1],Yf),
		cubic(getpixel(X+1,Y-1)[1],getpixel(X+1,Y)[2],getpixel(X+1,Y+1)[2],getpixel(X+1,Y+2)[1],Yf),
		cubic(getpixel(X+2,Y-1)[0],getpixel(X+2,Y)[1],getpixel(X+2,Y+1)[1],getpixel(X+2,Y+2)[0],Yf),
	Xf);
}

static const SmoothFunc smooths[]={
	smooth0, smooth1, smooth2, smooth3, smooth4, smooth5, smooth6,
};

static double turbulence(int i) {
	double x=i%width;
	double y=i/width;
	double s=1<<zoom;
	double r=s;
	double n=0.0;
	if(!option['i']) i=0;
	if(option['r']) r+=pic[i].c[3]/65535.0;
	while(r>=1.0) {
		n+=fsmooth(x/r,y/r,i&3)*pow(r,exponent);
		if(!option['j']) i++;
		r*=0.5;
	}
	return 0.5*n/s;
}

int main(int argc,char**argv) {
	int i,j;
	double n;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	zoom=argc>1?strtol(argv[1],0,0):1;
	smoothfunc=argc>2?strtol(argv[2],0,0):2;
	exponent=argc>3?strtod(argv[3],0):1.0;
	if(smoothfunc<0 || smoothfunc>=sizeof(smooths)/sizeof(*smooths)) {
		fprintf(stderr,"Argument out of range\n");
		return 1;
	}
	if(argc>4) for(i=0;argv[4][i];i++) option[argv[4][i]&127]=1;
	fsmooth=smooths[smoothfunc];
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
	buf[6]=buf[7]=255;
	i=width*height;
	while(i--) {
		n=turbulence(i);
		if(option['c']) {
			j=n<0.5?0:n>65534.5?65535:n;
			buf[0]=j>>8;
			buf[1]=j;
			n=-n;
			j=n<0.5?0:n>65534.5?65535:n;
			buf[2]=j>>8;
			buf[3]=j;
			n-=65535.5;
			j=n<0.5?0:n>65534.5?65535:n;
			buf[4]=j>>8;
			buf[5]=j;
		} else {
			j=n<0.5?0:n>65534.5?65535:n;
			buf[0]=buf[2]=buf[4]=j>>8;
			buf[1]=buf[3]=buf[5]=j;
		}
		fwrite(buf,1,8,stdout);
	}
	return 0;
}

