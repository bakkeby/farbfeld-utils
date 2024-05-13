#if 0
gcc -s -O2 -o ~/bin/ff-matrix ff-matrix.c -D_DEFAULT_SOURCE -lm
exit
#endif

#define _BSD_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static double param[20];
static double pixel[4];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-matrix <float1> <float2> ... <float20>                              \n"
		"                                                                              \n"
		"This farbfeld filter multiplies each pixel (considered as a vector) by a      \n"
		"matrix.                                                                       \n"
		"                                                                              \n"
		"Multiplies each pixel, considered as a vector of RGBA, by a matrix to produce \n"
		"the new RGBA values for that pixel.                                           \n"
		"                                                                              \n"
		"The filter takes twenty arguments.                                            \n"
		"                                                                              \n"
		"The twenty arguments are the matrix coefficients (which are floating numbers).\n"
		"                                                                              \n"
		"The input/output values in each channel are considered as numbers in range 0  \n"
		"to 1, and if the output is out of range then the closest valid value is used. \n"
		"                                                                              \n"
		"Possible use cases for matrix processing:                                     \n"
		"   - image filtering: matrix operations are often used for image filtering    \n"
		"     such as blurring, sharpening, edge detection and embossing (in which case\n"
		"     also see ff-convolve which applies a convolution matrix)                 \n"
		"   - colour transformations: matrix operations can be used to convert between \n"
		"     different colour models like RGB, CMYK, HSL, HSV and YUV                 \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-matrix 0.329 0.410 0.110 0.010                                        \n"
		"               0.982 0.412 0.111 0.234                                        \n"
		"               0.040 0.291 0.781 0.824                                        \n"
		"               0.111 0.434 0.291 0.331                                        \n"
		"               0.221 0.110 0.556 0.689 < image.ff > modified.ff               \n"
		"\n"
	);
	exit(1);
}

static inline void inconv() {
	int i,v;
	for(i=0;i<4;i++) {
		v=(buf[i<<1]<<8)|buf[(i<<1)+1];
		pixel[i]=((double)v)/65535.0;
	}
}

static inline void process() {
	int i;
	double p[4];
	for(i=0;i<4;i++) p[i]=pixel[0]*param[i]+pixel[1]*param[i+4]+pixel[2]*param[i+8]+pixel[3]*param[i+12]+param[i+16];
	for(i=0;i<4;i++) pixel[i]=p[i];
}

static inline void outconv() {
	double v;
	int i,r;
	for(i=0;i<4;i++) {
		v=pixel[i]*65535.0;
		r=0.0<v?v<65535.0?(int)rint(v):65535:0;
		if(v>65535) v=65535;
		buf[i<<1]=r>>8;
		buf[(i<<1)+1]=r;
	}
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	i=fread(buf,1,16,stdin);
	i=fwrite(buf,1,16,stdout);
	for(i=0;i<20;i++) if(argc>i+1) param[i]=strtod(argv[i+1],0);
	while(fread(buf,1,8,stdin)) {
		inconv();
		process();
		outconv();
		i=fwrite(buf,1,8,stdout);
	}
	return 0;
}
