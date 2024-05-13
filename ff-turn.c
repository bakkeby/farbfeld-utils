#if 0
gcc -s -O2 -o ~/bin/ff-turn -Wno-unused-result ff-turn.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char header[16];
static char mode;
static int width,height;
static char*data;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-turn <mode>                                                         \n"
		"                                                                              \n"
		"This farbfeld filter is used to flip, mirror, and/or rotate a picture.        \n"
		"                                                                              \n"
		"The filter takes one argument which is a number from 0 to 7 specifying what to\n"
		"do, as a sum of the following:                                                \n"
		"   1 -- reverse Y axis                                                        \n"
		"   2 -- reverse X axis                                                        \n"
		"   4 -- swap X with Y axis (rotate 90 degrees)                                \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-turn 3 < image.ff > image-rotated-180-degrees.ff                      \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i,x,y;
	if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"--help")) {
		usage();
	}
	fread(header,1,16,stdin);
	width=(header[8]<<24)|(header[9]<<16)|(header[10]<<8)|header[11];
	height=(header[12]<<24)|(header[13]<<16)|(header[14]<<8)|header[15];
	mode=strtol(argv[1],0,0)&7;
	if(mode&4) {
		for(x=0;x<4;x++) i=header[x+8],header[x+8]=header[x+12],header[x+12]=i;
	}
	fwrite(header,1,16,stdout);
	data=malloc(width*height*8);
	if(!data) {
		fprintf(stderr,"Out of memory\n");
		return 1;
	}
	fread(data,8,width*height,stdin);
	switch(mode) {
		case 0: for(y=0;y<height;y++) for(x=0;x<width;x++) fwrite(data+(y*width+x)*8,1,8,stdout); break;
		case 1: for(y=height-1;y>=0;y--) for(x=0;x<width;x++) fwrite(data+(y*width+x)*8,1,8,stdout); break;
		case 2: for(y=0;y<height;y++) for(x=width-1;x>=0;x--) fwrite(data+(y*width+x)*8,1,8,stdout); break;
		case 3: for(y=height-1;y>=0;y--) for(x=width-1;x>=0;x--) fwrite(data+(y*width+x)*8,1,8,stdout); break;
		case 4: for(x=0;x<width;x++) for(y=0;y<height;y++) fwrite(data+(y*width+x)*8,1,8,stdout); break;
		case 5: for(x=0;x<width;x++) for(y=height-1;y>=0;y--) fwrite(data+(y*width+x)*8,1,8,stdout); break;
		case 6: for(x=width-1;x>=0;x--) for(y=0;y<height;y++) fwrite(data+(y*width+x)*8,1,8,stdout); break;
		case 7: for(x=width-1;x>=0;x--) for(y=height-1;y>=0;y--) fwrite(data+(y*width+x)*8,1,8,stdout); break;
	}
	return 0;
}

