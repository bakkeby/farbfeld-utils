#if 0
gcc -s -O2 -o ~/bin/ff-tocmyk -Wno-unused-result ff-tocmyk.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static float conv[4];
static int data[4];

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-tocmyk <float> <float> <float> <float>                              \n"
		"                                                                              \n"
		"This farbfeld program that converts RGB colours to CMYK.                      \n"
		"                                                                              \n"
		"This program converts a picture in RGB format (on input; alpha is ignored) to \n"
		"CMYK. Instead of red, green, blue, alpha, the channels now correspond to cyan,\n"
		"magenta, yellow, black/key.                                                   \n"
		"                                                                              \n"
		"The program takes four arguments, all of which are floating numbers.          \n"
		"                                                                              \n"
		"The first two are the multiply and add amount for black generation, and the   \n"
		"last two are the multiply and add amount for under removal. This is similar to\n"
		"the algorithm in PostScript.                                                  \n"
		"                                                                              \n"
		"Use ff-matrix to convert CMYK to RGB.                                         \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-tocmyk 1.0 0.0 0.0 0.5 < image.ff > image-cmyk.ff                     \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i;
	if (argc!=5 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	conv[0]=strtod(argv[1],0);
	conv[1]=strtod(argv[2],0)*65535.0;
	conv[2]=strtod(argv[3],0);
	conv[3]=strtod(argv[4],0)*65535.0;
	fread(buf,1,16,stdin);
	fwrite(buf,1,16,stdout);
	while(fread(buf,1,8,stdin)>0) {
		data[0]=65535-((buf[0]<<8)|buf[1]);
		data[1]=65535-((buf[2]<<8)|buf[3]);
		data[2]=65535-((buf[4]<<8)|buf[5]);
		data[3]=data[0];
		if(data[3]>data[1]) data[3]=data[1];
		if(data[3]>data[2]) data[3]=data[2];
		data[0]-=conv[2]*data[3]+conv[3];
		data[1]-=conv[2]*data[3]+conv[3];
		data[2]-=conv[2]*data[3]+conv[3];
		data[3]=conv[0]*data[3]+conv[1];
		if(data[0]<0) data[0]=0; else if(data[0]>65535) data[0]=65535;
		if(data[1]<0) data[1]=0; else if(data[1]>65535) data[1]=65535;
		if(data[2]<0) data[2]=0; else if(data[2]>65535) data[2]=65535;
		if(data[3]<0) data[3]=0; else if(data[3]>65535) data[3]=65535;
		buf[0]=data[0]>>8; buf[1]=data[0];
		buf[2]=data[1]>>8; buf[3]=data[1];
		buf[4]=data[2]>>8; buf[5]=data[2];
		buf[6]=data[3]>>8; buf[7]=data[3];
		fwrite(buf,1,8,stdout);
	}
	return 0;
}
