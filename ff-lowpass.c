#if 0
gcc -s -O2 -o ~/bin/ff-lowpass -Wno-unused-result ff-lowpass.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int width,height;
static unsigned char buf[16];
static double real[4];
static double imag[4];
static double p1,p2,p3;
static char d1,d2,d3;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-lowpass <?float?> <?float?> <?float?>                               \n"
		"                                                                              \n"
		"This farbfeld filter generates a simple IIR low pass filter to each scanline. \n"
		"                                                                              \n"
		"The filter takes up to three arguments arguments (all floating-point numbers):\n"
		"   - real part of filter coefficient (default 0.5) - zero means no filter     \n"
		"   - imaginary part of filter coefficient (default 0)                         \n"
		"   - bias amount (default 0.5) - this number is added to the result before    \n"
		"     clipping to range 0 to 65535                                             \n"
		"                                                                              \n"
		"A forward slash (/) can be used in place of any value in order to use the     \n"
		"alpha channel instead.                                                        \n"
		"                                                                              \n"
		"The imaginary part of the result is not used for output, although it is used  \n"
		"for calculation of further pixels in the scanline.                            \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-lowpass 0.73 0.49 0.11 < image.ff > image-lowpass.ff                  \n"
		"   $ ff-lowpass / 0.009 0.11 < image.ff > image-lowpass.ff                    \n"
		"   $ ff-lowpass 0.003 / 0.94 < image.ff > image-lowpass.ff                    \n"
		"\n"
	);
	exit(1);
}

static void process(int x,int y) {
	double v=(buf[y+y]<<8)|buf[y+y+1];
	double w=0.0;
	if(d1) p1=((buf[6]<<8)|buf[7])/65535.0;
	if(d2) p2=((buf[6]<<8)|buf[7])/65535.0;
	if(d3) p3=((buf[6]<<8)|buf[7])-32767.25;
	if(x) {
		w=p1*imag[y]+p2*(real[y]-v);
		v+=p1*real[y]-p1*v-p2*imag[y];
	}
	real[y]=v;
	imag[y]=w;
	v+=p3;
	y=v<0.1?0:v>=65535.0?65535:v;
	putchar(y>>8);
	putchar(y);
}

int main(int argc,char**argv) {
	int x;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	p1=argc>1?strtod(argv[1],0):0.5;
	p2=argc>2?strtod(argv[2],0):0.0;
	p3=argc>3?strtod(argv[3],0):0.5;
	if(argc>1 && argv[1][0]=='/') d1=1;
	if(argc>2 && argv[2][0]=='/') d2=1;
	if(argc>3 && argv[3][0]=='/') d3=1;
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	while(height--) {
		for(x=0;x<width;x++) {
			fread(buf,1,8,stdin);
			process(x,0);
			process(x,1);
			process(x,2);
			process(x,3);
		}
	}
	return 0;
}
