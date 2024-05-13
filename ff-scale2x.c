#if 0
gcc -s -O2 -o ~/bin/ff-scale2x -Wno-unused-result ff-scale2x.c
exit
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,height;
static int64_t*pic;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-scale2x                                                             \n"
		"                                                                              \n"
		"This farbfeld filter applies a Scale2x effect on an image and writes the      \n"
		"result to stdout.                                                             \n"
		"                                                                              \n"
		"The output image will be twice the width and twice the height of the original \n"
		"image.                                                                        \n"
		"                                                                              \n"
		"The filter takes no arguments.                                                \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-scale2x < image.ff > image-scale2x.ff                                 \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int x,y,z;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	pic=malloc(8*width*height);
	if(!pic || sizeof(int64_t)!=8) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	buf[8]=width>>23;
	buf[9]=width>>15;
	buf[10]=width>>7;
	buf[11]<<=1;
	buf[12]=height>>23;
	buf[13]=height>>15;
	buf[14]=height>>7;
	buf[15]<<=1;
	fwrite(buf,1,16,stdout);
	fread(pic,width<<3,height,stdin);
#define B pic[x-width]
#define D pic[x-1]
#define E pic[x]
#define F pic[x+1]
#define H pic[x+width]
	for(y=0;y<height;y++) {
		for(z=0;z<2;z++) for(x=0;x<width;x++) {
			if(!x || !y || x==width-1 || y==height-1) {
				fwrite(pic+x,1,8,stdout);
				fwrite(pic+x,1,8,stdout);
			} else if(B!=H && D!=F) {
				fwrite(D==(z?H:B)?&D:&E,1,8,stdout);
				fwrite(F==(z?H:B)?&F:&E,1,8,stdout);
			} else {
				fwrite(pic+x,1,8,stdout);
				fwrite(pic+x,1,8,stdout);
			}
		}
		pic+=width;
	}
	return 0;
}
