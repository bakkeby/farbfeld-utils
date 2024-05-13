#if 0
gcc -s -O2 -o ~/bin/ff-outline -Wno-unused-result ff-outline.c
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[16];
static int width,height;
static unsigned char*pic;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: ff-outline                                                             \n"
		"                                                                              \n"
		"This farbfeld filter add a transparent outline around areas of different      \n"
		"translucency (making the insides all opaque).                                 \n"
		"                                                                              \n"
		"The filter takes no arguments.                                                \n"
		"                                                                              \n"
		"The value in the alpha channel of each pixel specifies what country it belongs\n"
		"to, and then the alpha value is changed to zero to make an outline between    \n"
		"regions or maximum if it is the inside of the region.                         \n"
		"                                                                              \n"
		"Combine with ff-back to make the colour of the lines.                         \n"
		"                                                                              \n"
		"Example usage:                                                                \n"
		"   $ ff-outline < image.ff | ff-back FF0000 > image-outline.ff                \n"
		"\n"
	);
	exit(1);
}

int main(int argc,char**argv) {
	int i,j,k;
	if (argc>1 && (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))) {
		usage();
	}
	fread(buf,1,16,stdin);
	width=(buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	height=(buf[12]<<24)|(buf[13]<<16)|(buf[14]<<8)|buf[15];
	pic=malloc(width<<4); // two rows
	if(!pic) {
		fprintf(stderr,"Allocation failed\n");
		return 1;
	}
	fwrite(buf,1,16,stdout);
	fread(pic,8,width,stdin);
	memcpy(pic+8*width,pic,8*width);
	while(height--) {
		j=height&1?0:8*width;
		k=height&1?8*width:0;
		for(i=0;i<width;i++) {
			fwrite(pic+j,1,6,stdout);
			if(pic[j+6]==pic[k+6] && pic[j+7]==pic[k+7] && (!i || (pic[j+6]==pic[j-2] && pic[j+7]==pic[j-1]))) {
				putchar(255); putchar(255);
			} else {
				putchar(0); putchar(0);
			}
			j+=8;
			k+=8;
		}
		fread(pic+(height&1?8*width:0),8,width,stdin);
	}
	return 0;
}

