#if 0
gcc -s -O2 -o ~/bin/ff-blur -Wno-unused-result ff-blur.c -lm
exit
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	double v[4];
} Pixel;

static unsigned char buf[16];
static int width,height;
static Pixel*pic;
static double*kernel;
static int ksize;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-blur <kernel size> <standard deviation>                             \n"
		"                                                                              \n"
		"This farbfeld filter applies Gaussian blur to the input picture.              \n"
		"                                                                              \n"
		"The filter takes two arguments, which are the kernel size (a positive         \n"
		"integer), and the standard deviation (a nonzero real number).                 \n"
		"                                                                              \n"
		"The kernel size is the amount that each edge of the picture is cropped, as    \n"
		"well as setting the filter quality; larger numbers will result better quality \n"
		"filter.                                                                       \n"
		"                                                                              \n"
		"If the standard deviation is x, then the recommended kernel size is (3x+1),   \n"
		"rounded up.                                                                   \n"
		"                                                                              \n"
		"Tip: use ff-border to add a border if you want the output of the Gaussian blur\n"
		"to be the same size as the input.                                             \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-blur 9 2.399 < image.ff > image-blur.ff                               \n"
		"\n"
	);
	exit(1);
}

static void make_kernel(double v) {
	int i;
	double t=1.0;
	v=2.0*v*v;
	kernel=malloc((ksize+1)*sizeof(double));
	if(!kernel) {
		fprintf(stderr,"Allocation failed\n");
		exit(1);
	}
	for(i=1;i<=ksize;i++) t+=exp(-i*i/v)*2.0;
	for(i=0;i<=ksize;i++) kernel[i]=exp(-i*i/v)/t;
}

static void convolve_column(Pixel*p) {
	int y,i,j;
	double d;
	for(y=ksize;y<height-ksize;y++) {
		for(i=0;i<4;i++) {
			d=*kernel*p[y*width].v[i];
			for(j=1;j<=ksize;j++) d+=kernel[j]*(p[(y-j)*width].v[i]+p[(y+j)*width].v[i]);
			p[(y-ksize)*width].v[i]=d;
		}
	}
}

static void convolve_row(Pixel*p) {
	int x,i,j,k;
	double d;
	for(x=ksize;x<width-ksize;x++) {
		for(i=0;i<4;i++) {
			d=*kernel*p[x].v[i];
			for(j=1;j<=ksize;j++) d+=kernel[j]*(p[x-j].v[i]+p[x+j].v[i]);
			k=d<=0.0?0:d>=65535.0?65535:(int)d;
			putchar(k>>8);
			putchar(k);
		}
	}
}

int main(int argc,char**argv) {
	int x,y;
	if(argc<3 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	ksize=strtol(argv[1],0,0);
	make_kernel(strtod(argv[2],0));
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	if(width<=ksize+ksize || height<=ksize+ksize) {
		fprintf(stderr,"Picture is too small\n");
		return 1;
	}
	buf[8]=(width-ksize-ksize)>>24;
	buf[9]=(width-ksize-ksize)>>16;
	buf[10]=(width-ksize-ksize)>>8;
	buf[11]=(width-ksize-ksize)>>0;
	buf[12]=(height-ksize-ksize)>>24;
	buf[13]=(height-ksize-ksize)>>16;
	buf[14]=(height-ksize-ksize)>>8;
	buf[15]=(height-ksize-ksize)>>0;
	pic=malloc(width*height*sizeof(Pixel));
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	// Read picture
	for(y=0;y<height;y++) for(x=0;x<width;x++) {
		fread(buf,1,8,stdin);
		pic[y*width+x].v[0]=(buf[0]<<8)|buf[1];
		pic[y*width+x].v[1]=(buf[2]<<8)|buf[3];
		pic[y*width+x].v[2]=(buf[4]<<8)|buf[5];
		pic[y*width+x].v[3]=(buf[6]<<8)|buf[7];
	}
	// Do vertical convolution
	for(x=0;x<width;x++) convolve_column(pic+x);
	// Do horizontal convolution and write output
	for(y=0;y<height-ksize-ksize;y++) convolve_row(pic+y*width);
	// Finished
	return 0;
}

