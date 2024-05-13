#if 0
gcc -s -O2 -o ~/bin/ff-convolve -Wno-unused-result ff-convolve.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M(a,b) matrix[a]=strtol(argv[b],0,0)

static unsigned char buf[16];
static int matrix[100];
static int msize,width,height,omsize,owidth,oheight,bias,divide;
static unsigned char*data;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-convolve <divide amount> <bias amount> <matrix data>                \n"
		"                                                                              \n"
		"This farbfeld filter is used to apply a convolution matrix.                   \n"
		"                                                                              \n"
		"A convolution matrix can be used for blurring, sharpening, embossing, edge    \n"
		"detection and more.                                                           \n"
		"                                                                              \n"
		"The first argument is the divide amount; if zero, then it will select the     \n"
		"pixel in range that has the largest value when multiplied by the element in   \n"
		"the convolution matrix.                                                       \n"
		"                                                                              \n"
		"The second argument is the bias amount, which is added to the result of the   \n"
		"convolution (after division) to determine the output value. They can be       \n"
		"positive or negative or zero. It is irrelevant if the divide amount is zero.  \n"
		"                                                                              \n"
		"The rest of the arguments is the matrix data, which is one integer each (they \n"
		"can be positive or negative or zero).                                         \n"
		"                                                                              \n"
		"Valid sizes are:                                                              \n"
		"   3x3 matrix                                                                 \n"
		"   5x5 matrix                                                                 \n"
		"   7x7 matrix                                                                 \n"
		"                                                                              \n"
		"Other sizes are not acceptable.                                               \n"
		"                                                                              \n"
		"Note that the edges of the input picture will be cropped, since there is not  \n"
		"enough data near the edges to compute the convolution. Use ff-border to add a \n"
		"border first if you want to end up having the same size; in this case the type\n"
		"of border will specify the data to use at the edges.                          \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-convolve 1 0 0 1 0 1 -4 1 0 1 0 < image.ff > edge-detect.ff           \n"
		"   $ ff-convolve 2 0 -2 -1 0 -1 1 1 0 1 2 < image.ff > emboss.ff              \n"
		"\n"
	);
	exit(1);
}

static void process(unsigned char*ptr) {
	int v=0;
	int i,j;
	for(i=0;i<omsize;i++) {
		for(j=0;j<omsize;j++) {
			v+=((ptr[0]<<8)|ptr[1])*matrix[i*10+j];
			ptr+=8;
		}
		ptr+=(width-omsize)<<3;
	}
	v=v/divide+bias;
	if(v<0) v=0;
	if(v>65535) v=65535;
	putchar(v>>8);
	putchar(v);
}

static void process_max(unsigned char*ptr) {
	int v=bias;
	int i,j;
	unsigned char*p=ptr+((width*msize+msize)<<3);
	for(i=0;i<omsize;i++) {
		for(j=0;j<omsize;j++) {
			if(matrix[i*10+j] && v<((ptr[0]<<8)|ptr[1])*matrix[i*10+j]) {
				v=((ptr[0]<<8)|ptr[1])*matrix[i*10+j];
				p=ptr;
			}
			ptr+=8;
		}
		ptr+=(width-omsize)<<3;
	}
	putchar(p[0]);
	putchar(p[1]);
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	switch(argc) {
		case 12:
			msize=1;
			M( 0, 3); M( 1, 4); M( 2, 5);
			M(10, 6); M(11, 7); M(12, 8);
			M(20, 9); M(21,10); M(22,11);
			break;
		case 28:
			msize=2;
			M( 0, 3); M( 1, 4); M( 2, 5); M( 3, 6); M( 4, 7);
			M(10, 8); M(11, 9); M(12,10); M(13,11); M(14,12);
			M(20,13); M(21,14); M(22,15); M(23,16); M(24,17);
			M(30,18); M(31,19); M(32,20); M(33,21); M(34,22);
			M(40,23); M(41,24); M(42,25); M(43,26); M(44,27);
			break;
		case 52:
			msize=3;
			M( 0, 3); M( 1, 4); M( 2, 5); M( 3, 6); M( 4, 7); M( 5, 8); M( 6, 9);
			M(10,10); M(11,11); M(12,12); M(13,13); M(14,14); M(15,15); M(16,16);
			M(20,17); M(21,18); M(22,19); M(23,20); M(24,21); M(25,22); M(26,23);
			M(30,24); M(31,25); M(32,26); M(33,27); M(34,28); M(35,29); M(36,30);
			M(40,31); M(41,32); M(42,33); M(43,34); M(44,35); M(45,36); M(46,37);
			M(50,38); M(51,39); M(52,40); M(53,41); M(54,42); M(55,43); M(56,44);
			M(60,45); M(61,46); M(62,47); M(63,48); M(64,49); M(65,50); M(66,51);
			break;
#if 0
		case 84:
			msize=4;
			break;
#endif
		default:
			fprintf(stderr,"Improper number of arguments\n");
			return 1;
	}
	omsize=(msize<<1)+1;
	divide=strtol(argv[1],0,0);
	bias=strtol(argv[2],0,0);
	fread(buf,1,16,stdin);
	owidth=width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	oheight=height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	owidth-=msize<<1;
	oheight-=msize<<1;
	if(owidth<=0 || oheight<=0) {
		fprintf(stderr,"Width/height too small (must be at least %dx%d)\n",omsize,omsize);
		return 1;
	}
	data=calloc(omsize<<3,width);
	if(!data) {
		fprintf(stderr,"Memory allocation failed\n");
		return 1;
	}
	fwrite(buf,1,8,stdout);
	putchar(owidth>>24);
	putchar(owidth>>16);
	putchar(owidth>>8);
	putchar(owidth>>0);
	putchar(oheight>>24);
	putchar(oheight>>16);
	putchar(oheight>>8);
	putchar(oheight>>0);
	fread(data+(width<<3),width<<4,msize,stdin);
	if(divide) {
		while(oheight--) {
			memmove(data,data+(width<<3),msize*width<<4);
			fread(data+(msize*width<<4),8,width,stdin);
			for(i=0;i<owidth<<2;i++) process(data+(i<<1));
		}
	} else {
		while(oheight--) {
			memmove(data,data+(width<<3),msize*width<<4);
			fread(data+(msize*width<<4),8,width,stdin);
			for(i=0;i<owidth<<2;i++) process_max(data+(i<<1));
		}
	}
	return 0;
}
