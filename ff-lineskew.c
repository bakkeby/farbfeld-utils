#if 0
gcc -s -O2 -o ~/bin/ff-lineskew -Wno-unused-result ff-lineskew.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,height,skew;
static unsigned char*data;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-lineskew <?pixels?>                                                 \n"
		"                                                                              \n"
		"This farbfeld filter skews a picture horizontally by an integer number of     \n"
		"pixels per scanline and writes the result to stdout.                          \n"
		"                                                                              \n"
		"The argument is how many pixels to skew additionally per each scanline.       \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-lineskew 1 < image.ff > image-lineskew.ff                             \n"
		"   $ ff-lineskew -2 < image.ff > image-lineskew.ff                            \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	if(argc>1) skew=strtol(argv[1],0,10);
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	while(skew<0) skew+=width;
	skew*=8;
	data=malloc(width*=8);
	if(!data) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	i=0;
	while(height--) {
		fread(data,1,width,stdin);
		fwrite(data+i,1,width-i,stdout);
		fwrite(data,1,i,stdout);
		i=(i+skew)%width;
	}
	return 0;
}

