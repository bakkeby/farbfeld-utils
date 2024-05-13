#if 0
gcc -s -O2 -o ~/bin/ff-enlarge -Wno-unused-result ff-enlarge.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char data[8];
} Pixel;

static unsigned char buf[16];
static int width,height,xlarge,ylarge;
static Pixel*row;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-enlarge <?x factor?> <?y factor?>                                   \n"
		"                                                                              \n"
		"This farbfeld filter is used to TODO                                          \n"
		"                                                                              \n"
		"Enlarges a picture by making \"large pixels\", i.e. it does not interpolate.  \n"
		"                                                                              \n"
		"The filter takes up to two optional arguments:                                \n"
		"   X factor (defaults to 2)                                                   \n"
		"   Y factor (defaults to value of X factor)                                   \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-enlarge < image.ff > image-enlarged-2x.ff                             \n"
		"   $ ff-enlarge 3 < image.ff > image-enlarged-3x.ff                           \n"
		"   $ ff-enlarge 1 3 < image.ff > image-enlarge.ff                             \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i,j,k;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	xlarge=argc>1?strtol(argv[1],0,0):2;
	ylarge=argc>2?strtol(argv[2],0,0):xlarge;
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	row=calloc(8,width);
	if(!row) {
		fprintf(stderr,"Out of memory\n");
		return 1;
	}
	buf[8]=(width*xlarge)>>24;
	buf[9]=(width*xlarge)>>16;
	buf[10]=(width*xlarge)>>8;
	buf[11]=(width*xlarge);
	buf[12]=(height*ylarge)>>24;
	buf[13]=(height*ylarge)>>16;
	buf[14]=(height*ylarge)>>8;
	buf[15]=(height*ylarge);
	fwrite(buf,1,16,stdout);
	while(height--) {
		fread(row,8,width,stdin);
		for(i=0;i<ylarge;i++) for(j=0;j<width;j++) for(k=0;k<xlarge;k++) fwrite(row+j,8,1,stdout);
	}
	return 0;
}
